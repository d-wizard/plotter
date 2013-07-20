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

   dServerSocket_init(&m_servSock,
                      port,
                      RxPacketCallback,
                      ClientStartCallback,
                      ClientEndCallback,
                      this);

   dServerSocket_bind(&m_servSock);

   dServerSocket_accept(&m_servSock);
}

TCPMsgReader::~TCPMsgReader()
{
   dServerSocket_killAll(&m_servSock);

   std::map<struct sockaddr_storage*, GetEntirePlotMsg*>::iterator iter;
   for(iter = m_msgReaderMap.begin(); iter != m_msgReaderMap.end(); ++iter)
   {
       delete iter->second;
   }
}

void TCPMsgReader::ClientStartCallback(void* inPtr, struct sockaddr_storage* client)
{
   TCPMsgReader* _this = (TCPMsgReader*)inPtr;
   if(_this->m_msgReaderMap.find(client) == _this->m_msgReaderMap.end())
   {
      _this->m_msgReaderMap[client] = new GetEntirePlotMsg();
   }
}

void TCPMsgReader::ClientEndCallback(void* inPtr, struct sockaddr_storage* client)
{
   TCPMsgReader* _this = (TCPMsgReader*)inPtr;
   if(_this->m_msgReaderMap.find(client) != _this->m_msgReaderMap.end())
   {
      delete _this->m_msgReaderMap.find(client)->second;
      _this->m_msgReaderMap.erase(_this->m_msgReaderMap.find(client));
   }
}

void TCPMsgReader::RxPacketCallback(void* inPtr, struct sockaddr_storage* client, char* packet, unsigned int size)
{
    TCPMsgReader* _this = (TCPMsgReader*)inPtr;
    char* plotMsg = NULL;
    unsigned int plotMsgSize = 0;

    _this->m_msgReaderMap[client]->ProcessPlotPacket(packet, size);
    while(_this->m_msgReaderMap[client]->ReadPlotPackets(&plotMsg, &plotMsgSize))
    {
        _this->m_parent->readPlotMsg(plotMsg, plotMsgSize);
        _this->m_msgReaderMap[client]->finishedReadMsg();
    }

}
