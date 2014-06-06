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
#include <string.h>
#include "DataTypes.h"
#include "PackUnpackPlotMsg.h"

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
               if(validPlotAction(m_curAction))
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
               if(m_curMsgSize > (sizeof(m_curAction) + sizeof(m_curMsgSize)))
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
   unpack(&m_plotAction, sizeof(m_plotAction));
   unpack(&m_msgSize, sizeof(m_msgSize));
   if(m_msgSize == size)
   {
      switch(m_plotAction)
      {
         case E_CREATE_1D_PLOT:
         case E_UPDATE_1D_PLOT:
            unpackStr(&m_plotName);
            unpackStr(&m_curveName);
            unpack(&m_numSamplesInPlot, sizeof(m_numSamplesInPlot));
            if(m_plotAction == E_UPDATE_1D_PLOT)
            {
               // Update action has an extra parameter to define where the samples should go
               unpack(&m_sampleStartIndex, sizeof(m_sampleStartIndex));
            }
            unpack(&m_yAxisDataType, sizeof(m_yAxisDataType));
            unpack(&m_yShiftFactor, sizeof(m_yShiftFactor));
            if(validPlotDataTypes(m_yAxisDataType))
            {
               m_yAxisValues.resize(m_numSamplesInPlot);
               if(m_yShiftFactor == 0)
               {
                  for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                  {
                     m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
                  }
               }
               else if(m_yShiftFactor > 0)
               {
                  double divideFactor = (double)(1 << m_yShiftFactor);
                  for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                  {
                     m_yAxisValues[i] = readSampleValue(m_yAxisDataType) / divideFactor;
                  }
               }
               else
               {
                  double multiplyFactor = (double)(1 << (-m_yShiftFactor));
                  for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                  {
                     m_yAxisValues[i] = readSampleValue(m_yAxisDataType) * multiplyFactor;
                  }
               }
            }
         break;
         case E_CREATE_2D_PLOT:
         case E_UPDATE_2D_PLOT:
            unpackStr(&m_plotName);
            unpackStr(&m_curveName);
            unpack(&m_numSamplesInPlot, sizeof(m_numSamplesInPlot));
            if(m_plotAction == E_UPDATE_2D_PLOT)
            {
               // Update action has an extra parameter to define where the samples should go
               unpack(&m_sampleStartIndex, sizeof(m_sampleStartIndex));
            }
            unpack(&m_xAxisDataType, sizeof(m_xAxisDataType));
            unpack(&m_xShiftFactor, sizeof(m_xShiftFactor));
            unpack(&m_yAxisDataType, sizeof(m_yAxisDataType));
            unpack(&m_yShiftFactor, sizeof(m_yShiftFactor));
            unpack(&m_interleaved, sizeof(m_interleaved));

            if(validPlotDataTypes(m_xAxisDataType) && validPlotDataTypes(m_yAxisDataType))
            {
               m_xAxisValues.resize(m_numSamplesInPlot);
               m_yAxisValues.resize(m_numSamplesInPlot);
               if(m_interleaved == false)
               {
                  // Not interleaved data, avoid if inside for loops to increase speed.
                  if(m_xShiftFactor == 0)
                  {
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_xAxisValues[i] = readSampleValue(m_xAxisDataType);
                     }
                  }
                  else if(m_xShiftFactor > 0)
                  {
                     double divideFactor = (double)(1 << m_xShiftFactor);
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_xAxisValues[i] = readSampleValue(m_xAxisDataType) / divideFactor;
                     }
                  }
                  else
                  {
                     double multiplyFactor = (double)(1 << (-m_xShiftFactor));
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_xAxisValues[i] = readSampleValue(m_xAxisDataType) * multiplyFactor;
                     }
                  }
                  
                  if(m_yShiftFactor == 0)
                  {
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
                     }
                  }
                  else if(m_yShiftFactor > 0)
                  {
                     double divideFactor = (double)(1 << m_yShiftFactor);
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_yAxisValues[i] = readSampleValue(m_yAxisDataType) / divideFactor;
                     }
                  }
                  else
                  {
                     double multiplyFactor = (double)(1 << (-m_yShiftFactor));
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_yAxisValues[i] = readSampleValue(m_yAxisDataType) * multiplyFactor;
                     }
                  }
               } // end if(m_interleaved == false)
               else
               {
                  // Interleaved data
                  double x_multFactor = 0.0;
                  double y_multFactor = 0.0;

                  if(m_xShiftFactor >= 0)
                  {
                     x_multFactor = 1.0 / (double)(1 << m_xShiftFactor);
                  }
                  else
                  {
                     x_multFactor = (double)(1 << (-m_xShiftFactor));
                  }
                  
                  if(m_yShiftFactor >= 0)
                  {
                     y_multFactor = 1.0 / (double)(1 << m_yShiftFactor);
                  }
                  else
                  {
                     y_multFactor = (double)(1 << (-m_yShiftFactor));
                  }
                  
                  // Try to avoid unnessecary calculations.
                  // If both are have no shift factor there are far fewer calculations.
                  if(m_xShiftFactor == 0 && m_yShiftFactor == 0)
                  {
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_xAxisValues[i] = readSampleValue(m_xAxisDataType);
                        m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
                     }
                  }
                  else if(m_xShiftFactor != 0 && m_yShiftFactor != 0) // both need shifting
                  {
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_xAxisValues[i] = readSampleValue(m_xAxisDataType) * x_multFactor;
                        m_yAxisValues[i] = readSampleValue(m_yAxisDataType) * y_multFactor;
                     }
                  }
                  else if(m_xShiftFactor != 0)
                  {
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_xAxisValues[i] = readSampleValue(m_xAxisDataType) * x_multFactor;
                        m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
                     }
                  }
                  else // y needs shifting
                  {
                     for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
                     {
                        m_xAxisValues[i] = readSampleValue(m_xAxisDataType);
                        m_yAxisValues[i] = readSampleValue(m_yAxisDataType) * y_multFactor;
                     }
                  }

               }
            }
         break;
         default:
            m_plotAction = E_INVALID_PLOT_ACTION;
         break;
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
}

void UnpackPlotMsg::unpackStr(std::string* dst)
{
   *dst = m_msg+m_msgReadIndex;
   m_msgReadIndex += (dst->size()+1);
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
      case E_INVALID_DATA_TYPE:
      break;
   }

   return retVal;
}




