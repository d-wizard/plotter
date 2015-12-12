/* Copyright 2013 - 2015 Dan Williams. All Rights Reserved.
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
#include <string.h>
#include "DataTypes.h"
#include "PackUnpackPlotMsg.h"

// Global, can be changed outside of this file.
unsigned int g_maxTcpPlotMsgSize = (40*1024*1024) + MAX_PLOT_MSG_HEADER_SIZE; // 40 MB+

GetEntirePlotMsg::GetEntirePlotMsg():
   m_unpackState(E_READ_ACTION),
   m_curAction(E_INVALID_PLOT_ACTION),
   m_curMsgSize(0),
   m_msgsWriteIndex(0),
   m_msgsReadIndex(0),
   m_curValueNumBytesFilled(0),
   m_bytesNeededForCurValue(sizeof(ePlotAction))
{
   initNextWriteMsg();
   m_msgsReadIndex = m_msgsWriteIndex;
   reset();

   m_timeBetweenPackets.restart();
}

GetEntirePlotMsg::~GetEntirePlotMsg()
{
}

void GetEntirePlotMsg::reset()
{
   m_curAction = E_INVALID_PLOT_ACTION;

   setNextState(E_READ_ACTION, &m_curAction, sizeof(m_curAction));

   m_curMsgSize = 0;
}

void GetEntirePlotMsg::initNextWriteMsg()
{
   if(++m_msgsWriteIndex == NUM_PACKET_SAVE)
   {
      m_msgsWriteIndex = 0;
   }
   m_msgs[m_msgsWriteIndex].clear();
}

bool GetEntirePlotMsg::ReadPlotPackets(char** retValMsgPtr, unsigned int* retValMsgSize)
{
   bool validMsg = (m_msgsReadIndex != m_msgsWriteIndex);
   if(validMsg)
   {
       *retValMsgPtr = &m_msgs[m_msgsReadIndex][0];
       *retValMsgSize = m_msgs[m_msgsReadIndex].size();
   }
   else
   {
       retValMsgPtr = NULL;
       retValMsgSize = 0;
   }
   return validMsg;
}

void GetEntirePlotMsg::finishedReadMsg()
{
   if(m_msgsReadIndex != m_msgsWriteIndex)
   {
      m_msgs[m_msgsReadIndex].clear();
      if(++m_msgsReadIndex == NUM_PACKET_SAVE)
      {
         m_msgsReadIndex = 0;
      }
   }
}

// Take in raw packet from TCP Server. Read packet header and combine message that
// is split into multiple TCP packets.
void GetEntirePlotMsg::ProcessPlotPacket(const char* inBytes, unsigned int numBytes)
{
   // If too much time has passed since the last packet, assume the current message is
   // lost and reset for a new message.
   if(m_timeBetweenPackets.elapsed() >= MS_BETWEEN_PACKETS_FOR_REINIT)
   {
      reset();
   }
   m_timeBetweenPackets.restart();

   for(unsigned int i = 0; i < numBytes; ++i)
   {
      if(ReadOneByte(inBytes[i]))
      {
         switch(m_unpackState)
         {
            case E_READ_ACTION:
               if(validPlotAction(m_curAction) || m_curAction == E_MULTIPLE_PLOTS)
               {
                   setNextState(E_READ_SIZE, &m_curMsgSize, sizeof(m_curMsgSize));
               }
               else
               {
                  reset();
               }
            break;
            case E_READ_SIZE:
            {
               if( (m_curMsgSize > (sizeof(m_curAction) + sizeof(m_curMsgSize))) &&
                   (m_curMsgSize <= g_maxTcpPlotMsgSize) )
               {
                  m_msgs[m_msgsWriteIndex].resize(m_curMsgSize);
                  
                  // Got to add the beginning of the message to the message buffer.
                  unsigned int bytesAlreadyRead = 0;
                  memcpy(&m_msgs[m_msgsWriteIndex][bytesAlreadyRead], &m_curAction, sizeof(m_curAction));
                  bytesAlreadyRead += sizeof(m_curAction);
                  memcpy(&m_msgs[m_msgsWriteIndex][bytesAlreadyRead], &m_curMsgSize, sizeof(m_curMsgSize));
                  bytesAlreadyRead += sizeof(m_curMsgSize);
                  setNextState(E_READ_REST_OF_MSG, &m_msgs[m_msgsWriteIndex][bytesAlreadyRead], m_curMsgSize - bytesAlreadyRead);
               }
               else
               {
                  reset();
               }
            }
            break;
            case E_READ_REST_OF_MSG:
               reset();
               initNextWriteMsg();
            break;
         }
      }
   }
}

void GetEntirePlotMsg::setNextState(eMsgUnpackState state, void* ptrToFill, unsigned int numBytesToFill)
{
   m_unpackState = state;
   m_curPtrToFill = (char*)ptrToFill;
   m_curValueNumBytesFilled = 0;
   m_bytesNeededForCurValue = numBytesToFill;
}

bool GetEntirePlotMsg::ReadOneByte(char inByte)
{
   bool valueFilled = false;
   m_curPtrToFill[m_curValueNumBytesFilled] = inByte;
   if(++m_curValueNumBytesFilled >= m_bytesNeededForCurValue)
   {
      m_curValueNumBytesFilled = 0;
      valueFilled = true;
   }

   return valueFilled;
}




UnpackPlotMsg::UnpackPlotMsg(const char *msg, unsigned int size):
   m_plotAction(E_INVALID_PLOT_ACTION),
   m_plotName(""),
   m_curveName(""),
   m_sampleStartIndex(0),
   m_msg(msg),
   m_msgSize(size),
   m_msgReadIndex(0),
   m_numSamplesInPlot(0),
   m_xAxisDataType(E_FLOAT_64),
   m_yAxisDataType(E_FLOAT_64)
{
   if(msg == NULL || size == 0)
   {
      return;
   }

   unpack(&m_plotAction, sizeof(m_plotAction));
   unpack(&m_msgSize, sizeof(m_msgSize));
   if(m_msgSize == size)
   {
      try
      {
         switch(m_plotAction)
         {
            case E_CREATE_1D_PLOT:
            case E_UPDATE_1D_PLOT:
               m_plotType = E_PLOT_TYPE_1D;
               unpackStr(&m_plotName);
               unpackStr(&m_curveName);
               unpack(&m_numSamplesInPlot, sizeof(m_numSamplesInPlot));
               if(m_plotAction == E_UPDATE_1D_PLOT)
               {
                  // Update action has an extra parameter to define where the samples should go
                  unpack(&m_sampleStartIndex, sizeof(m_sampleStartIndex));
               }
               unpack(&m_yAxisDataType, sizeof(m_yAxisDataType));
               if(validPlotDataTypes(m_yAxisDataType))
               {
                  // Calculate size of the data portion of the message.
                  UINT_32 dataSize = getPlotDataTypeSize(m_yAxisDataType) * m_numSamplesInPlot;

                  // Make sure the remainder of the messages is exactly the size of the amount
                  // of data specified in the message.
                  if( (m_msgReadIndex + dataSize) == m_msgSize )
                  {
                     m_yAxisValues.resize(m_numSamplesInPlot);
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
                     }
                  }
               }
            break;
            case E_CREATE_2D_PLOT:
            case E_UPDATE_2D_PLOT:
               m_plotType = E_PLOT_TYPE_2D;
               unpackStr(&m_plotName);
               unpackStr(&m_curveName);
               unpack(&m_numSamplesInPlot, sizeof(m_numSamplesInPlot));
               if(m_plotAction == E_UPDATE_2D_PLOT)
               {
                  // Update action has an extra parameter to define where the samples should go
                  unpack(&m_sampleStartIndex, sizeof(m_sampleStartIndex));
               }
               unpack(&m_xAxisDataType, sizeof(m_xAxisDataType));
               unpack(&m_yAxisDataType, sizeof(m_yAxisDataType));
               unpack(&m_interleaved, sizeof(m_interleaved));

               if(validPlotDataTypes(m_xAxisDataType) && validPlotDataTypes(m_yAxisDataType))
               {
                  // Calculate size of the data portion of the message.
                  UINT_32 dataSize = getPlotDataTypeSize(m_xAxisDataType) * m_numSamplesInPlot +
                                     getPlotDataTypeSize(m_yAxisDataType) * m_numSamplesInPlot;

                  // Make sure the remainder of the messages is exactly the size of the amount
                  // of data specified in the message.
                  if( (m_msgReadIndex + dataSize) == m_msgSize )
                  {
                     m_xAxisValues.resize(m_numSamplesInPlot);
                     m_yAxisValues.resize(m_numSamplesInPlot);
                     if(m_interleaved == false)
                     {
                        for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                        {
                           m_xAxisValues[i] = readSampleValue(m_xAxisDataType);
                        }
                        for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                        {
                           m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
                        }

                     } // end if(m_interleaved == false)
                     else
                     {
                        // Interleaved data
                        for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                        {
                           m_xAxisValues[i] = readSampleValue(m_xAxisDataType);
                           m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
                        }
                     }
                  }
               }
            break;
            default:
               m_plotAction = E_INVALID_PLOT_ACTION;
            break;
         } // End switch(m_plotAction)
      }
      catch(int dontCare)
      {
         m_plotAction = E_INVALID_PLOT_ACTION; // Tried to unpack beyond the packet size, return Invalid Action enum.
      }
   }
   else
   {
       m_plotAction = E_INVALID_PLOT_ACTION;
   }
   
}

UnpackPlotMsg::~UnpackPlotMsg(){}
   
void UnpackPlotMsg::unpack(void* dst, unsigned int size)
{
   if( (m_msgReadIndex + size) <= m_msgSize)
   {
      memcpy(dst, m_msg+m_msgReadIndex, size);
      m_msgReadIndex += size;
   }
   else
   {
      throw 0; // Indicate to calling function that an error has occurred.
   }
}

void UnpackPlotMsg::unpackStr(std::string* dst)
{
   // Validate string... find null terminator
   bool validStr = false;
   for(unsigned int i = m_msgReadIndex; i < m_msgSize; ++i)
   {
      if(m_msg[i] == '\0')
      {
         validStr = true;
         break;
      }
   }

   if(validStr)
   {
      *dst = m_msg+m_msgReadIndex;
      m_msgReadIndex += (dst->size()+1);
   }
   else
   {
      throw 0; // Indicate to calling function that an error has occurred.
   }
}

double UnpackPlotMsg::readSampleValue(ePlotDataTypes dataType)
{
   double retVal = 0.0;
   switch(dataType)
   {
      case E_CHAR:
      {
         SCHAR samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_UCHAR:
      {
         UCHAR samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_INT_16:
      {
         INT_16 samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_UINT_16:
      {
         UINT_16 samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_INT_32:
      {
         INT_32 samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_UINT_32:
      {
         UINT_32 samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_INT_64:
      {
         INT_64 samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_UINT_64:
      {
         UINT_64 samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_FLOAT_32:
      {
         FLOAT_32 samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_FLOAT_64:
      {
         FLOAT_64 samp = 0;
         unpack(&samp, sizeof(samp));
         retVal = (double)samp;
      }
      break;
      case E_TIME_STRUCT_64:
      {
         UINT_32 samp[2]; // Index 0 is seconds, index 1 is nanoseconds.
         unpack(&samp, sizeof(samp));
         retVal = (double)samp[0] + ((double)samp[1] / (double)1000000000.0);
      }
      break;
      case E_TIME_STRUCT_128:
      {
         UINT_32 samp[4]; // Index 0 is seconds, index 2 is nanoseconds.
         unpack(&samp, sizeof(samp));
         retVal = (double)samp[0] + ((double)samp[2] / (double)1000000000.0);
      }
      break;
      case E_INVALID_DATA_TYPE:
      break;
   }

   return retVal;
}


UnpackMultiPlotMsg::UnpackMultiPlotMsg(const char* msg, unsigned int size):
   m_msgReadIndex(0)
{
   ePlotAction plotAction;
   UINT_32 msgSize;

   if(size > (sizeof(plotAction) + sizeof(msgSize)))
   {
      memcpy(&plotAction, msg + m_msgReadIndex, sizeof(plotAction));
      m_msgReadIndex += sizeof(plotAction);
      memcpy(&msgSize, msg + m_msgReadIndex, sizeof(msgSize));
      m_msgReadIndex += sizeof(msgSize);

      if(size == msgSize)
      {
         if(plotAction == E_MULTIPLE_PLOTS)
         {
            UINT_32 plotMsgIndex = 0;
            bool validMsg = true;
            while(validMsg == true && m_msgReadIndex < msgSize)
            {
               // UnpackPlotMsg expects the size passed in to match the value in the packed message.
               UINT_32 individualPlotMsgSize = 0;
               memcpy(&individualPlotMsgSize, msg + m_msgReadIndex + sizeof(ePlotAction), sizeof(individualPlotMsgSize));

               if(individualPlotMsgSize >= (msgSize - m_msgReadIndex))
               {
                  m_plotMsgs.push_back(new UnpackPlotMsg(msg + m_msgReadIndex, individualPlotMsgSize));
                  m_msgReadIndex += individualPlotMsgSize;
                  if(validPlotAction(m_plotMsgs[plotMsgIndex]->m_plotAction) == false || m_msgReadIndex > msgSize)
                  {
                     validMsg = false;
                  }
                  plotMsgIndex++;
               }
               else
               {
                  validMsg = false;
               }
            }
         }
         else
         {
            // Single plot in plot message.
            m_plotMsgs.push_back(new UnpackPlotMsg(msg, size));
         }
      } // end if(size == msgSize)

   }
}

UnpackMultiPlotMsg::~UnpackMultiPlotMsg()
{
}
