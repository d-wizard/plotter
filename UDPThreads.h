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
#ifndef UDPThreads_h
#define UDPThreads_h

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

#if (defined(_WIN32) || defined(__WIN32__))
   #define UDP_THREADS_WIN_BUILD
#else
   #define UDP_THREADS_LINUX_BUILD
#endif

#ifdef UDP_THREADS_WIN_BUILD
   #ifdef __MINGW32_VERSION
      #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
      #endif
      #define _WIN32_WINNT 0x501
   #endif
   #include <winsock2.h>
   #include <windows.h>
   #include <ws2tcpip.h>
   
#elif defined UDP_THREADS_LINUX_BUILD
   #include <unistd.h>
   #include <string.h>
   #include <netdb.h>
   #include <sys/types.h>
   #include <netinet/in.h>
   #include <sys/socket.h>
   
   typedef int SOCKET;
   #define closesocket(fd) close(fd)
#endif

#define UDP_MAX_PACKET_SIZE ((2*1024*1024) + 256)
#define UDP_MAX_PACKETS (20)
#define UDP_MAX_STORED_PACKETS (2048)

#define INVALID_FD (0xFFFFFFFF)

typedef struct
{
   pthread_t thread;
   // Multiple threads could read/write to these parameters, so make them volatile.
   volatile int active;
   volatile int kill;
}dUDPThread;

typedef struct
{
   unsigned int buff[((UDP_MAX_PACKET_SIZE*UDP_MAX_PACKETS)+(sizeof(unsigned int)-1))/sizeof(unsigned int)];

   char* buffPtr;
   unsigned int buffIndex;
   unsigned int maxPacketSize;

   char* packetAddr[UDP_MAX_STORED_PACKETS];
   unsigned int packetSize[UDP_MAX_STORED_PACKETS];
   struct sockaddr_storage packetSource[UDP_MAX_STORED_PACKETS];
   unsigned int readIndex;
   unsigned int writeIndex;
}dUDPRxBuff;


typedef void (*dRxUDPCallback)(void*, struct sockaddr_storage*, char*, unsigned int);

typedef struct
{
   unsigned short port;
   SOCKET socketFd;
   dRxUDPCallback rxPacketCallback;
   void* callbackValuePtr;
   pthread_mutex_t mutex;

   dUDPThread rxThread;
   dUDPThread procThread;
   sem_t dataReadySem;
   dUDPRxBuff rxBuff;
}dUDPSocket;


void dUDPSocket_wsaInit();
void dUDPSocket_start(dUDPSocket* dSock,
                      unsigned short port,
                      dRxUDPCallback rxPacketCallback,
                      void* callbackValuePtr);
void dUDPSocket_bind(dUDPSocket* dSock);

void* dUDPSocket_rxThread(void* voidDUDPSocket);
void* dUDPSocket_procThread(void* voidDUDPSocket);

void dUDPSocket_stop(dUDPSocket* dSock);
void dUDPSocket_writeNewPacket(dUDPRxBuff* rxBuff, char* packet, unsigned int size, struct sockaddr_storage* source);
void dUDPSocket_readAllPackets(dUDPSocket* udpSock);
void dUDPSocket_updateIndexForNextPacket(dUDPRxBuff* rxBuff);

#endif

