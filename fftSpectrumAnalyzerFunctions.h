/* Copyright 2019 Dan Williams. All Rights Reserved.
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
 * fftSpectrumAnalyzerFunctions.h
 *
 *  Created on: Mar 27, 2019
 *      Author: d
 */

#ifndef FFTSPECTRUMANALYZERFUNCTIONS_H_
#define FFTSPECTRUMANALYZERFUNCTIONS_H_

#include <QMutex>
#include <QList>

#include "PlotHelperTypes.h"

class fftSpecAnFunc
{
public:
   typedef enum
   {
      E_CLEAR_WRITE,
      E_AVERAGE,
      E_MAX_HOLD
   }eFftSpecAnTraceType;
   
   fftSpecAnFunc(ePlotType plotType);
   ~fftSpecAnFunc();
   
   void reset();
   void update(const dubVect& newPoints);

   void getMaxHoldPoints(dubVect& ioXPoints);
   void getAveragePoints(dubVect& ioXPoints);
   
   void setAvgSize(int newAvgSize);

   bool isFftPlot(){return m_isFftPlot;}

private:
   // Do not allow default constructor, copy contructor, etc.
   fftSpecAnFunc();
   fftSpecAnFunc(fftSpecAnFunc const&);
   void operator=(fftSpecAnFunc const&);

   void updateMax(const dubVect& newPoints);
   void updateAvg(const dubVect& newPoints);
   void calcAvg();

   void avgSum_add(const dubVect& fftBins);
   void avgSum_sub(const dubVect& fftBins);

   ePlotType m_plotType;
   bool m_isFftPlot;
   bool m_isLogFft;

   QMutex m_fftSpecAnFuncMutex;

   int m_fftNumBins;

   dubVect m_maxHold;

   int m_numDesiredAvgPoints;

   QList<dubVect> m_allAvgFfts; // Front is oldest
   std::vector<int> m_numAvgPointsPerBin;
   dubVect m_avgSumLinear;
   dubVect m_avgValue;
};



#endif /* FFTSPECTRUMANALYZERFUNCTIONS_H_ */
