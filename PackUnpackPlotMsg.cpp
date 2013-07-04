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

UnpackPlotMsg::UnpackPlotMsg():
   m_unpackState(E_READ_ACTION),
   m_curAction(E_INVALID_PLOT_ACTION),
   m_plotName(""),
   m_numSamplesInPlot(0),
   m_xAxisDataType(E_FLOAT_64),
   m_yAxisDataType(E_FLOAT_64),
   m_curPtrToFill((char*)&m_curAction),
   m_curValueNumBytesFilled(0),
   m_bytesNeededForCurValue(sizeof(ePlotAction)),
   m_sampleIndex(0)
{
   reset();
}

UnpackPlotMsg::~UnpackPlotMsg()
{
}

void UnpackPlotMsg::reset()
{
   m_curAction = E_INVALID_PLOT_ACTION;
   m_plotName = "";
   m_numSamplesInPlot = 0;
   m_xAxisDataType = E_FLOAT_64;
   m_yAxisDataType = E_FLOAT_64;

   setNextState(E_READ_ACTION, &m_curAction, sizeof(m_curAction));

   m_sampleIndex = 0;
   m_xAxisValues.clear();
   m_yAxisValues.clear();
}

ePlotAction UnpackPlotMsg::Unpack(const char* inBytes, unsigned int numBytes)
{
   ePlotAction retVal = E_INVALID_PLOT_ACTION;
   for(unsigned int i = 0; i < numBytes; ++i)
   {
      if(ReadOneByte(inBytes[i]))
      {
         switch(m_unpackState)
         {
            case E_READ_ACTION:
               if(validPlotAction(m_curAction))
               {
                  switch(m_curAction)
                  {
                     case E_RESET_PLOT:
                        retVal = m_curAction;
                        setNextState(E_READ_ACTION, &m_curAction, sizeof(m_curAction));
                     break;
                     case E_PLOT_1D:
                     case E_PLOT_2D:
                        setNextState(E_READ_NAME, m_tempRead, 1);
                     break;
                  }
               }
               else
               {
                  reset();
               }
            break;
            case E_READ_NAME:
               if(m_curPtrToFill[0] == 0)
               {
                  setNextState(E_READ_NUM_SAMP, &m_numSamplesInPlot, sizeof(m_numSamplesInPlot));
               }
               else
               {
                  m_curPtrToFill[1] = 0;
                  m_plotName.append(m_curPtrToFill);
               }
            break;
            case E_READ_NUM_SAMP:
               m_sampleIndex = 0;
               switch(m_curAction)
               {
                  case E_PLOT_1D:
                     m_yAxisValues.resize(m_numSamplesInPlot);
                     setNextState(E_READ_Y_AXIS_DATA_TYPE, &m_yAxisDataType, sizeof(m_yAxisDataType));
                  break;
                  case E_PLOT_2D:
                     m_xAxisValues.resize(m_numSamplesInPlot);
                     m_yAxisValues.resize(m_numSamplesInPlot);
                     setNextState(E_READ_X_AXIS_DATA_TYPE, &m_xAxisDataType, sizeof(m_xAxisDataType));
                  break;
               }
            break;
            case E_READ_X_AXIS_DATA_TYPE:
               if(validPlotDataTypes(m_xAxisDataType))
               {
                  switch(m_curAction)
                  {
                     case E_PLOT_2D:
                        setNextState(E_READ_Y_AXIS_DATA_TYPE, &m_yAxisDataType, sizeof(m_yAxisDataType));
                     break;
                  }
               }
               else
               {
                  reset();
               }
            break;
            case E_READ_Y_AXIS_DATA_TYPE:
               if(validPlotDataTypes(m_yAxisDataType))
               {
                  switch(m_curAction)
                  {
                     case E_PLOT_1D:
                        setNextState(E_READ_Y_AXIS_VALUE, m_tempRead, PLOT_DATA_TYPE_SIZES[m_yAxisDataType]);
                     break;
                     case E_PLOT_2D:
                        setNextState(E_READ_X_AXIS_VALUE, m_tempRead, PLOT_DATA_TYPE_SIZES[m_xAxisDataType]);
                     break;
                  }
               }
               else
               {
                  reset();
               }
            break;
            case E_READ_X_AXIS_VALUE:
               m_xAxisValues[m_sampleIndex] = readSampleValue(m_xAxisDataType);
               switch(m_curAction)
               {
                  case E_PLOT_2D:
                     setNextState(E_READ_Y_AXIS_VALUE, m_tempRead, PLOT_DATA_TYPE_SIZES[m_yAxisDataType]);
                  break;
               }
            break;
            case E_READ_Y_AXIS_VALUE:
               m_yAxisValues[m_sampleIndex] = readSampleValue(m_yAxisDataType);
               if(++m_sampleIndex >= m_numSamplesInPlot)
               {
                   retVal = m_curAction;
                  setNextState(E_READ_ACTION, &m_curAction, sizeof(m_curAction));
               }
               else
               {
                  switch(m_curAction)
                  {
                     case E_PLOT_1D:
                        setNextState(E_READ_Y_AXIS_VALUE, m_tempRead, PLOT_DATA_TYPE_SIZES[m_yAxisDataType]);
                     break;
                     case E_PLOT_2D:
                        setNextState(E_READ_X_AXIS_VALUE, m_tempRead, PLOT_DATA_TYPE_SIZES[m_xAxisDataType]);
                     break;
                  }
               }
            break;
         }
      }
   }

   return retVal;
}

