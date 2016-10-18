/* Copyright 2016 Dan Williams. All Rights Reserved.
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
#ifndef sendTCPPacket_h
#define sendTCPPacket_h

#if (defined(_WIN32) || defined(__WIN32__))
#define SEND_MSG_TCP_WIN_BUILD
#else
#define SEND_MSG_TCP_LINUX_BUILD
#endif

#ifdef SEND_MSG_TCP_WIN_BUILD
   #include <stdio.h>
   #include <winsock2.h>
   #include <windows.h>
   #include <ws2tcpip.h>

   #ifndef inline
      #define inline __inline
   #endif
   
#elif defined SEND_MSG_TCP_LINUX_BUILD
   #include <stdio.h>
   #include <unistd.h>
   #include <string.h>
   #include <netdb.h>
   #include <sys/types.h>
   #include <netinet/in.h>
   #include <sys/socket.h>

   #ifndef SOCKET
      #define SOCKET int
   #endif
   #ifndef closesocket
      #define closesocket close
   #endif
#endif


static inline int sendTCPPacket_init(const char* hostName, unsigned short port)
{
   SOCKET sockfd = -1; // Initialize to invalid value.
   struct addrinfo hints;
   struct addrinfo* serverInfoList;
   struct addrinfo* servInfo;
   int returnErrorCode;

   char portStr[16];

#ifdef SEND_MSG_TCP_WIN_BUILD
   WSADATA wsda;
#endif

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;// AF_UNSPEC; IPv4 only for now
   hints.ai_socktype = SOCK_STREAM;
   //hints.ai_flags = AI_PASSIVE; // use my IP

#ifdef SEND_MSG_TCP_WIN_BUILD
#ifdef __MINGW32_VERSION
   snprintf( portStr, sizeof(portStr), "%d", port);
#else
   _itoa_s( port, portStr, sizeof(portStr), 10);
#endif

   WSAStartup(0x0101, &wsda);
#else
   snprintf( portStr, sizeof(portStr), "%d", port);
#endif

   returnErrorCode = getaddrinfo(hostName, portStr, &hints, &serverInfoList);
   if(returnErrorCode != 0)
   {
      printf("getaddrinfo Error Code: %s\n", (char*)gai_strerror(returnErrorCode));
      return -1;
   }

   // Loop through the list, until a connection is made.
   for(servInfo = serverInfoList; servInfo != NULL; servInfo = servInfo->ai_next)
   {
      SOCKET newConnectionFd = socket( servInfo->ai_family, 
                                       servInfo->ai_socktype,
                                       servInfo->ai_protocol );

      if(newConnectionFd >= 0)
      {
         if(connect(newConnectionFd, servInfo->ai_addr, (int)servInfo->ai_addrlen) >= 0)
         {
            // A connection was made, no need to continue looping through the list.
            sockfd = newConnectionFd;
            break;
         }
         else
         {
            closesocket(newConnectionFd);
         }
      }
   }

   // Check if a connection was actually made.
   if(servInfo == NULL)
   {
      printf("Client failed to connect to server.\n");
   }

   freeaddrinfo(serverInfoList); // Free the memory allocated in getaddrinfo.

   return (int)sockfd;
   
}


static inline int sendTCPPacket_send(SOCKET sockfd, const char* msg, unsigned int msgSize)
{
   return send(sockfd, msg, msgSize, 0);
}

static inline int sendTCPPacket_close(SOCKET sockfd)
{
   return closesocket(sockfd);
}

static inline int sendTCPPacket(const char* hostName, unsigned short port, const char* msg, unsigned int msgSize)
{
   int success = 0;
   SOCKET sockfd = sendTCPPacket_init(hostName, port);

   if((int)sockfd > -1)
   {
      success = sendTCPPacket_send(sockfd, msg, msgSize) != -1;

      success = (sendTCPPacket_close(sockfd) == 0) && success;
   }   
   
   return success ? 0 : -1; // Convert bool (1 = pass, 0 = fail) to (0 = pass, -1 = fail)
}



#endif


