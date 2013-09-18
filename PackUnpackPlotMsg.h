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

#include "DataTypes.h"
#include "plotMsgPack.h"

#define MSG_SIZE_PARAM_NUM_BYTES (4)


inline bool validPlotAction(ePlotAction in)
{
   bool valid = false;
   switch(in)
   {
   case E_CREATE_1D_PLOT:
   case E_CREATE_2D_PLOT:
   case E_UPDATE_1D_PLOT:
   case E_UPDATE_2D_PLOT:
      valid = true;
      break;
   default:
      break;
   }
   return valid;
}

inline bool validPlotDataTypes(ePlotDataTypes in)
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


#define NUM_PACKET_SAVE (5)
class GetEntirePlotMsg
{
public:
   GetEntirePlotMsg();
   ~GetEntirePlotMsg();

   // If reading an action and no bytes have been filled in, then not active.
   bool isActiveReceive(){ return (m_unpackState != E_READ_ACTION || m_curValueNumBytesFilled != 0); }
   void ProcessPlotPacket(const char *inBytes, unsigned int numBytes);
   bool ReadPlotPackets(char** retValMsgPtr, unsigned int* retValMsgSize);
   void finishedReadMsg();

private:

   typedef enum
   {
      E_READ_ACTION,
      E_READ_SIZE,
      E_READ_REST_OF_MSG
   }eMsgUnpackState;

   bool ReadOneByte(char inByte);
   void setNextState(eMsgUnpackState state, void* ptrToFill, unsigned int numBytesToFill);
   void initNextWriteMsg();
   void reset();

   eMsgUnpackState m_unpackState;

   ePlotAction m_curAction;
   unsigned int m_curMsgSize;
   std::vector<char> m_msgs[NUM_PACKET_SAVE];
   unsigned int m_msgsWriteIndex;
   unsigned int m_msgsReadIndex;

   char*        m_curPtrToFill;
   unsigned int m_curValueNumBytesFilled;
   unsigned int m_bytesNeededForCurValue;

};

class UnpackPlotMsg
{
public:
   UnpackPlotMsg(const char* msg, unsigned int size);
   ~UnpackPlotMsg();

   ePlotAction m_plotAction;
   std::string m_plotName;
   std::string m_curveName;
   UINT_32 m_sampleStartIndex;
   std::vector<double> m_xAxisValues;
   std::vector<double> m_yAxisValues;
private:
   UnpackPlotMsg();
   
   void unpack(void* dst, unsigned int size);
   void unpackStr(std::string* dst);

   double readSampleValue(ePlotDataTypes dataType);
   
   
   const char* m_msg;
   UINT_32 m_msgSize;
   UINT_32 m_msgReadIndex;

   UINT_32 m_numSamplesInPlot;
   ePlotDataTypes m_xAxisDataType;
   ePlotDataTypes m_yAxisDataType;

   INT_32 m_xShiftFactor;
   INT_32 m_yShiftFactor;
};


#endif