void UnpackPlotMsg::setNextState(eMsgUnpackState state, void* ptrToFill, unsigned int numBytesToFill)
{
   m_unpackState = state;
   m_curPtrToFill = (char*)ptrToFill;
   m_curValueNumBytesFilled = 0;
   m_bytesNeededForCurValue = numBytesToFill;
}

bool UnpackPlotMsg::ReadOneByte(char inByte)
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


bool UnpackPlotMsg::validPlotAction(ePlotAction in)
{
   bool valid = false;
   switch(in)
   {
   case E_RESET_PLOT:
   case E_PLOT_1D:
   case E_PLOT_2D:
      valid = true;
      break;
   }
   return valid;
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
         SCHAR samp;
         memcpy(&samp, m_curPtrToFill, sizeof(SCHAR));
         retVal = (double)samp;
      }
      break;
      case E_UCHAR:
      {
         UCHAR samp;
         memcpy(&samp, m_curPtrToFill, sizeof(UCHAR));
         retVal = (double)samp;
      }
      break;
      case E_INT_16:
      {
         INT_16 samp;
         memcpy(&samp, m_curPtrToFill, sizeof(INT_16));
         retVal = (double)samp;
      }
      break;
      case E_UINT_16:
      {
         UINT_16 samp;
         memcpy(&samp, m_curPtrToFill, sizeof(UINT_16));
         retVal = (double)samp;
      }
      break;
      case E_INT_32:
      {
         INT_32 samp;
         memcpy(&samp, m_curPtrToFill, sizeof(INT_32));
         retVal = (double)samp;
      }
      break;
      case E_UINT_32:
      {
         UINT_32 samp;
         memcpy(&samp, m_curPtrToFill, sizeof(UINT_32));
         retVal = (double)samp;
      }
      break;
      case E_INT_64:
      {
         INT_64 samp;
         memcpy(&samp, m_curPtrToFill, sizeof(INT_64));
         retVal = (double)samp;
      }
      break;
      case E_UINT_64:
      {
         UINT_64 samp;
         memcpy(&samp, m_curPtrToFill, sizeof(UINT_64));
         retVal = (double)samp;
      }
      break;
      case E_FLOAT_32:
      {
         FLOAT_32 samp;
         memcpy(&samp, m_curPtrToFill, sizeof(FLOAT_32));
         retVal = (double)samp;
      }
      break;
      case E_FLOAT_64:
      {
         FLOAT_64 samp;
         memcpy(&samp, m_curPtrToFill, sizeof(FLOAT_64));
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

void packResetPlotMsg(std::vector<char>& msg)
{
   unsigned int totalMsgSize = MSG_SIZE_PARAM_NUM_BYTES + sizeof(ePlotAction);

   msg.resize(totalMsgSize);
   ePlotAction temp = E_RESET_PLOT;

   unsigned int curCopyIndex = 0;
   copyAndIncIndex(&msg[0], &curCopyIndex, &totalMsgSize, MSG_SIZE_PARAM_NUM_BYTES);
   copyAndIncIndex(&msg[0], &curCopyIndex, &temp, sizeof(temp));
}
void pack1dPlotMsg(std::vector<char>& msg, std::string name, unsigned int numSamp, ePlotDataTypes yAxisType, void* yAxisSamples)
{
   unsigned int totalMsgSize = MSG_SIZE_PARAM_NUM_BYTES +
      sizeof(ePlotAction) + name.size() + 1 + sizeof(numSamp) + 
      sizeof(yAxisType) + (numSamp*PLOT_DATA_TYPE_SIZES[yAxisType]);

   msg.resize(totalMsgSize);
   ePlotAction temp = E_PLOT_1D;

   unsigned int curCopyIndex = 0;
   copyAndIncIndex(&msg[0], &curCopyIndex, &totalMsgSize, MSG_SIZE_PARAM_NUM_BYTES);
   copyAndIncIndex(&msg[0], &curCopyIndex, &temp, sizeof(temp));
   copyAndIncIndex(&msg[0], &curCopyIndex, name.c_str(), name.size()+1);
   copyAndIncIndex(&msg[0], &curCopyIndex, &numSamp, sizeof(numSamp));
   copyAndIncIndex(&msg[0], &curCopyIndex, &yAxisType, sizeof(yAxisType));
   copyAndIncIndex(&msg[0], &curCopyIndex, yAxisSamples, (numSamp*PLOT_DATA_TYPE_SIZES[yAxisType]));

}
void pack2dPlotMsg(std::vector<char>& msg, std::string name, unsigned int numSamp, ePlotDataTypes xAxisType, ePlotDataTypes yAxisType, void* xAxisSamples, void* yAxisSamples)
{
   unsigned int totalMsgSize = MSG_SIZE_PARAM_NUM_BYTES +
      sizeof(ePlotAction) + name.size() + 1 + sizeof(numSamp) + 
      sizeof(xAxisType) + (numSamp*PLOT_DATA_TYPE_SIZES[xAxisType]) +
      sizeof(yAxisType) + (numSamp*PLOT_DATA_TYPE_SIZES[yAxisType]);

   msg.resize(totalMsgSize);
   ePlotAction temp = E_PLOT_2D;

   unsigned int curCopyIndex = 0;
   copyAndIncIndex(&msg[0], &curCopyIndex, &totalMsgSize, MSG_SIZE_PARAM_NUM_BYTES);
   copyAndIncIndex(&msg[0], &curCopyIndex, &temp, sizeof(temp));
   copyAndIncIndex(&msg[0], &curCopyIndex, name.c_str(), name.size()+1);
   copyAndIncIndex(&msg[0], &curCopyIndex, &numSamp, sizeof(numSamp));
   copyAndIncIndex(&msg[0], &curCopyIndex, &xAxisType, sizeof(xAxisType));
   copyAndIncIndex(&msg[0], &curCopyIndex, &yAxisType, sizeof(yAxisType));

   unsigned int xAxisSampleIndex = 0;
   unsigned int yAxisSampleIndex = 0;
   for(unsigned int i = 0; i < numSamp; ++i)
   {
      copyAndIncIndex(&msg[0], &curCopyIndex, &((char*)xAxisSamples)[xAxisSampleIndex], PLOT_DATA_TYPE_SIZES[xAxisType]);
      copyAndIncIndex(&msg[0], &curCopyIndex, &((char*)yAxisSamples)[yAxisSampleIndex], PLOT_DATA_TYPE_SIZES[yAxisType]);

      xAxisSampleIndex += PLOT_DATA_TYPE_SIZES[xAxisType];
      yAxisSampleIndex += PLOT_DATA_TYPE_SIZES[yAxisType];
   }
}





