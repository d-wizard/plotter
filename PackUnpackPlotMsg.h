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
#ifndef PackUnpackPlotMsg_h
#define PackUnpackPlotMsg_h

#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>

#define MAX_PLOT_VALUE_SIZE (8)

#define MSG_SIZE_PARAM_NUM_BYTES (4)

typedef enum
{
   E_RESET_PLOT,
   E_PLOT_1D,
   E_PLOT_2D,
   E_INVALID_PLOT_ACTION
}ePlotAction;

typedef enum
{
   E_CHAR,
   E_UCHAR,
   E_INT_16,
   E_UINT_16,
   E_INT_32,
   E_UINT_32,
   E_INT_64,
   E_UINT_64,
   E_FLOAT_32,
   E_FLOAT_64
}ePlotDataTypes;


class GetEntirePlotMsg
{
public:
    GetEntirePlotMsg():
        m_msgSize(0),
        m_numMsgBytesRead(0),
        m_bytesNeededForMsgSize(MSG_SIZE_PARAM_NUM_BYTES){}

    void ReadPlotPacket(char* packet, unsigned int packetSize, char** retValMsgPtr, unsigned int* retValMsgSize)
    {
        unsigned int numBytesInPacketRemaining = packetSize;
        *retValMsgPtr = NULL;
        *retValMsgSize = 0;
        if(m_bytesNeededForMsgSize > 0)
        {
            unsigned int bytesToCopyToMsgSize =
               std::min(numBytesInPacketRemaining, m_bytesNeededForMsgSize);
            memcpy( ((char*)&m_msgSize)+(MSG_SIZE_PARAM_NUM_BYTES - m_bytesNeededForMsgSize),
                    packet,
                    bytesToCopyToMsgSize);

            numBytesInPacketRemaining -= bytesToCopyToMsgSize;
            m_bytesNeededForMsgSize -= bytesToCopyToMsgSize;
            if(m_bytesNeededForMsgSize == 0)
            {
                m_msgSize -= MSG_SIZE_PARAM_NUM_BYTES;
                m_msg.resize(m_msgSize);
                m_numMsgBytesRead = 0;
            }
        }
        if(numBytesInPacketRemaining)
        {
            unsigned int bytesToCopy =
               std::min(numBytesInPacketRemaining, (m_msgSize-m_numMsgBytesRead));
            memcpy( &m_msg[m_numMsgBytesRead],
                    packet+(packetSize-numBytesInPacketRemaining),
                    bytesToCopy);

            m_numMsgBytesRead += bytesToCopy;
            if(m_numMsgBytesRead >= m_msgSize)
            {
                *retValMsgPtr = &m_msg[0];
                *retValMsgSize = m_msgSize;
                m_bytesNeededForMsgSize = MSG_SIZE_PARAM_NUM_BYTES;
            }
        }
    }

private:
    unsigned int m_msgSize;
    unsigned int m_bytesNeededForMsgSize;
    unsigned int m_numMsgBytesRead;
    std::vector<char> m_msg;
};



class UnpackPlotMsg
{
public:
   UnpackPlotMsg();
   ~UnpackPlotMsg();

   ePlotAction Unpack(const char *inBytes, unsigned int numBytes);
   void reset();


   std::string m_plotName;
   std::vector<double> m_xAxisValues;
   std::vector<double> m_yAxisValues;
private:

   typedef enum
   {
      E_READ_ACTION,
      E_READ_NAME,
      E_READ_NUM_SAMP,
      E_READ_X_AXIS_DATA_TYPE,
      E_READ_Y_AXIS_DATA_TYPE,
      E_READ_X_AXIS_VALUE,
      E_READ_Y_AXIS_VALUE
   }eMsgUnpackState;


   bool ReadOneByte(char inByte);


   eMsgUnpackState m_unpackState;

   ePlotAction m_curAction;
   unsigned int m_numSamplesInPlot;
   ePlotDataTypes m_xAxisDataType;
   ePlotDataTypes m_yAxisDataType;
   unsigned int m_sampleIndex;


   char*        m_curPtrToFill;
   unsigned int m_curValueNumBytesFilled;
   unsigned int m_bytesNeededForCurValue;

   char m_tempRead[MAX_PLOT_VALUE_SIZE];

   bool validPlotAction(ePlotAction in);
   bool validPlotDataTypes(ePlotDataTypes in);

   void setNextState(eMsgUnpackState state, void* ptrToFill, unsigned int numBytesToFill);

   double readSampleValue(ePlotDataTypes dataType);

};

void packResetPlotMsg(std::vector<char>& msg);
void pack1dPlotMsg(std::vector<char>& msg, std::string name, unsigned int numSamp, ePlotDataTypes yAxisType, void* yAxisSamples);
void pack2dPlotMsg(std::vector<char>& msg, std::string name, unsigned int numSamp, ePlotDataTypes xAxisType, ePlotDataTypes yAxisType, void* xAxisSamples, void* yAxisSamples);



#endif

