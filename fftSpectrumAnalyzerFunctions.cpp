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
 * fftSpectrumAnalyzerFunctions.cpp
 *
 *  Created on: Mar 27, 2019
 *      Author: d
 */
#include <QMutexLocker>
#include "fftSpectrumAnalyzerFunctions.h"



fftSpecAnFunc::fftSpecAnFunc(ePlotType plotType):
   m_plotType(plotType),
   m_fftSpecAnFuncMutex(QMutex::Recursive),
   m_fftNumBins(0),
   m_numDesiredAvgPoints(1)
{
   // Determine FFT parameters from the Plot Type.
   switch(m_plotType)
   {
      case E_PLOT_TYPE_REAL_FFT:
         m_isFftPlot = true;
         m_isLogFft  = false;
      break;
      case E_PLOT_TYPE_DB_POWER_FFT_REAL:
      case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
         m_isFftPlot = true;
         m_isLogFft  = true;
      break;
      case E_PLOT_TYPE_COMPLEX_FFT: // Complex FFT has negative numbers, which can't be averaged / max hold'd
      default:
         m_isFftPlot = false;
         m_isLogFft  = false;
      break;
   }


   reset();
}

fftSpecAnFunc::~fftSpecAnFunc()
{
   
}

void fftSpecAnFunc::getMaxHoldPoints(dubVect& ioXPoints)
{
   ioXPoints = m_maxHold;
}

void fftSpecAnFunc::getAveragePoints(dubVect& ioXPoints)
{
   ioXPoints = m_avgValue;
}

void fftSpecAnFunc::reset()
{
   QMutexLocker lock(&m_fftSpecAnFuncMutex);

   m_fftNumBins = 0;
   m_maxHold.clear();

   m_allAvgFfts.clear();
   m_numAvgPointsPerBin.clear();
   m_avgSumLinear.clear();
   m_avgValue.clear();

}


void fftSpecAnFunc::update(const dubVect& newPoints, eFftSpecAnTraceType fftSpecAnTraceType)
{
   int newNumBins = newPoints.size();
   if(newNumBins > 0)
   {
      QMutexLocker lock(&m_fftSpecAnFuncMutex);

      if(newNumBins != m_fftNumBins)
      {
         reset();

         // Set the size of the Average vectors to match the new FFT size.
         // Fill the Average vectors with zeros.
         m_avgSumLinear.resize(newNumBins);
         m_numAvgPointsPerBin.resize(newNumBins);
         std::fill(m_avgSumLinear.begin(), m_avgSumLinear.end(), 0);
         std::fill(m_numAvgPointsPerBin.begin(), m_numAvgPointsPerBin.end(), 0);
      }
      m_fftNumBins = newNumBins;

      // Update Max Hold / Average.
      if(fftSpecAnTraceType == E_MAX_HOLD)
      {
         updateMax(newPoints);
      }
      else if(fftSpecAnTraceType == E_AVERAGE)
      {
         updateAvg(newPoints);
      }
   }

}

void fftSpecAnFunc::setAvgSize(int newAvgSize)
{
   if(newAvgSize < 1)
      newAvgSize = 1;

   QMutexLocker lock(&m_fftSpecAnFuncMutex);
   int origSize = m_allAvgFfts.size();
   if(newAvgSize < origSize)
   {
      // Need to reduce. Remove oldest values.
      int numToRemove = origSize - newAvgSize;

      // First, clear out the oldest values from the average.
      for(int fft = 0; fft < numToRemove; ++fft)
      {
         avgSum_sub(m_allAvgFfts[fft]);
      }

      // Next, remove the oldest values from the stored off FFTs
      for(int fft = 0; fft < numToRemove; ++fft)
      {
         m_allAvgFfts.pop_front();
      }
   }
   // else New Average Size is bigger or the same size.

   m_numDesiredAvgPoints = newAvgSize;
   calcAvg();
}


void fftSpecAnFunc::updateMax(const dubVect& newPoints)
{
   if(m_maxHold.size() != newPoints.size())
   {
      // Just copy the values over.
      m_maxHold = newPoints;
   }
   else
   {
      unsigned int newPointsSize = newPoints.size();
      for(unsigned int i = 0; i < newPointsSize; ++i)
      {
         bool oldValid = isDoubleValid(m_maxHold[i]);
         bool newValid = isDoubleValid(newPoints[i]);
         if(!oldValid && newValid)
         {
            // Old value is invalid, new is valid. Use the new value.
            m_maxHold[i] = newPoints[i];
         }
         else if(oldValid && newValid)
         {
            if(newPoints[i] > m_maxHold[i])
            {
               m_maxHold[i] = newPoints[i];
            }
         }
      }
   }
}

