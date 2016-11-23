/* Copyright 2016 Dan Williams. All Rights Reserved.
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
   m_curveSampleRate(0.0),
   m_isVisable(false),
   m_activeBarIndex(-1),
   m_dcBinIndex(-1)
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


void plotSnrCalc::moveToFront(bool skipReplot)
{
   if(m_isVisable)
   {
      for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
      {
         m_allBars[i]->moveToFront();
      }
      if(skipReplot == false)
      {
         m_parentPlot->replot();
      }
   }
}

void plotSnrCalc::setCurve(CurveData* curve)
{
   if(curve != m_parentCurve)
   {
      m_parentCurve = curve;
      m_curveSampleRate = m_parentCurve->getSampleRate();
      autoSetBars();
      calcSnrSlow();
   }
}

bool plotSnrCalc::curveUpdated(CurveData* curve)
{
   bool curveIsAValidFftForSnrCalc = false;
   if(m_isVisable && curve == m_parentCurve && m_parentCurve != NULL)
   {
      calcSnrSlow();
   }
   if(curve != NULL)
   {
      switch(curve->getPlotType())
      {
         case E_PLOT_TYPE_REAL_FFT:
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

void plotSnrCalc::sampleRateChanged()
{
   if(m_isVisable && m_parentCurve != NULL)
   {
      double newSampRate = m_parentCurve->getSampleRate();
      if(m_curveSampleRate != newSampRate)
      {
         if(m_curveSampleRate <= 0.0)
         {
            m_curveSampleRate = (double)m_parentCurve->getNumPoints();
         }

         for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
         {
            QPointF newBarPos;
            newBarPos.setX(m_allBars[i]->getBarPos() * newSampRate / m_curveSampleRate);
            m_allBars[i]->moveBar(newBarPos);
         }
         m_curveSampleRate = newSampRate;
         calcSnrSlow();
         setLabel();
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
   if(m_noiseChunk.indexes.startIndex < 0 || m_signalChunk.indexes.startIndex < 0)
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
      QColor color = m_parentCurve->getColor();

      double hzPerBin = (xPoints[numPoints-1] - xPoints[0]) / (double)(numPoints-1);

      findDcBinIndex(numPoints, xPoints);

      tCurveDataIndexes newNoiseIndexes  = m_noiseChunk.indexes;
      tCurveDataIndexes newSignalIndexes = m_signalChunk.indexes;

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

      tCurveDataIndexes newSignalNoiseOverlapIndexes = determineBinOverlap(newNoiseIndexes, newSignalIndexes);

      calcFftChunk(&m_noiseChunk,              newNoiseIndexes);
      calcFftChunk(&m_signalChunk,             newSignalIndexes);
      calcFftChunk(&m_signalNoiseOverlapChunk, newSignalNoiseOverlapIndexes);

      setLabel();
   }
}

void plotSnrCalc::calcSnrFast()
{
   if(m_isVisable && m_parentCurve != NULL)
   {
      unsigned int numPoints = m_parentCurve->getNumPoints();
      const double* xPoints = m_parentCurve->getXPoints();
      QColor color = m_parentCurve->getColor();

      double hzPerBin = (xPoints[numPoints-1] - xPoints[0]) / (double)(numPoints-1);

      findDcBinIndex(numPoints, xPoints);

      tCurveDataIndexes newNoiseIndexes  = m_noiseChunk.indexes;
      tCurveDataIndexes newSignalIndexes = m_signalChunk.indexes;

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
      tCurveDataIndexes newSignalNoiseOverlapIndexes = determineBinOverlap(newNoiseIndexes, newSignalIndexes);

      updateFftChunk(&m_noiseChunk,              newNoiseIndexes);
      updateFftChunk(&m_signalChunk,             newSignalIndexes);
      updateFftChunk(&m_signalNoiseOverlapChunk, newSignalNoiseOverlapIndexes);

      setLabel();
   }
}

void plotSnrCalc::calcPower(
   tFftBinChunk* fftChunk,
   ePlotType plotType,
   const double* xPoints,
   const double *yPoints,
   double hzPerBin )
{
   double powerSumLinear = 0;

   if(plotType == E_PLOT_TYPE_DB_POWER_FFT_REAL || plotType == E_PLOT_TYPE_DB_POWER_FFT_COMPLEX)
   {
      for(int i = fftChunk->indexes.startIndex; i <= fftChunk->indexes.stopIndex; ++i)
      {
         powerSumLinear += pow(10.0, yPoints[i] / 10.0);
      }
   }
   else
   {
      for(int i = fftChunk->indexes.startIndex; i <= fftChunk->indexes.stopIndex; ++i)
      {
         powerSumLinear += yPoints[i];
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


plotSnrCalc::tCurveDataIndexes plotSnrCalc::determineBinOverlap(const tCurveDataIndexes &indexes1, const tCurveDataIndexes &indexes2)
{
   tCurveDataIndexes retVal;
   if( (indexes1.startIndex > indexes2.stopIndex) || (indexes2.startIndex > indexes1.stopIndex) )
   {
      // No Overlap.
   }
   else
   {
      retVal.startIndex = std::max(indexes1.startIndex, indexes2.startIndex);
      retVal.stopIndex  = std::min(indexes1.stopIndex,  indexes2.stopIndex);
   }
   return retVal;
}

bool plotSnrCalc::calcBinDelta(const tCurveDataIndexes& oldIndexes, const tCurveDataIndexes& newIndexes, tFftBinChunk* retFftBinChunk)
{
   unsigned int numPoints = m_parentCurve->getNumPoints();
   ePlotType plotType = m_parentCurve->getPlotType();
   const double* xPoints = m_parentCurve->getXPoints();
   const double* yPoints = m_parentCurve->getYPoints();
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
      if(retFftBinChunk->indexes.startIndex < 0)
         retFftBinChunk->indexes.startIndex = 0;
      if(retFftBinChunk->indexes.stopIndex >= (int)numPoints)
         retFftBinChunk->indexes.stopIndex = numPoints - 1;

      calcPower(
         retFftBinChunk,
         plotType,
         xPoints,
         yPoints,
         hzPerBin);
   }

   return binsAdded;
}

void plotSnrCalc::updateFftChunk(tFftBinChunk* fftChunk, const tCurveDataIndexes& newIndexes)
{
   if(fftChunk->indexes != newIndexes)
   {
      // If only one index changed, we can just calculate the delta.
      if( fftChunk->indexes.startIndex == newIndexes.startIndex ||
          fftChunk->indexes.stopIndex == newIndexes.stopIndex )
      {
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
      }
      else
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
   double hzPerBin = (xPoints[numPoints-1] - xPoints[0]) / (double)(numPoints-1);

   fftChunk->indexes = newIndexes;
   calcPower(
      fftChunk,
      plotType,
      xPoints,
      yPoints,
      hzPerBin);
}

QString plotSnrCalc::numToStrDb(double num, QString units)
{
   int sigFigs = 4;
   return QString::number(num, 'g', sigFigs) + " " + units;
}

QString plotSnrCalc::numToHz(double num)
{
   int sigFigs = 4;
   if(num < 1000.0)
   {
      return QString::number(num, 'g', sigFigs) + " Hz";
   }
   else if(num < 1000000.0)
   {
      return QString::number(num / 1000.0, 'g', sigFigs) + " kHz";
   }
   else if(num < 1000000000.0)
   {
      return QString::number(num / 1000000.0, 'g', sigFigs) + " MHz";
   }
   return QString::number(num / 1000000000.0, 'g', sigFigs) + " GHz";
}

void plotSnrCalc::setLabel()
{
   QColor color = m_parentCurve->getColor();

   double signalPower = 10*log10(m_signalChunk.powerLinear);
   double signalBandwidth = m_signalChunk.bandwidth;

   double noisePower = 10*log10(m_noiseChunk.powerLinear - m_signalNoiseOverlapChunk.powerLinear);
   double noiseBandwidth = m_noiseChunk.bandwidth - m_signalNoiseOverlapChunk.bandwidth;
   double noiseBwPerHz = noisePower - 10*log10(noiseBandwidth);

   QString lblText =
         "S: " +
         numToStrDb(signalPower, "dB") +
         " / " +
         numToHz(signalBandwidth) +
         " | N: " +
         numToStrDb(noisePower, "dB") +
         " / " +
         numToHz(noiseBandwidth) +
         " / " +
         numToStrDb(noiseBwPerHz, "dB/Hz") +
         " | SNR: " +
         numToStrDb(signalPower - noisePower, "dB") +
         " / " +
         numToStrDb(signalPower - noiseBwPerHz, "dB/Hz");

   QPalette palette = m_snrLabel->palette();
   palette.setColor( QPalette::WindowText, color);
   palette.setColor( QPalette::Text, color);
   m_snrLabel->setPalette(palette);
   m_snrLabel->setText(lblText);
}