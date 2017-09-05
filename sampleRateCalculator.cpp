/* Copyright 2017 Dan Williams. All Rights Reserved.
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
/*
 * sampleRateCalculator.cpp
 *
 *  Created on: Sep 2, 2017
 *      Author: d
 */
#include <QTime>
#include <QMutexLocker>
#include "sampleRateCalculator.h"

#define DEFAULT_MIN_TIME_TO_STORE (1.0)
#define DEFAULT_MIN_MSGS_TO_STORE (10)


sampleRateCalc::sampleRateCalc():
   m_sampleRate(0),
   m_averageTimeBetweenSampleMessages(1000000000), // Initialize to really big number of seconds
   m_minTimeToStore(DEFAULT_MIN_TIME_TO_STORE),
   m_minMsgsToStore(DEFAULT_MIN_MSGS_TO_STORE),
   m_mutex(QMutex::Recursive)
{

}

sampleRateCalc::~sampleRateCalc()
{

}

void sampleRateCalc::newSamples(long numSamp)
{
   // Get the current time.
   qint64 nowTimeMs = QDateTime::currentMSecsSinceEpoch();
   long numSec = nowTimeMs / 1000;
   long numMs  = nowTimeMs - ((qint64)numSec * 1000);
   double nowDouble = (double)numSec + (double)numMs / (double)1000;

   tTimeNumSampPair pair;
   pair.numSamp = numSamp;
   pair.time = nowDouble;

   QMutexLocker lock(&m_mutex);
   m_sampleMsgList.push_back(pair);
   determineSampleRate();
}

double sampleRateCalc::getTimeOfLastMsg()
{
   double retVal = -1;

   QMutexLocker lock(&m_mutex);
   if(m_sampleMsgList.size() > 0)
   {
      retVal = m_sampleMsgList[m_sampleMsgList.size() - 1].time;
   }

   return retVal;
}

sampleRateCalc::tTimeNumSampPair sampleRateCalc::getSampleMsgListAvg()
{
   tTimeNumSampPair retVal;

   QMutexLocker lock(&m_mutex);
   int listSize = m_sampleMsgList.size();
   if(listSize > 1)
   {
      // Don't take the last packet's number of samples into account. (If we did, the sample rate would be too large)
      for(int i = 0; i < listSize-1; ++i)
      {
         retVal.numSamp += m_sampleMsgList[i].numSamp;
      }
      retVal.time = m_sampleMsgList[listSize-1].time - m_sampleMsgList[0].time;
   }

   return retVal;
}


void sampleRateCalc::removeOldSampleMsgsFromList(double timeThreshold)
{
   QMutexLocker lock(&m_mutex);
   for(int i = 0; i < m_sampleMsgList.size(); ++i)
   {
      if(m_sampleMsgList[i].time < timeThreshold)
      {
         m_sampleMsgList.removeFirst();
         --i;
      }
      else
      {
         break;
      }
   }
}

void sampleRateCalc::determineSampleRate()
{
   QMutexLocker lock(&m_mutex);
   if(m_sampleMsgList.size() > 1)
   {
      double lastMsgTime = getTimeOfLastMsg();

      removeOldSampleMsgsFromList(lastMsgTime - m_averageTimeBetweenSampleMessages * 100);

      if(m_sampleMsgList.size() > 1)
      {
         tTimeNumSampPair sampleAverage = getSampleMsgListAvg();
         int numMsgsInSampAve = m_sampleMsgList.size() - 1;
         m_averageTimeBetweenSampleMessages = sampleAverage.time / (double)numMsgsInSampAve;
         m_sampleRate = (double)sampleAverage.numSamp / sampleAverage.time;
      }
      else
      {
         m_sampleRate = 0;
         m_averageTimeBetweenSampleMessages = 1000000000; // Initialize to really big number of seconds
      }
   }
}

double sampleRateCalc::getSampleRate()
{
   double retVal;
   QMutexLocker lock(&m_mutex);
   retVal = m_sampleRate;
   return retVal;
}
