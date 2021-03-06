/* Copyright 2016 - 2017, 2019 - 2020 Dan Williams. All Rights Reserved.
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
#include <limits>       // std::numeric_limits
#include <assert.h>
#include "plotSnrCalc.h"



plotSnrCalc::plotSnrCalc(QwtPlot* parentPlot, QLabel* snrLabel):
   m_barsAreItialized(false),
   m_parentPlot(parentPlot),
   m_snrLabel(snrLabel),
   m_parentCurve(NULL),
   m_parentCurve_complex(NULL),
   m_multipleParentCurves(false),
   m_curveSampleRate(0.0),
   m_isVisable(false),
   m_activeBarIndex(-1),
   m_dcBinIndex(-1),
   m_signalPower(0.0),
   m_signalBandwidth(0.0),
   m_noisePower(0.0),
   m_noiseBandwidth(0.0),
   m_noiseBwPerHz(0.0),
   m_snrDb(0.0),
   m_snrDbHz(0.0)
{
   m_snrLabel->setVisible(m_isVisable);

   unsigned int allBarsIndex = 0;
   for(size_t i = 0; i < ARRAY_SIZE(m_noiseBars); ++i)
   {
      QString barName = "Noise Bar " + QString::number(i + 1);
      m_noiseBars[i] = new plotBar(parentPlot, barName, E_X_AXIS, Qt::white, 3, Qt::DashLine, QwtPlotCurve::Lines);
      m_allBars[allBarsIndex++] = m_noiseBars[i];
   }
   for(size_t i = 0; i < ARRAY_SIZE(m_signalBars); ++i)
   {
      QString barName = "Signal Bar " + QString::number(i + 1);
      QColor lightGreen = QColor(128,255,128);
      m_signalBars[i] = new plotBar(parentPlot, barName, E_X_AXIS, lightGreen, 3, Qt::DashLine, QwtPlotCurve::Lines);
      m_allBars[allBarsIndex++] = m_signalBars[i];
   }

}

plotSnrCalc::~plotSnrCalc()
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      delete m_allBars[i];
   }
}


void plotSnrCalc::show(const tMaxMinXY& plotDimensions, const tMaxMinXY& zoomDimensions)
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      m_allBars[i]->updatePlotDim(plotDimensions);
      m_allBars[i]->updateZoomDim(zoomDimensions);
      m_allBars[i]->show();
   }
   m_isVisable = true;
   autoSetBars();
   calcSnrSlow();
   m_snrLabel->setVisible(m_isVisable);
}

void plotSnrCalc::hide()
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      m_allBars[i]->hide();
   }
   m_isVisable = false;
   m_snrLabel->setVisible(m_isVisable);
}

bool plotSnrCalc::isVisable()
{
   return m_isVisable;
}

void plotSnrCalc::updateZoomDim(const maxMinXY& zoomDim)
{
   if(m_isVisable)
   {
      for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
      {
         m_allBars[i]->updateZoomDim(zoomDim);
      }
   }
}

void plotSnrCalc::updatePlotDim(const maxMinXY& plotDim)
{
   if(m_isVisable)
   {
      for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
      {
         m_allBars[i]->updatePlotDim(plotDim);
      }
   }
}


bool plotSnrCalc::isSelectionCloseToBar( const QPointF& pos,
                                         const maxMinXY& zoomDim,
                                         const int canvasWidth_pixels,
                                         const int canvasHeight_pixels)
{
   bool closeBarFound = false;
   double closestBarDelta = std::numeric_limits<double>::max();
   int closestBarIndex = -1; // invalid initial value.
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      double delta;
      bool barFound = m_allBars[i]->isSelectionCloseToBar(pos, zoomDim, canvasWidth_pixels, canvasHeight_pixels, &delta);
      if(barFound)
      {
         if(delta < closestBarDelta)
         {
            closestBarDelta = delta;
            closestBarIndex = i;
            closeBarFound = true;
         }
      }
   }
   if(closeBarFound)
   {
      m_activeBarIndex = closestBarIndex;
   }
   return closeBarFound;
}

void plotSnrCalc::moveBar(const QPointF& pos)
{
   if(m_activeBarIndex >= 0 && m_activeBarIndex < (signed)ARRAY_SIZE(m_allBars))
   {
      m_allBars[m_activeBarIndex]->moveBar(pos);
   }
   calcSnr();
}


void plotSnrCalc::moveToFront()
{
   if(m_isVisable)
   {
      for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
      {
         m_allBars[i]->moveToFront();
      }
   }
}

void plotSnrCalc::setCurve(CurveData* curve, QList<CurveData *>& allCurves)
{
   if(curve != m_parentCurve || (m_multipleParentCurves && m_parentCurve_complex == NULL))
   {
      m_parentCurve = curve;
      double parentSampRate = m_parentCurve->getSampleRate();
      m_curveSampleRate = parentSampRate > 0.0 ? parentSampRate : (double)m_parentCurve->getNumPoints();

      if(curve->getPlotType() == E_PLOT_TYPE_COMPLEX_FFT)
      {
         m_multipleParentCurves = true;
         m_parentCurve_complex = getComplexParentCurve(allCurves);
      }
      else
      {
         m_multipleParentCurves = false;
         m_parentCurve_complex = NULL;
      }

      autoSetBars();
      calcSnrSlow();
   }
}

bool plotSnrCalc::curveUpdated(CurveData* curve, QList<CurveData*>& allCurves)
{
   bool curveIsAValidFftForSnrCalc = false;

   if(m_multipleParentCurves && m_parentCurve_complex == NULL)
   {
      m_parentCurve_complex = getComplexParentCurve(allCurves);
   }

   if(m_isVisable && curve == m_parentCurve && m_parentCurve != NULL)
   {
      calcSnrSlow();
   }
   if(curve != NULL)
   {
      switch(curve->getPlotType())
      {
         case E_PLOT_TYPE_REAL_FFT:
         case E_PLOT_TYPE_COMPLEX_FFT:
         case E_PLOT_TYPE_DB_POWER_FFT_REAL:
         case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
            curveIsAValidFftForSnrCalc = true;
         break;
         default:
            // Nothing to do.
         break;
      }
   }
   return curveIsAValidFftForSnrCalc;
}


CurveData* plotSnrCalc::getComplexParentCurve(QList<CurveData*>& allCurves)
{
   CurveData* retVal = NULL;

   // Complex FFTs have 2 curves, need to find the other curve for the FFT.
   QString curveName = m_parentCurve->getCurveTitle();
   int curveNameLen = curveName.length();

   int reAppLen = COMPLEX_FFT_REAL_APPEND.length();
   int imAppLen = COMPLEX_FFT_IMAG_APPEND.length();

   QString otherCurveName = "";

   // Generate other curve name. If the curve name ends with the real append string
   // make it end with the imag append string, if it ends with the imag append string
   // make it end with the real append string.
   if( curveNameLen >= reAppLen &&
       curveName.right(reAppLen) == COMPLEX_FFT_REAL_APPEND)
   {
      otherCurveName = curveName.left(curveNameLen - reAppLen) + COMPLEX_FFT_IMAG_APPEND;
   }
   else if( curveNameLen >= imAppLen &&
       curveName.right(imAppLen) == COMPLEX_FFT_IMAG_APPEND)
   {
      otherCurveName = curveName.left(curveNameLen - imAppLen) + COMPLEX_FFT_REAL_APPEND;
   }

   // Find the curve with the matching curve name.
   if(otherCurveName != "")
   {
      for(int i = 0; i < allCurves.size(); ++i)
      {
         if(allCurves[i]->getCurveTitle() == otherCurveName)
         {
            retVal = allCurves[i];
            break;
         }
      }
   }

   return retVal;
}

void plotSnrCalc::updateSampleRate()
{
   if(m_parentCurve != NULL)
   {
      double newSampRate = m_parentCurve->getSampleRate();
      double oldSampRate = m_curveSampleRate;

      if(newSampRate <= 0.0)
         newSampRate = (double)m_parentCurve->getNumPoints();

      if(oldSampRate != newSampRate)
      {
         m_curveSampleRate = newSampRate;

         if(m_barsAreItialized)
         {
            // Scale the SNR bar positions so that they stay in the same relative position after the sample rate change.
            for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
            {
               QPointF newBarPos;
               newBarPos.setX(m_allBars[i]->getBarPos() * newSampRate / oldSampRate);
               m_allBars[i]->moveBar(newBarPos);
            }
         }

         if(m_isVisable)
         {
            calcSnrSlow();
            setLabel();
         }
      }
   }
}

void plotSnrCalc::autoSetBars()
{
   if(m_isVisable && m_parentCurve != NULL)
   {
      unsigned int numPoints = m_parentCurve->getNumPoints();
      const double* xPoints = m_parentCurve->getXPoints();

      // Compute full range. 0 to numPoints-1 is off by 1 bin. Add the average Hz per bin to range to finish the computation.
      double range = xPoints[numPoints-1] - xPoints[0];
      double hzPerBin = range / (double)(numPoints-1);
      range += hzPerBin;

      findDcBinIndex(numPoints, xPoints);

      if(m_barsAreItialized == false)
      {
         m_barsAreItialized = true;

         QPointF barPoint;

         barPoint.setX(xPoints[0] + range * 0.1);
         m_noiseBars[0]->moveBar(barPoint);
         barPoint.setX(xPoints[0] + range * 0.9);
         m_noiseBars[1]->moveBar(barPoint);

         barPoint.setX(xPoints[0] + range * 0.4);
         m_signalBars[0]->moveBar(barPoint);
         barPoint.setX(xPoints[0] + range * 0.6);
         m_signalBars[1]->moveBar(barPoint);
      }
   }
}

void plotSnrCalc::calcSnr()
{
   if(m_noiseChunkLeft.indexes.startIndex < 0 || m_signalChunk.indexes.startIndex < 0)
   {
      calcSnrSlow();
   }
   else
   {
      calcSnrFast();
   }
}

void plotSnrCalc::calcSnrSlow()
{
   if(m_isVisable && m_parentCurve != NULL)
   {
      unsigned int numPoints = m_parentCurve->getNumPoints();
      const double* xPoints = m_parentCurve->getXPoints();

      double hzPerBin = (xPoints[numPoints-1] - xPoints[0]) / (double)(numPoints-1);

      findDcBinIndex(numPoints, xPoints);

      tCurveDataIndexes newSignalIndexes = m_signalChunk.indexes;

      // Left has start index, right has stop index.
      tCurveDataIndexes newNoiseIndexes  = m_noiseChunkLeft.indexes;
      newNoiseIndexes.stopIndex = m_noiseChunkRight.indexes.stopIndex;

      // Noise
      findIndexes(
         m_noiseBars[0]->getBarPos(),
         m_noiseBars[1]->getBarPos(),
         &newNoiseIndexes,
         numPoints,
         xPoints,
         hzPerBin);

      // Signal
      findIndexes(
         m_signalBars[0]->getBarPos(),
         m_signalBars[1]->getBarPos(),
         &newSignalIndexes,
         numPoints,
         xPoints,
         hzPerBin);

      tCurveDataIndexes noiseLeft, noiseRight;
      determineNoiseChunks(newNoiseIndexes, newSignalIndexes, noiseLeft, noiseRight);

      calcFftChunk(&m_noiseChunkLeft,  noiseLeft);
      calcFftChunk(&m_noiseChunkRight, noiseRight);
      calcFftChunk(&m_signalChunk,     newSignalIndexes);

      setLabel();
   }
}

void plotSnrCalc::calcSnrFast()
{
   if(m_isVisable && m_parentCurve != NULL)
   {
      unsigned int numPoints = m_parentCurve->getNumPoints();
      const double* xPoints = m_parentCurve->getXPoints();

      double hzPerBin = (xPoints[numPoints-1] - xPoints[0]) / (double)(numPoints-1);

      findDcBinIndex(numPoints, xPoints);

      tCurveDataIndexes newSignalIndexes = m_signalChunk.indexes;

      // Left has start index, right has stop index.
      tCurveDataIndexes newNoiseIndexes  = m_noiseChunkLeft.indexes;
      newNoiseIndexes.stopIndex = m_noiseChunkRight.indexes.stopIndex;

      if(m_activeBarIndex < 2)
      {
         // Noise
         findIndexes(
            m_noiseBars[0]->getBarPos(),
            m_noiseBars[1]->getBarPos(),
            &newNoiseIndexes,
            numPoints,
            xPoints,
            hzPerBin);
      }
      if(m_activeBarIndex >= 2)
      {
         // Signal
         findIndexes(
            m_signalBars[0]->getBarPos(),
            m_signalBars[1]->getBarPos(),
            &newSignalIndexes,
            numPoints,
            xPoints,
            hzPerBin);
      }

      tCurveDataIndexes noiseLeft, noiseRight;
      determineNoiseChunks(newNoiseIndexes, newSignalIndexes, noiseLeft, noiseRight);


      updateFftChunk(&m_noiseChunkLeft,  noiseLeft);
      updateFftChunk(&m_noiseChunkRight, noiseRight);
      updateFftChunk(&m_signalChunk,     newSignalIndexes);

      setLabel();
   }
}

void plotSnrCalc::calcPower(
   tFftBinChunk* fftChunk,
   ePlotType plotType,
   const double* xPoints,
   const double *yPoints,
   const double *yPoints_complex,
   int numPoints,
   double hzPerBin )
{
   double powerSumLinear = 0;

   bool startIndexIsLow  = fftChunk->indexes.startIndex < 0;
   bool startIndexIsHigh = fftChunk->indexes.startIndex >= numPoints;
   bool stopIndexIsLow   = fftChunk->indexes.stopIndex  < 0;
   bool stopIndexIsHigh  = fftChunk->indexes.stopIndex  >= numPoints;

   // If both indexes are out of range on the same side (i.e. both are low or both are high),
   // then return early. Also, return early if start index is ahead of stop index.
   if( (startIndexIsLow && stopIndexIsLow) || (startIndexIsHigh && stopIndexIsHigh) || (fftChunk->indexes.startIndex > fftChunk->indexes.stopIndex) )
   {
      fftChunk->powerLinear = 0.0;
      fftChunk->bandwidth = 0.0;
      return;
   }

   // If we got passed the early return above, then at least one of the start / stop indexes must be correct.
   // If the other index is incorrect, repair it.
   if(startIndexIsLow)
      fftChunk->indexes.startIndex = 0;
   if(stopIndexIsHigh)
      fftChunk->indexes.stopIndex = numPoints-1;

   if(plotType == E_PLOT_TYPE_DB_POWER_FFT_REAL || plotType == E_PLOT_TYPE_DB_POWER_FFT_COMPLEX)
   {
      for(int i = fftChunk->indexes.startIndex; i <= fftChunk->indexes.stopIndex; ++i)
      {
         if(isDoubleValid(yPoints[i]))
         {
            powerSumLinear += pow(10.0, yPoints[i] / 10.0);
         }
      }
   }
   else if(plotType == E_PLOT_TYPE_REAL_FFT)
   {
      for(int i = fftChunk->indexes.startIndex; i <= fftChunk->indexes.stopIndex; ++i)
      {
         if(isDoubleValid(yPoints[i]))
         {
            powerSumLinear += (yPoints[i] * yPoints[i]);
         }
      }
   }
   else if(plotType == E_PLOT_TYPE_COMPLEX_FFT && yPoints_complex != NULL)
   {
      for(int i = fftChunk->indexes.startIndex; i <= fftChunk->indexes.stopIndex; ++i)
      {
         if(isDoubleValid(yPoints[i]) && isDoubleValid(yPoints_complex[i]))
         {
            powerSumLinear += (yPoints[i] * yPoints[i] + yPoints_complex[i] * yPoints_complex[i]);
         }
      }
   }

   fftChunk->powerLinear = powerSumLinear;
   fftChunk->bandwidth =
      xPoints[fftChunk->indexes.stopIndex] - xPoints[fftChunk->indexes.startIndex] + hzPerBin; // Start and stop are inclusive, need to add 1 more bin.

}

bool plotSnrCalc::findDcBinIndex(unsigned int numPoints, const double* xPoints)
{
   int newDcBinIndex = -1;

   if(xPoints[0] == 0.0)
   {
      newDcBinIndex = 0;
   }
   else
   {
      // The DC bin should be right in the middle. Search in the middle for the DC bin.
      int numBinsToCheck = 3;
      if(numPoints < (unsigned int)numBinsToCheck)
      {
         numBinsToCheck = numPoints;
      }

      int midBin = (int)numPoints >> 1;
      int binStart = numBinsToCheck >> 1;
      int startSearchBin = midBin - binStart;
      for(int i = startSearchBin; i < (startSearchBin + numBinsToCheck); ++i)
      {
         if(xPoints[i] == 0.0)
         {
            newDcBinIndex = i;
            break;
         }
      }
   }

   bool changed = newDcBinIndex != m_dcBinIndex;
   m_dcBinIndex = newDcBinIndex;
   return changed;
}

void plotSnrCalc::findIndexes(
      double start,
      double stop,
      tCurveDataIndexes* indexes,
      int numPoints,
      const double* xPoints,
      double hzPerBin)
{
   if(start < stop)
   {
      // Correct order.
      indexes->startIndex = findIndex(start, numPoints, xPoints, hzPerBin, false);
      indexes->stopIndex  = findIndex(stop, numPoints, xPoints, hzPerBin, true);
   }
   else
   {
      // Swap bar order.
      indexes->startIndex = findIndex(stop, numPoints, xPoints, hzPerBin, false);
      indexes->stopIndex  = findIndex(start, numPoints, xPoints, hzPerBin, true);
   }

}

int plotSnrCalc::findIndex(double barPoint, int numPoints, const double* xPoints, double hzPerBin, bool highSide)
{
   int retVal = -1;
   int numBinsToCheck = 5;
   if(numPoints < numBinsToCheck)
   {
      numBinsToCheck = numPoints;
   }

   int midSearchBin = barPoint / hzPerBin;
   int binStart = numBinsToCheck >> 1;
   int startSearchBin = midSearchBin - binStart;
   if(highSide == true)
   {
      retVal = -1; // Default to invalid bin in case the break is never hit.
      for(int i = (startSearchBin + numBinsToCheck - 1); i >= startSearchBin; --i)
      {
         int index = i + m_dcBinIndex;
         if(index < 0)
         {
            index = 0;
         }
         else if(index >= numPoints)
         {
            index = numPoints - 1;
         }

         if(xPoints[index] <= barPoint)
         {
            retVal = index;
            break;
         }
      }
   }
   else
   {
      retVal = numPoints; // Default to invalid bin in case the break is never hit.
      for(int i = startSearchBin; i < (startSearchBin + numBinsToCheck); ++i)
      {
         int index = i + m_dcBinIndex;
         if(index < 0)
         {
            index = 0;
         }
         else if(index >= numPoints)
         {
            index = numPoints - 1;
         }

         if(xPoints[index] >= barPoint)
         {
            retVal = index;
            break;
         }
      }
   }

   return retVal;
}

void plotSnrCalc::determineNoiseChunks(const tCurveDataIndexes& noise, const tCurveDataIndexes& signal, tCurveDataIndexes& noiseLeftRet, tCurveDataIndexes& noiseRightRet)
{
   tCurveDataIndexes noiseLeft = noise;
   tCurveDataIndexes noiseRight = noise;
   bool emptyLeft = false;
   bool emptyRight = false;

   // There are 6 orientations.

   // S1, S2, N1, N2
   if(noise.startIndex > signal.stopIndex)
   {
      // No overlap
      emptyLeft = true;
   }
   // N1, N2, S1, S2
   else if(signal.startIndex > noise.stopIndex)
   {
      // No overlap
      emptyRight = true;
   }
   // S1, N1, N2, S2 (noise is completely overlapped)
   else if(signal.startIndex <= noise.startIndex && noise.stopIndex <= signal.stopIndex)
   {
      emptyLeft = true;
      emptyRight = true;
   }
   // N1, S1, S2, N2
   else if(noise.startIndex < signal.startIndex && signal.stopIndex < noise.stopIndex)
   {
      // Signal is in the middle of noise, there is some noise in both chunks.
      noiseLeft.stopIndex = signal.startIndex-1;
      noiseRight.startIndex = signal.stopIndex+1;
   }
   // S1, N1, S2, N2
   else if(signal.startIndex <= noise.startIndex && signal.stopIndex < noise.stopIndex)
   {
      emptyLeft = true;

      noiseRight.startIndex = signal.stopIndex+1;
   }
   // N1, S1, N2, S2
   else if(noise.startIndex < signal.startIndex && noise.stopIndex <= signal.stopIndex)
   {
      noiseLeft.stopIndex = signal.startIndex-1;

      emptyRight = true;
   }
   else
   {
      assert(0);
   }

   if(emptyLeft)
   {
      // Indexes are inclusive. Make sure start is still the begining of noise. So subtract 1 from start to indicate empty chunk.
      noiseLeft.stopIndex = noiseLeft.startIndex-1;
   }
   if(emptyRight)
   {
      // Indexes are inclusive. Make sure stop is still the end of noise. So add 1 to stop to indicate empty chunk.
      noiseRight.startIndex = noiseRight.stopIndex+1;
   }

   noiseLeftRet = noiseLeft;
   noiseRightRet = noiseRight;
}


bool plotSnrCalc::calcBinDelta(const tCurveDataIndexes& oldIndexes, const tCurveDataIndexes& newIndexes, tFftBinChunk* retFftBinChunk)
{
   unsigned int numPoints = m_parentCurve->getNumPoints();
   ePlotType plotType = m_parentCurve->getPlotType();
   const double* xPoints = m_parentCurve->getXPoints();
   const double* yPoints = m_parentCurve->getYPoints();
   const double* yPoints_complex = m_parentCurve_complex == NULL ? NULL : m_parentCurve_complex->getYPoints();
   double hzPerBin = (xPoints[numPoints-1] - xPoints[0]) / (double)(numPoints-1);

   bool binsAdded = false;
   bool indexesAreDifferent = true;

   // Assume only start or stop has moved. Not both.
   if(newIndexes.startIndex < oldIndexes.startIndex)
   {
      // Adding to bottom.
      assert(newIndexes.stopIndex == oldIndexes.stopIndex);
      retFftBinChunk->indexes.startIndex = newIndexes.startIndex;
      retFftBinChunk->indexes.stopIndex = oldIndexes.startIndex - 1;
      binsAdded = true;
   }
   else if(newIndexes.stopIndex > oldIndexes.stopIndex)
   {
      // Adding to end.
      assert(newIndexes.startIndex == oldIndexes.startIndex);
      retFftBinChunk->indexes.startIndex = oldIndexes.stopIndex + 1;
      retFftBinChunk->indexes.stopIndex = newIndexes.stopIndex;
      binsAdded = true;
   }
   else if(newIndexes.startIndex > oldIndexes.startIndex)
   {
      // Removing from bottom.
      assert(newIndexes.stopIndex == oldIndexes.stopIndex);
      retFftBinChunk->indexes.startIndex = oldIndexes.startIndex;
      retFftBinChunk->indexes.stopIndex = newIndexes.startIndex - 1;
      binsAdded = false;
   }
   else if(newIndexes.stopIndex < oldIndexes.stopIndex)
   {
      // Removing from end.
      assert(newIndexes.startIndex == oldIndexes.startIndex);
      retFftBinChunk->indexes.startIndex = newIndexes.stopIndex + 1;
      retFftBinChunk->indexes.stopIndex = oldIndexes.stopIndex;
      binsAdded = false;
   }
   else
   {
      indexesAreDifferent = false;
   }

   if(indexesAreDifferent)
   {
      calcPower(
         retFftBinChunk,
         plotType,
         xPoints,
         yPoints,
         yPoints_complex,
         numPoints,
         hzPerBin);
   }

   return binsAdded;
}

void plotSnrCalc::updateFftChunk(tFftBinChunk* fftChunk, const tCurveDataIndexes& newIndexes)
{
   if(fftChunk->indexes != newIndexes)
   {
      bool fullCalcFftChunk = false;
      // If start / stop indexes are out of order, no calculations are needed.
      if(newIndexes.startIndex > newIndexes.stopIndex)
      {
         fftChunk->bandwidth = 0;
         fftChunk->powerLinear = 0;
      }
      // If only one index changed, we can just calculate the delta.
      else if( fftChunk->indexes.startIndex == newIndexes.startIndex ||
               fftChunk->indexes.stopIndex == newIndexes.stopIndex )
      {
         double origPower = fftChunk->powerLinear; // Store off so we can check for floating point round error after the delta has been taken into account.

         tFftBinChunk delta;
         bool binsAdded = calcBinDelta(fftChunk->indexes, newIndexes, &delta);
         if(binsAdded)
         {
            fftChunk->bandwidth += delta.bandwidth;
            fftChunk->powerLinear += delta.powerLinear;
         }
         else
         {
            fftChunk->bandwidth -= delta.bandwidth;
            fftChunk->powerLinear -= delta.powerLinear;
         }

         // Check if the delta has caused the logarithmic power to be Not-a-number.
         if(fftChunk->powerLinear <= 0 && origPower > 0)
         {
            fullCalcFftChunk = true; // Floating point rounding has likely caused us to have an invalid power. Do a full FFT Chunk calc.
         }
      }
      else
      {
         fullCalcFftChunk = true; // Do a full FFT Chunk calc.
      }

      if(fullCalcFftChunk)
      {
         calcFftChunk(fftChunk, newIndexes);
      }

      fftChunk->indexes = newIndexes;
   }
}

void plotSnrCalc::calcFftChunk(tFftBinChunk* fftChunk, const tCurveDataIndexes& newIndexes)
{
   unsigned int numPoints = m_parentCurve->getNumPoints();
   ePlotType plotType = m_parentCurve->getPlotType();
   const double* xPoints = m_parentCurve->getXPoints();
   const double* yPoints = m_parentCurve->getYPoints();
   const double* yPoints_complex = m_parentCurve_complex == NULL ? NULL : m_parentCurve_complex->getYPoints();
   double hzPerBin = (xPoints[numPoints-1] - xPoints[0]) / (double)(numPoints-1);

   fftChunk->indexes = newIndexes;
   calcPower(
      fftChunk,
      plotType,
      xPoints,
      yPoints,
      yPoints_complex,
      numPoints,
      hzPerBin);
}

QString plotSnrCalc::doubleToStr(double num)
{
   char result[10];
   bool isNeg = num < 0;
   double absVal = isNeg ? -num : num;

   if(isDoubleValid(num) == false)
   {
      sprintf(result, "   NaN ");
   }
   else if(absVal >= 1000)
   {
      sprintf(result, " %d ", (int)absVal);
   }
   else if(absVal >= 100)
   {
      sprintf(result, " %3.1lf", absVal);
   }
   else if(absVal >= 10)
   {
      sprintf(result, " %2.2lf", absVal);
   }
   else if(absVal >= 1)
   {
      sprintf(result, " %1.3lf", absVal);
   }
   else
   {
      sprintf(result, " %1.3lf", absVal);
   }

   if(isNeg)
      result[0] = '-';
   return QString(result);
}

QString plotSnrCalc::numToStrDb(double num, QString units)
{
   return doubleToStr(num) + " " + units;
}

QString plotSnrCalc::numToHz(double num)
{
   if(num < 1000.0)
   {
      return doubleToStr(num) + "  Hz";
   }
   else if(num < 1000000.0)
   {
      return doubleToStr(num / 1000.0) + " kHz";
   }
   else if(num < 1000000000.0)
   {
      return doubleToStr(num / 1000000.0) + " MHz";
   }
   return doubleToStr(num / 1000000000.0) + " GHz";
}

void plotSnrCalc::setLabel()
{
   QColor color = m_parentCurve->getColor();

   m_signalPower = 10*log10(m_signalChunk.powerLinear);
   m_signalBandwidth = m_signalChunk.bandwidth;

   m_noisePower = 10*log10(m_noiseChunkLeft.powerLinear + m_noiseChunkRight.powerLinear);
   m_noiseBandwidth = m_noiseChunkLeft.bandwidth + m_noiseChunkRight.bandwidth;
   m_noiseBwPerHz = m_noisePower - 10*log10(m_noiseBandwidth);

   m_snrDb = m_signalPower - m_noisePower;
   m_snrDbHz = m_signalPower - m_noiseBwPerHz;

   QString lblText =
         "S: " +
         numToStrDb(m_signalPower, "dB") +
         " / " +
         numToHz(m_signalBandwidth) +
         "  |  N: " +
         numToStrDb(m_noisePower, "dB") +
         " / " +
         numToHz(m_noiseBandwidth) +
         " / " +
         numToStrDb(m_noiseBwPerHz, "dB/Hz") +
         "  |  SNR: " +
         numToStrDb(m_snrDb, "dB") +
         " / " +
         numToStrDb(m_snrDbHz, "dB/Hz");

   QPalette palette = m_snrLabel->palette();
   palette.setColor( QPalette::WindowText, color);
   palette.setColor( QPalette::Text, color);
   m_snrLabel->setPalette(palette);
   m_snrLabel->setText(lblText);
}

double plotSnrCalc::getMeasurement(eFftSigNoiseMeasurements type)
{
   double retVal = NAN;
   switch(type)
   {
      case E_FFT_MEASURE__SIG_POWER:
         retVal = m_signalPower;
      break;
      case E_FFT_MEASURE__SIG_BW:
         retVal = m_signalBandwidth;
      break;
      case E_FFT_MEASURE__NOISE_POWER:
         retVal = m_noisePower;
      break;
      case E_FFT_MEASURE__NOISE_BW:
         retVal = m_noiseBandwidth;
      break;
      case E_FFT_MEASURE__NOISE_PER_HZ:
         retVal = m_noiseBwPerHz;
      break;
      case E_FFT_MEASURE__SNR:
         retVal = m_snrDb;
      break;
      case E_FFT_MEASURE__SNR_PER_HZ:
         retVal = m_snrDbHz;
      break;
      default:
         // Do Nothing.
      break;
   }
   return retVal;
}

QPointF plotSnrCalc::selectedBarPos()
{
   QPointF retVal(NAN, NAN);
   if(m_activeBarIndex >= 0 && m_activeBarIndex < (signed)ARRAY_SIZE(m_allBars))
   {
      double pos = m_allBars[m_activeBarIndex]->getBarPos();
      retVal.setX(pos);
      retVal.setY(pos);
   }
   return retVal;
}

void plotSnrCalc::selectedBarPos(QPointF val)
{
   moveBar(val);
}
