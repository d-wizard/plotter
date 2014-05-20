/* Copyright 2014 Dan Williams. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "UDPThreads.h"


#if defined UDP_THREADS_WIN_BUILD && !defined __MINGW32_VERSION
   #ifdef __cplusplus
      #define snprintf _snprintf_s
   #else
      #define snprintf _snprintf
   #endif
#endif

void dUDPSocket_updateIndexForNextPacket(dUDPRxBuff* rxBuff)
{
   unsigned int buffIndex = (rxBuff->buffIndex + 3) & ((unsigned int)-4);
   if( (buffIndex + rxBuff->maxPacketSize) >= sizeof(rxBuff->buff))
   {
      rxBuff->buffIndex = 0;
   }
   else
   {
      rxBuff->buffIndex = buffIndex;
   }
}

void dUDPSocket_writeNewPacket(dUDPRxBuff* rxBuff, char* packet, unsigned int size, struct sockaddr_storage* source)
{
   rxBuff->packetAddr[rxBuff->writeIndex] = packet;
   rxBuff->packetSize[rxBuff->writeIndex] = size;
   rxBuff->packetSource[rxBuff->writeIndex] = *source;
   if(++rxBuff->writeIndex == UDP_MAX_STORED_PACKETS)
   {
      rxBuff->writeIndex = 0;
   }
}

void dUDPSocket_readAllPackets(dUDPSocket* udpSock)
{
   while(udpSock->rxBuff.readIndex != udpSock->rxBuff.writeIndex)
   {
      udpSock->rxPacketCallback(
         udpSock->callbackValuePtr,
         &udpSock->rxBuff.packetSource[udpSock->rxBuff.readIndex],
         udpSock->rxBuff.packetAddr[udpSock->rxBuff.readIndex],
         udpSock->rxBuff.packetSize[udpSock->rxBuff.readIndex] );
      if(++udpSock->rxBuff.readIndex == UDP_MAX_STORED_PACKETS)
      {
         udpSock->rxBuff.readIndex = 0;
      }
   }
}

#ifdef UDP_THREADS_WIN_BUILD
void dUDPSocket_wsaInit()
{
   WSADATA wsda;
   WSAStartup(0x0101, &wsda);
}
#endif


void dUDPSocket_start(dUDPSocket* dSock,
                      unsigned short port,
                      dRxUDPCallback rxPacketCallback,
                      void* callbackValuePtr)
{
   memset(dSock, 0, sizeof(dUDPSocket));
   dSock->port = port;
   dSock->rxPacketCallback = rxPacketCallback;
   dSock->callbackValuePtr = callbackValuePtr;
   dSock->rxBuff.maxPacketSize = UDP_MAX_PACKET_SIZE;
   dSock->rxBuff.buffPtr = (char*)dSock->rxBuff.buff;

#ifdef UDP_THREADS_WIN_BUILD
   dUDPSocket_wsaInit();
#endif
   
   dUDPSocket_bind(dSock);
   
   if(dSock->socketFd != INVALID_FD)
   {
      sem_init(&dSock->dataReadySem, 0, 0);
      pthread_mutex_init(&dSock->mutex, NULL);
      
      pthread_create((pthread_t*)&dSock->rxThread, NULL, dUDPSocket_rxThread, dSock);
      pthread_create((pthread_t*)&dSock->procThread, NULL, dUDPSocket_procThread, dSock);
   }
}

void* dUDPSocket_rxThread(void* voidDUDPSocket)
{
   dUDPSocket* dSock = (dUDPSocket*)voidDUDPSocket;
   int packetSize = 0;
   struct sockaddr_storage sourceAddr;
   socklen_t sourceAddrLen = sizeof(sourceAddr);

   dSock->rxThread.active = 1;
   printf("rxThread start\n");
   while(!dSock->rxThread.kill)
   {
      dUDPSocket_updateIndexForNextPacket(&dSock->rxBuff);

      packetSize = recvfrom( dSock->socketFd, 
                             &dSock->rxBuff.buffPtr[dSock->rxBuff.buffIndex], 
                             dSock->rxBuff.maxPacketSize, 
                             0,
                             (struct sockaddr *)&sourceAddr,
                             &sourceAddrLen);
      if(packetSize > 0)
      {
         dUDPSocket_writeNewPacket( &dSock->rxBuff, 
                                    &dSock->rxBuff.buffPtr[dSock->rxBuff.buffIndex], 
                                    packetSize,
                                    &sourceAddr );
         sem_post(&dSock->dataReadySem);
         dSock->rxBuff.buffIndex += packetSize;
      }
      else
      {
         printf("rxThread wrong packet size\n");
      }

   }
   dSock->rxThread.active = 0;
   printf("rxThread end\n");

   return NULL;
}

void* dUDPSocket_procThread(void* voidDUDPSocket)
{
   dUDPSocket* dSock = (dUDPSocket*)voidDUDPSocket;
   dSock->procThread.active = 1;
   printf("procThread start\n");

   do
   {
      sem_wait(&dSock->dataReadySem);
      dUDPSocket_readAllPackets(dSock);
   }while(!dSock->procThread.kill);

   dSock->procThread.active = 0;
   printf("procThread end\n");

   return NULL;
}

void dUDPSocket_bind(dUDPSocket* dSock)
{
   struct addrinfo hints;
   struct addrinfo* serverInfoList;
   struct addrinfo* serverInfo;
   SOCKET socketFd;

   char portStr[6];

   int success = 0;

   dSock->socketFd = INVALID_FD;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;// AF_UNSPEC; IPv4 only for now
   hints.ai_socktype = SOCK_DGRAM;
   hints.ai_flags = AI_PASSIVE; // use my IP

   snprintf( portStr, sizeof(portStr), "%d", dSock->port);

   if( !getaddrinfo(NULL, portStr, &hints, &serverInfoList) )
   {
      for(serverInfo = serverInfoList; serverInfo != NULL; serverInfo = serverInfo->ai_next)
      {
         socketFd = socket( serverInfo->ai_family,
                            serverInfo->ai_socktype,
                            serverInfo->ai_protocol );
         if(socketFd != INVALID_FD)
         {
            if( bind(socketFd, serverInfo->ai_addr, (int)serverInfo->ai_addrlen) != -1)
            {
               dSock->socketFd = socketFd;
               success = 1;
               // Only use the first successfull bind, might need to use all for IPv4 and IPv6 ???
               break;
            }
            
            if(!success)
            {
               closesocket(socketFd);
            }
         }
      }
   }

   freeaddrinfo(serverInfoList);

}

void dUDPSocket_stop(dUDPSocket* dSock)
{
   if( (int)dSock->socketFd > 0)
   {
      // Kill the accept thread.
      while(dSock->rxThread.active == 0 && dSock->rxThread.kill == 0); // Make sure it has actually started.
      dSock->rxThread.kill = 1;

#ifdef UDP_THREADS_WIN_BUILD
      closesocket(dSock->socketFd);
#elif defined UDP_THREADS_LINUX_BUILD
      shutdown(dSock->socketFd, 2);
#endif
      pthread_join(dSock->rxThread.thread, NULL);

#ifdef UDP_THREADS_LINUX_BUILD
      close(dSock->socketFd);
#endif
      // Kill the kill thread
      while(dSock->procThread.active == 0 && dSock->procThread.kill == 0); // Make sure it has actually started.
      dSock->procThread.kill = 1;
      
      sem_post(&dSock->dataReadySem);
      pthread_join(dSock->procThread.thread, NULL);

      sem_destroy(&dSock->dataReadySem);
      pthread_mutex_destroy(&dSock->mutex);
      
      dSock->socketFd = INVALID_FD;
   }
}

