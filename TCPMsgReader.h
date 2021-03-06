/* Copyright 2013 - 2014 Dan Williams. All Rights Reserved.
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
#ifndef TCPMsgReader_h
#define TCPMsgReader_h

#include "TCPThreads.h"
#ifdef RX_FROM_UDP
#include "UDPThreads.h"
#endif
#include "DataTypes.h"
#include "PackUnpackPlotMsg.h"

#include <map>
#include <QSharedPointer>

class plotGuiMain;

class TCPMsgReader
{
public:
   TCPMsgReader(plotGuiMain* parent, int port);
   ~TCPMsgReader();

   plotGuiMain* m_parent;

   std::map<struct sockaddr_storage*, GetEntirePlotMsg*> m_msgReaderMap;
   
private:
   dServerSocket m_servSock;

#ifdef RX_FROM_UDP
   dUDPSocket m_udpSock;
#endif

   static void RxPacketCallback(void* inPtr, struct sockaddr_storage* client, char* packet, unsigned int size);
   static void ClientStartCallback(void* inPtr, struct sockaddr_storage* client);
   static void ClientEndCallback(void* inPtr, struct sockaddr_storage* client);

#ifdef RX_FROM_UDP
   static void RxPacketCallbackUDP(void* inPtr, struct sockaddr_storage* client, char* packet, unsigned int size);
#endif
   
   TCPMsgReader();
   
};


#endif