void fftSpecAnFunc::updateAvg(const dubVect& newPoints)
{
   bool hadToConvertToLinear = false;
   const dubVect* linearNewPoints = &newPoints; // Default to using input points (if the input is already linear, this avoids a copy)

   // If the FFT is in the log domain, we need to covert to linear.
   if(m_isLogFft)
   {
      dubVect* linearPoints = new dubVect; // This will be deleted later in this function.
      convertToLinear(newPoints, *linearPoints);
      linearNewPoints = linearPoints;
      hadToConvertToLinear = true; // Make sure 'new' gets deleted.
   }

   if(m_allAvgFfts.size() < m_numDesiredAvgPoints)
   {
      // Haven't filled in all the average FFTs yet.
      m_allAvgFfts.push_back(*linearNewPoints);
      avgSum_add(*linearNewPoints);
   }
   else
   {
      // Average is full. Need to overwrite oldest.

      // First, remove oldest from sum.
      avgSum_sub(m_allAvgFfts[0]);

      // Next, add new points to sum.
      avgSum_add(*linearNewPoints);

      // Next, overwrite old FFT bin points with the new ones.
      dubVect* oldest = &m_allAvgFfts[0];
      memcpy( &((*oldest)[0]), &((*linearNewPoints)[0]), sizeof(newPoints[0])*m_fftNumBins );

      // Finally, move the oldest FFT in the list to the back (i.e. make it the newest).
      m_allAvgFfts.move(0, m_numDesiredAvgPoints-1);
   }

   calcAvg();

   // If we had to convert to linear, then the pointer is pointing to dynamically allocated memory. Make sure it is deleted.
   if(hadToConvertToLinear)
   {
      delete linearNewPoints;
   }
}

void fftSpecAnFunc::convertToLinear(const dubVect& fftBins, dubVect& linearBins)
{
   int numFftBins = fftBins.size();
   linearBins.resize(numFftBins);

   for(int bin = 0; bin < numFftBins; ++bin)
   {
      if(isDoubleValid(fftBins[bin]))
      {
         linearBins[bin] += pow(10.0, fftBins[bin] / 10);
      }
      else
      {
         linearBins[bin] = 0; // If not valid, then the input must be 0 (i.e. log of 0 is invalid)
      }
   }
}

void fftSpecAnFunc::avgSum_add(const dubVect& fftBins)
{
   int numFftBins = fftBins.size();
   for(int bin = 0; bin < numFftBins; ++bin)
   {
      if(isDoubleValid(fftBins[bin]))
      {
         m_avgSumLinear[bin] += fftBins[bin];
         m_numAvgPointsPerBin[bin]++;
      }
   }
}

void fftSpecAnFunc::avgSum_sub(const dubVect& fftBins)
{
   int numFftBins = fftBins.size();
   for(int bin = 0; bin < numFftBins; ++bin)
   {
      if(isDoubleValid(fftBins[bin]))
      {
         m_avgSumLinear[bin] -= fftBins[bin];
         m_numAvgPointsPerBin[bin]--;
      }
   }
}

void fftSpecAnFunc::calcAvg()
{
   if(m_avgValue.size() != (size_t)m_fftNumBins)
   {
      m_avgValue.resize(m_fftNumBins);
   }
   if(m_isLogFft)
   {
      for(int bin = 0; bin < m_fftNumBins; ++bin)
      {
         if(m_numAvgPointsPerBin[bin] > 0)
         {
            m_avgValue[bin] = 10.0 * log10(m_avgSumLinear[bin] / m_numAvgPointsPerBin[bin]);
         }
         else
         {
            m_avgValue[bin] = NAN;
         }
      }
   }
   else
   {
      for(int bin = 0; bin < m_fftNumBins; ++bin)
      {
         if(m_numAvgPointsPerBin[bin] > 0)
         {
            m_avgValue[bin] = m_avgSumLinear[bin] / m_numAvgPointsPerBin[bin];
         }
         else
         {
            m_avgValue[bin] = NAN;
         }
      }
   }
}
