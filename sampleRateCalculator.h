/* Copyright 2017, 2020 Dan Williams. All Rights Reserved.
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
 * sampleRateCalculator.h
 *
 *  Created on: Sep 2, 2017
 *      Author: d
 */

#ifndef SAMPLERATECALCULATOR_H_
#define SAMPLERATECALCULATOR_H_

#include <QMutex>
#include <QList>

class sampleRateCalc
{
   typedef struct TimeNumSampPair
	{
		double time;
		long numSamp;
      TimeNumSampPair(): time(0), numSamp(0){}
	}tTimeNumSampPair;

public:
	sampleRateCalc();
	~sampleRateCalc();

   void newSamples(long numSamp);

   double getSampleRate();
private:

	double m_sampleRate;

	double m_averageTimeBetweenSampleMessages;

   double m_minTimeToStore;
   long m_minMsgsToStore;

   int m_numFirstTimesToRemove;

	QMutex m_mutex;


   QList<tTimeNumSampPair> m_sampleMsgList;
   long m_totalSamplesInMsgList;


   double getTimeOfLastMsg();

   tTimeNumSampPair getSampleMsgListAvg();

   void removeOldSampleMsgsFromList(double timeThreshold);

   void determineSampleRate(double timeOfSecondToLastMsg);

};



#endif /* SAMPLERATECALCULATOR_H_ */
