/* Copyright 2013 - 2017, 2019, 2025 Dan Williams. All Rights Reserved.
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
#include <list>
#include <map>
#include <stdio.h>
#include <string.h>
#include <QElapsedTimer>

#include "DataTypes.h"
#include "plotMsgPack.h"
#include "PlotHelperTypes.h"

#define MSG_SIZE_PARAM_NUM_BYTES (4)

#define MAX_PLOT_MSG_HEADER_SIZE (256)
extern unsigned int g_maxTcpPlotMsgSize; // This value can be changed via ini file.

PlotMsgIdType getUniquePlotMsgId(); // Prototype of function provided in .cpp file

inline bool validPlotAction(ePlotAction in)
{
   bool valid = false;
   switch(in)
   {
   case E_CREATE_1D_PLOT:
   case E_CREATE_2D_PLOT:
   case E_UPDATE_1D_PLOT:
   case E_UPDATE_2D_PLOT:
   case E_OPEN_PLOT_FILE:
      valid = true;
      break;
   default:
      break;
   }
   return valid;
}

inline UINT_32 getPlotDataTypeSize(ePlotDataTypes in)
{
   UINT_32 size = 0;
   switch(in)
   {
   case E_CHAR:
      size = sizeof(SCHAR);
      break;
   case E_UCHAR:
      size = sizeof(UCHAR);
      break;
   case E_INT_16:
      size = sizeof(INT_16);
      break;
   case E_UINT_16:
      size = sizeof(UINT_16);
      break;
   case E_INT_32:
      size = sizeof(INT_32);
      break;
   case E_UINT_32:
      size = sizeof(UINT_32);
      break;
   case E_INT_64:
      size = sizeof(INT_64);
      break;
   case E_UINT_64:
      size = sizeof(UINT_64);
      break;
   case E_FLOAT_16:
      size = sizeof(UINT_16);
      break;
   case E_FLOAT_32:
      size = sizeof(FLOAT_32);
      break;
   case E_FLOAT_64:
      size = sizeof(FLOAT_64);
      break;
   case E_TIME_STRUCT_64:
      size = sizeof(UINT_32) + sizeof(UINT_32);
      break;
   default:
   case E_TIME_STRUCT_128:
      size = sizeof(UINT_64) + sizeof(UINT_64);
      break;
   case E_INVALID_DATA_TYPE:
       size = 0;
       break;
   }
   return size;
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
   case E_FLOAT_16:
   case E_FLOAT_32:
   case E_FLOAT_64:
   case E_TIME_STRUCT_64:
   case E_TIME_STRUCT_128:
      valid = true;
      break;
   case E_INVALID_DATA_TYPE:
       valid = false;
       break;
   }
   return valid;
}


#define NUM_PACKET_SAVE (5)
class GetEntirePlotMsg
{
   // After 1.5 seconds without a packet, assume any incomplete messages are lost and reinitialize for a new message.
   static const qint64 MS_BETWEEN_PACKETS_FOR_REINIT = 1500;

public:
   GetEntirePlotMsg(struct sockaddr_storage* client);
   ~GetEntirePlotMsg();

   // If reading an action and no bytes have been filled in, then not active.
   bool isActiveReceive(){ return (m_unpackState != E_READ_ACTION || m_curValueNumBytesFilled != 0); }
   void ProcessPlotPacket(const char *inBytes, unsigned int numBytes);
   bool ReadPlotPackets(tIncomingMsg* inMsg);
   void finishedReadMsg();

private:

   GetEntirePlotMsg();
   GetEntirePlotMsg(GetEntirePlotMsg const&);
   void operator=(GetEntirePlotMsg const&);
   
   typedef enum
   {
      E_READ_ACTION,
      E_READ_SIZE,
      E_READ_REST_OF_MSG
   }eMsgUnpackState;

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

   QElapsedTimer m_timeBetweenPackets;

   tPlotterIpAddr m_ipAddr;
};

class UnpackPlotMsg
{
public:
   UnpackPlotMsg();
   UnpackPlotMsg(tIncomingMsg* inMsg);
   ~UnpackPlotMsg();

   PlotMsgIdType m_plotMsgID;

   ePlotAction m_plotAction;
   std::string m_plotName;
   std::string m_curveName;
   UINT_32 m_sampleStartIndex;
   ePlotType m_plotType;
   ePlotDataTypes m_xAxisDataType;
   ePlotDataTypes m_yAxisDataType;
   tPlotterIpAddr m_ipAddr;
   std::vector<double> m_xAxisValues;
   std::vector<double> m_yAxisValues;

   std::list<std::string> m_restorePlotFromFileList;

   bool m_useCurveMathProps;
   tCurveMathProperties m_curveMathProps;

   const char* GetMsgPtr(){return m_msg;}
   UINT_32 GetMsgPtrSize(){return m_msgSize;}

private:
   
   void unpack(void* dst, unsigned int size);
   void unpackStr(std::string* dst);

   double readSampleValue(ePlotDataTypes dataType);
   
   
   const char* m_msg;
   UINT_32 m_msgSize;
   UINT_32 m_msgReadIndex;

   UINT_32 m_numSamplesInPlot;

   UCHAR m_interleaved;
};

typedef std::list<UnpackPlotMsg*> UnpackPlotMsgPtrList;

class plotMsgGroup
{
public:
   plotMsgGroup():
      m_groupMsgId(getUniquePlotMsgId()),
      m_changeCausedByUserGuiInput(false)
   {
   }

   // Pass in pointer to dynamically allocated UnpackPlotMsg type. delete will be called in destructor.
   plotMsgGroup(UnpackPlotMsg* unpackPlotMsg):
      m_groupMsgId(PLOT_MSG_ID_TYPE_NO_PARENT_MSG),
      m_changeCausedByUserGuiInput(true)
   {
      m_plotMsgs.push_back(unpackPlotMsg);
   }

   ~plotMsgGroup()
   {
      for(UnpackPlotMsgPtrList::iterator iter = m_plotMsgs.begin(); iter != m_plotMsgs.end(); ++iter)
      {
         delete (*iter);
      }
   }

   UnpackPlotMsgPtrList m_plotMsgs;

   PlotMsgIdType m_groupMsgId;

   // Indicates whether this plot msg group was created from User GUI input (example: creation of a
   // new child curve) or this plot msg group contains the data from an external plot msg (example: a TCP
   // plot message or a child curve that has a parent that's data was modified from a TCP plot message).
   bool m_changeCausedByUserGuiInput;
};

class UnpackMultiPlotMsg
{
public:
   UnpackMultiPlotMsg();
   UnpackMultiPlotMsg(const char* msg, unsigned int size, std::string plotNameOverride);
   UnpackMultiPlotMsg(tIncomingMsg* inMsg);
   ~UnpackMultiPlotMsg();

   plotMsgGroup* getPlotMsgGroup(std::string plotName);

   std::map<std::string, plotMsgGroup*> m_plotMsgs;

   tPlotterIpAddr m_msgSourceIpAddr;

private:
   void init(tIncomingMsg* inMsg, std::string plotNameOverride = "");
};

#endif

