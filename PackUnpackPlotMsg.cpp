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
#include <string.h>
#include "DataTypes.h"
#include "PackUnpackPlotMsg.h"

static int PLOT_DATA_TYPE_SIZES[]=
{
   sizeof(SCHAR),
   sizeof(UCHAR),
   sizeof(INT_16),
   sizeof(UINT_16),
   sizeof(INT_32),
   sizeof(UINT_32),
   sizeof(INT_64),
   sizeof(UINT_64),
   sizeof(FLOAT_32),
   sizeof(FLOAT_64)
};

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

void GetEntirePlotMsg::ReadPlotPacket(const char* inBytes, unsigned int numBytes, char **retValMsgPtr, unsigned int *retValMsgSize)
{
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
               m_msgs[m_msgsWriteIndex].resize(m_curMsgSize);
               
               // Got to add the beginning of the message to the message buffer.
               unsigned int bytesAlreadyRead = 0;
               memcpy(&m_msgs[m_msgsWriteIndex][bytesAlreadyRead], &m_curAction, sizeof(m_curAction));
               bytesAlreadyRead += sizeof(m_curAction);
               memcpy(&m_msgs[m_msgsWriteIndex][bytesAlreadyRead], &m_curMsgSize, sizeof(m_curMsgSize));
               bytesAlreadyRead += sizeof(m_curMsgSize);
               setNextState(E_READ_REST_OF_MSG, &m_msgs[m_msgsWriteIndex][bytesAlreadyRead], m_curMsgSize - bytesAlreadyRead);
            }
            break;
            case E_READ_REST_OF_MSG:
               *retValMsgPtr = &m_msgs[m_msgsWriteIndex][0];
               *retValMsgSize = m_curMsgSize;
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


bool GetEntirePlotMsg::validPlotAction(ePlotAction in)
{
   bool valid = false;
   switch(in)
   {
   case E_PLOT_1D:
   case E_PLOT_2D:
      valid = true;
      break;
   default:
      break;
   }
   return valid;
}






