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
#include "TCPMsgReader.h"
#include "plotGuiMain.h"

TCPMsgReader::TCPMsgReader(plotGuiMain* parent, int port):
    m_parent(parent)
{
   memset(&m_servSock, 0, sizeof(m_servSock));

   dServerSocket_init(&m_servSock, port, RxPacketCallback, this);
   dServerSocket_bind(&m_servSock);

   dServerSocket_accept(&m_servSock);
}

TCPMsgReader::~TCPMsgReader()
{
   dServerSocket_killAll(&m_servSock);
}

char* packetAddr = NULL;

// probably need to make a map of clients, in case multiple clients are talking at the same time
// do not want packets to be inter mixed.
void TCPMsgReader::RxPacketCallback(void* inPtr, struct sockaddr_storage* client, char* packet, unsigned int size)
{
    TCPMsgReader* _this = (TCPMsgReader*)inPtr;
    char* plotMsg = NULL;
    unsigned int plotMsgSize = 0;
    _this->m_plotMsgGetter.ReadPlotPacket(packet,size, &plotMsg, &plotMsgSize);
    if(plotMsg != NULL)
    {
        _this->m_parent->readPlotMsg(plotMsg, plotMsgSize);
    }
}
