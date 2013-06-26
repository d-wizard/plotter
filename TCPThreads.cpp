/* Copyright 2013 Dan Williams. All Rights Reserved.
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
#include "TCPThreads.h"

void dServerSocket_updateIndexForNextPacket(dSocketRxBuff* rxBuff)
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

void dServerSocket_writeNewPacket(dClientConnection* dConn, char* packet, unsigned int size)
{
   dConn->rxBuff.packetAddr[dConn->rxBuff.writeIndex] = packet;
   dConn->rxBuff.packetSize[dConn->rxBuff.writeIndex] = size;
   if(++dConn->rxBuff.writeIndex == MAX_STORED_PACKETS)
   {
      dConn->rxBuff.writeIndex = 0;
   }
}

void dServerSocket_readAllPackets(dClientConnection* dConn)
{
   while(dConn->rxBuff.readIndex != dConn->rxBuff.writeIndex)
   {
      dConn->rxPacketCallback(
         dConn->callbackInputPtr,
         &dConn->info,
         dConn->rxBuff.packetAddr[dConn->rxBuff.readIndex],
         dConn->rxBuff.packetSize[dConn->rxBuff.readIndex] );
      if(++dConn->rxBuff.readIndex == MAX_STORED_PACKETS)
      {
         dConn->rxBuff.readIndex = 0;
      }
   }
}

void dServerSocket_initClientConn(dClientConnection* dConn, dServerSocket* dSock)
{
   memset(dConn, 0, sizeof(dClientConnection));
   dConn->fd = -1;
   dConn->rxBuff.maxPacketSize = MAX_PACKET_SIZE;
   dConn->rxBuff.buffPtr = (char*)dConn->rxBuff.buff;
   dConn->rxPacketCallback = dSock->rxPacketCallback;
   dConn->callbackInputPtr = dSock->callbackInputPtr;
   dConn->killThisThreadSem = &dSock->killThreadSem;

}

void dServerSocket_wsaInit()
{
   WSADATA wsda;
   WSAStartup(0x0101, &wsda);
}

void dServerSocket_init(dServerSocket* dSock, unsigned short port, dRxPacketCallback rxPacketCallback, void* inputPtr)
{
   unsigned int index;
   memset(dSock, 0, sizeof(dSock));
   dSock->port = port;
   dSock->rxPacketCallback = rxPacketCallback;
   dSock->callbackInputPtr = inputPtr;

   sem_init(&dSock->killThreadSem, 0, 0);

   for(index = 0; index < MAX_CONNECTIONS; ++index)
   {
      dServerSocket_initClientConn(&dSock->clients[index], dSock);
   }

   dServerSocket_wsaInit();
}

void* dServerSocket_acceptThread(void* voidDSock)
{
   dServerSocket* dSock = (dServerSocket*)voidDSock;
   struct sockaddr_storage clientAddr;
   SOCKET connectionFd = 0;

   dSock->acceptThread.active = 1;
   printf("acceptThread start\n");

   while(!dSock->acceptThread.kill)
   {
      if( listen(dSock->socketFd, 10) != -1)
      {
         socklen_t sockStoreSize = sizeof(clientAddr);
         connectionFd = accept(dSock->socketFd, (struct sockaddr*)&clientAddr, &sockStoreSize);
         if(connectionFd != -1)
         {
            dServerSocket_newClientConn(dSock, connectionFd, &clientAddr);
         }
      }
      else
      {
         dSock->acceptThread.kill = 1;
      }
   }
   dSock->acceptThread.active = 0;
   printf("acceptThread end\n");

   return NULL;
}

void* dServerSocket_rxThread(void* voidDClientConn)
{
   dClientConnection* dClientConn = (dClientConnection*)voidDClientConn;
   int packetSize = 0;

   dClientConn->rxThread.active = 1;
   printf("rxThread start\n");
   while(!dClientConn->rxThread.kill)
   {
      dServerSocket_updateIndexForNextPacket(&dClientConn->rxBuff);

      packetSize = recv( dClientConn->fd, 
                         &dClientConn->rxBuff.buffPtr[dClientConn->rxBuff.buffIndex], 
                         dClientConn->rxBuff.maxPacketSize, 
                         0 );
      if(packetSize > 0)
      {
         dServerSocket_writeNewPacket( dClientConn, 
                                       &dClientConn->rxBuff.buffPtr[dClientConn->rxBuff.buffIndex], 
                                       packetSize );
         sem_post(&dClientConn->sem);
         dClientConn->rxBuff.buffIndex += packetSize;
      }
      else
      {
         printf("rxThread wrong packet size\n");
         dClientConn->rxThread.kill = 1;
      }

   }
   dClientConn->rxThread.active = 0;
   printf("rxThread end\n");

   sem_post(dClientConn->killThisThreadSem);

   return NULL;
}

void* dServerSocket_procThread(void* voidDClientConn)
{
   dClientConnection* dClientConn = (dClientConnection*)voidDClientConn;
   dClientConn->procThread.active = 1;
   printf("procThread start\n");
   while(!dClientConn->procThread.kill)
   {
      sem_wait(&dClientConn->sem);
      dServerSocket_readAllPackets(dClientConn);
   }
   dClientConn->procThread.active = 0;
   printf("procThread end\n");

   return NULL;
}

void* dServer_killThreads(void* voidDSock)
{
   dServerSocket* dSock = (dServerSocket*)voidDSock;
   unsigned int index = 0;
   
   dSock->killThread.active = 1;
   printf("killThread Start\n");
   while(!dSock->killThread.kill)
   {
      sem_wait(&dSock->killThreadSem);
      for(index = 0; index < MAX_CONNECTIONS; ++index)
      {
         if( dSock->clients[index].fd != -1 && 
             dSock->clients[index].rxThread.active == 0 && 
             dSock->clients[index].rxThread.kill == 1 )
         {
            pthread_join(dSock->clients[index].rxThread.thread, NULL);
            if(dSock->clients[index].procThread.active)
            {
               dSock->clients[index].procThread.kill = 1;
               sem_post(&dSock->clients[index].sem);
               pthread_join(dSock->clients[index].procThread.thread, NULL);
            }

            dServerSocket_initClientConn(&dSock->clients[index], dSock);
         }
      }
   }

   dSock->killThread.active = 0;
   printf("killThread end\n");

   return NULL;
}

void dServerSocket_bind(dServerSocket* dSock)
{
   struct addrinfo hints;
   struct addrinfo* serverInfoList;
   struct addrinfo* serverInfo;
   SOCKET socketFd;

   char portStr[6];

   int success = 0;

   dSock->socketFd = -1;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;// AF_UNSPEC; IPv4 only for now
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE; // use my IP

   _snprintf( portStr, sizeof(portStr), "%d", dSock->port);

   if( !getaddrinfo(NULL, portStr, &hints, &serverInfoList) )
   {
      for(serverInfo = serverInfoList; serverInfo != NULL; serverInfo = serverInfo->ai_next)
      {
         socketFd = socket( serverInfo->ai_family,
                            serverInfo->ai_socktype,
                            serverInfo->ai_protocol );
         if(socketFd != -1)
         {
            int numParam = 1;
            if( setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (char*)&numParam, sizeof(numParam)) != -1 )
            {
               if( bind(socketFd, serverInfo->ai_addr, (int)serverInfo->ai_addrlen) != -1)
               {
                  dSock->socketFd = socketFd;
                  success = 1;
                  // Only use the first successfull bind, might need to use all for IPv4 and IPv6 ???
                  break;
               }
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

void dServerSocket_accept(dServerSocket* dSock)
{
   pthread_create(&dSock->acceptThread.thread, NULL, dServerSocket_acceptThread, dSock);
   pthread_create(&dSock->killThread.thread, NULL, dServer_killThreads, dSock);
}

dClientConnection* dServerSocket_findAvailableClientConn(dServerSocket* dSock)
{
   unsigned int index = 0;
   for(index = 0; index < MAX_CONNECTIONS; ++index)
   {
      if(dSock->clients[index].fd == -1)
      {
         return &dSock->clients[index];
      }
   }
   return dSock->clients;
}

void dServerSocket_newClientConn(dServerSocket* dSock, SOCKET clientFd, struct sockaddr_storage* clientAddr)
{
   dClientConnection* dConn = dServerSocket_findAvailableClientConn(dSock);
   dConn->fd = clientFd;
   dConn->info = *clientAddr;
   sem_init(&dConn->sem, 0, 0);
   pthread_create((pthread_t*)&dConn->rxThread, NULL, dServerSocket_rxThread, dConn);
   pthread_create((pthread_t*)&dConn->procThread, NULL, dServerSocket_procThread, dConn);
   printf("New Client %d\n", clientFd);
}

void dServerSocket_killAll(dServerSocket* dSock)
{
   unsigned int index = 0;
   // Kill All Client Threads
   for(index = 0; index < MAX_CONNECTIONS; ++index)
   {
      if(dSock->clients[index].procThread.active)
      {
         dSock->clients[index].procThread.kill = 1;
         sem_post(&dSock->clients[index].sem);
         pthread_join(dSock->clients[index].procThread.thread, NULL);
      }
      if(dSock->clients[index].rxThread.active)
      {
         dSock->clients[index].rxThread.kill = 1;
         closesocket(dSock->clients[index].fd);
         pthread_join(dSock->clients[index].rxThread.thread, NULL);
      }
   }

   // Kill remaining threads
   if(dSock->killThread.active)
   {
      dSock->killThread.kill = 1;
      sem_post(&dSock->killThreadSem);
      pthread_join(dSock->killThread.thread, NULL);
   }
   if(dSock->acceptThread.active)
   {
      dSock->acceptThread.kill = 1;
      closesocket(dSock->socketFd);
      pthread_join(dSock->acceptThread.thread, NULL);
   }
}

void RxPacketCallback(struct sockaddr_storage* client, char* packet, unsigned int size)
{
   // Debug Print Code
   if(size > 0)
   {
      packet[size-1] = 0;
      if(packet[size-2] == '\r')
      {
         packet[size-2] = '\n';
      }
      printf("%d|%s", size, packet);
   }
   else
   {
      printf("0 Size Packet");
   }
}