UnpackPlotMsg::UnpackPlotMsg(const char *msg, unsigned int size):
   m_plotAction(E_INVALID_PLOT_ACTION),
   m_plotName(""),
   m_curveName(""),
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
         case E_PLOT_1D:
            unpackStr(&m_plotName);
            unpackStr(&m_curveName);
            unpack(&m_numSamplesInPlot, sizeof(m_numSamplesInPlot));
            unpack(&m_yAxisDataType, sizeof(m_yAxisDataType));
            if(validPlotDataTypes(m_yAxisDataType))
            {
               m_yAxisValues.resize(m_numSamplesInPlot);
               for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
               {
                  m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
               }
            }
         break;
         case E_PLOT_2D:
            unpackStr(&m_plotName);
            unpackStr(&m_curveName);
            unpack(&m_numSamplesInPlot, sizeof(m_numSamplesInPlot));
            unpack(&m_xAxisDataType, sizeof(m_xAxisDataType));
            unpack(&m_yAxisDataType, sizeof(m_yAxisDataType));
            if(validPlotDataTypes(m_xAxisDataType) && validPlotDataTypes(m_yAxisDataType))
            {
               m_xAxisValues.resize(m_numSamplesInPlot);
               m_yAxisValues.resize(m_numSamplesInPlot);
               for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
               {
                  m_xAxisValues[i] = readSampleValue(m_xAxisDataType);
               }
               for(unsigned int i = 0; i < m_numSamplesInPlot; ++i)
               {
                  m_yAxisValues[i] = readSampleValue(m_yAxisDataType);
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



bool UnpackPlotMsg::validPlotDataTypes(ePlotDataTypes in)
{
   bool valid = false;
   switch(in)
   {
   case E_CHAR:
   case E_UCHAR:
   case E_INT_16:
   case E_UINT_16:
   case E_INT_32:
   case E_UINT_32:
   case E_INT_64:
   case E_UINT_64:
   case E_FLOAT_32:
   case E_FLOAT_64:
      valid = true;
      break;
   }
   return valid;
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
   }

   return retVal;
}


//////////////////    PACK Functions ////////////////////

inline void copyAndIncIndex(char* baseWritePtr, unsigned int* index, const void* srcPtr, unsigned int copySize)
{
   memcpy(&baseWritePtr[*index], srcPtr, copySize);
   (*index) += copySize;
}

void pack1dPlotMsg(std::vector<char>& msg, std::string plotName, std::string curveName, unsigned int numSamp, ePlotDataTypes yAxisType, void* yAxisSamples)
{
   unsigned int totalMsgSize = sizeof(ePlotAction) + MSG_SIZE_PARAM_NUM_BYTES +
      plotName.size() + 1 + curveName.size() + 1 + sizeof(numSamp) + 
      sizeof(yAxisType) + (numSamp*PLOT_DATA_TYPE_SIZES[yAxisType]);

   msg.resize(totalMsgSize);
   ePlotAction temp = E_PLOT_1D;

   unsigned int curCopyIndex = 0;
   copyAndIncIndex(&msg[0], &curCopyIndex, &temp, sizeof(temp));
   copyAndIncIndex(&msg[0], &curCopyIndex, &totalMsgSize, MSG_SIZE_PARAM_NUM_BYTES);
   copyAndIncIndex(&msg[0], &curCopyIndex, plotName.c_str(), plotName.size()+1);
   copyAndIncIndex(&msg[0], &curCopyIndex, curveName.c_str(), curveName.size()+1);
   copyAndIncIndex(&msg[0], &curCopyIndex, &numSamp, sizeof(numSamp));
   copyAndIncIndex(&msg[0], &curCopyIndex, &yAxisType, sizeof(yAxisType));
   copyAndIncIndex(&msg[0], &curCopyIndex, yAxisSamples, (numSamp*PLOT_DATA_TYPE_SIZES[yAxisType]));

}
void pack2dPlotMsg(std::vector<char>& msg, std::string plotName, std::string curveName, unsigned int numSamp, ePlotDataTypes xAxisType, ePlotDataTypes yAxisType, void* xAxisSamples, void* yAxisSamples)
{
   unsigned int totalMsgSize = sizeof(ePlotAction) + MSG_SIZE_PARAM_NUM_BYTES +
      plotName.size() + 1 + curveName.size() + 1 + sizeof(numSamp) + 
      sizeof(xAxisType) + (numSamp*PLOT_DATA_TYPE_SIZES[xAxisType]) +
      sizeof(yAxisType) + (numSamp*PLOT_DATA_TYPE_SIZES[yAxisType]);

   msg.resize(totalMsgSize);
   ePlotAction temp = E_PLOT_2D;

   unsigned int curCopyIndex = 0;
   copyAndIncIndex(&msg[0], &curCopyIndex, &temp, sizeof(temp));
   copyAndIncIndex(&msg[0], &curCopyIndex, &totalMsgSize, MSG_SIZE_PARAM_NUM_BYTES);
   copyAndIncIndex(&msg[0], &curCopyIndex, plotName.c_str(), plotName.size()+1);
   copyAndIncIndex(&msg[0], &curCopyIndex, curveName.c_str(), curveName.size()+1);
   copyAndIncIndex(&msg[0], &curCopyIndex, &numSamp, sizeof(numSamp));
   copyAndIncIndex(&msg[0], &curCopyIndex, &xAxisType, sizeof(xAxisType));
   copyAndIncIndex(&msg[0], &curCopyIndex, &yAxisType, sizeof(yAxisType));
   copyAndIncIndex(&msg[0], &curCopyIndex, xAxisSamples, (numSamp*PLOT_DATA_TYPE_SIZES[xAxisType]));
   copyAndIncIndex(&msg[0], &curCopyIndex, yAxisSamples, (numSamp*PLOT_DATA_TYPE_SIZES[yAxisType]));
}





