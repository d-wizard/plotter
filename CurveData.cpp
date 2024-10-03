/* Copyright 2013 - 2024 Dan Williams. All Rights Reserved.
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
#include "CurveData.h"
#include "fftHelper.h"
#include "AmFmPmDemod.h"
#include "handleLogData.h"

///////////////////////////////////////////
// Debug Switches
///////////////////////////////////////////
//#define DEBUG_VALIDATE_SMART_MAX_MIN


///////////////////////////////////////////
// Constants
///////////////////////////////////////////
#define MIN_SAMPLE_PER_MAXMIN_SEGMENT (250)
#define MAX_SAMPLE_PER_MAXMIN_SEGMENT (1000)


CurveData::CurveData( QwtPlot* parentPlot,
                      const CurveAppearance &curveAppearance,
                      const UnpackPlotMsg *data):
   m_parentPlot(parentPlot),
   xOrigPoints(data->m_xAxisValues),
   yOrigPoints(data->m_yAxisValues),
   smartMaxMinXPoints(&xPoints, MIN_SAMPLE_PER_MAXMIN_SEGMENT, MAX_SAMPLE_PER_MAXMIN_SEGMENT),
   smartMaxMinYPoints(&yPoints, MIN_SAMPLE_PER_MAXMIN_SEGMENT, MAX_SAMPLE_PER_MAXMIN_SEGMENT),
   plotDim(plotActionToPlotDim(data->m_plotAction)),
   plotType(data->m_plotType),
   appearance(curveAppearance),
   curve(new QwtPlotCurve(data->m_curveName.c_str())),
   lastMsgIpAddr(0),
   lastMsgXAxisType(E_INVALID_DATA_TYPE),
   lastMsgYAxisType(E_INVALID_DATA_TYPE),
   maxNumPointsFromPlotMsg(0),
   fftSpecAnTraceType(fftSpecAnFunc::E_CLEAR_WRITE),
   fftSpecAn(data->m_plotType)
{
   init();
   if(plotDim != E_PLOT_DIM_1D)
   {
      // Make sure the number of points are the same for x and y.
      if(xOrigPoints.size() > yOrigPoints.size())
         xOrigPoints.resize(yOrigPoints.size());
      else if(xOrigPoints.size() < yOrigPoints.size())
         yOrigPoints.resize(xOrigPoints.size());
   }

   numPoints = yOrigPoints.size();
   handleNewSampleMsg(0, numPoints);

   if(plotDim == E_PLOT_DIM_1D)
   {
      fill1DxPoints();
   }

   performMathOnPoints();
   initCurve();
   setVisible(true); // Display the curve on the parent plot.
   storeLastMsgStats(data);

   // Init parameters that are used to de-mangled values when scroll mode is applied.
   oldestPoint_nonScrollModeVersion = numPoints; // Note data->m_sampleStartIndex is not used in the constructor. All the initial points need to be specified.
   plotSize_nonScrollModeVersion = std::max(plotSize_nonScrollModeVersion, oldestPoint_nonScrollModeVersion);
}

CurveData::~CurveData()
{
   if(curve != NULL)
   {
      delete curve;
      curve = NULL;
   }
   if(pointLabel != NULL)
   {
      delete pointLabel;
      pointLabel = NULL;
   }
   if(curveAction != NULL)
   {
      delete curveAction;
      curveAction = NULL;
   }
   if(mapper != NULL)
   {
      delete mapper;
      mapper = NULL;
   }
}


void CurveData::init()
{
   numPoints = 0;
   pointLabel = NULL;
   curveAction = NULL;
   mapper = NULL;
   attached = false;
   visible = false;
   hidden = false;
   scrollMode = false;

   oldestPoint_nonScrollModeVersion = 0;
   plotSize_nonScrollModeVersion = 0;

   samplePeriod = 0.0;
   sampleRate = 0.0;
   sampleRateIsUserSpecified = false;

   linearXAxisCorrection.m = 1.0;
   linearXAxisCorrection.b = 0.0;

   resetNormalizeFactor();
}

void CurveData::initCurve()
{
   curve->setPen(appearance.color, appearance.width);
   curve->setStyle(appearance.style);
   setCurveSamples();
}

void CurveData::fill1DxPoints()
{
   xOrigPoints.resize(yOrigPoints.size());
   switch(plotType)
   {
      case E_PLOT_TYPE_1D:
      case E_PLOT_TYPE_AM_DEMOD:
      case E_PLOT_TYPE_FM_DEMOD:
      case E_PLOT_TYPE_PM_DEMOD:
      case E_PLOT_TYPE_AVERAGE:
      case E_PLOT_TYPE_DELTA:
      case E_PLOT_TYPE_SUM:
      case E_PLOT_TYPE_MATH_BETWEEN_CURVES:
      case E_PLOT_TYPE_FFT_MEASUREMENT:
      case E_PLOT_TYPE_CURVE_STATS:
      {
         unsigned int xPointSize = xOrigPoints.size();
         if(samplePeriod == 0.0 || samplePeriod == 1.0)
         {
             for(unsigned int i = 0; i < xPointSize; ++i)
             {
                xOrigPoints[i] = (double)i;
             }
         }
         else
         {
             for(unsigned int i = 0; i < xPointSize; ++i)
             {
                xOrigPoints[i] = (double)i * samplePeriod;
             }
         }
         maxMin_1dXPoints.minX = xOrigPoints[0];
         maxMin_1dXPoints.maxX = xOrigPoints[xPointSize-1];
      }
      break;

      case E_PLOT_TYPE_REAL_FFT:
      case E_PLOT_TYPE_DB_POWER_FFT_REAL:
      {
         getFFTXAxisValues_real( xOrigPoints,
                                 xOrigPoints.size(),
                                 maxMin_1dXPoints.minX,
                                 maxMin_1dXPoints.maxX,
                                 sampleRate);
      }
      break;

      case E_PLOT_TYPE_COMPLEX_FFT:
      case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
      {
         getFFTXAxisValues_complex( xOrigPoints,
                                    xOrigPoints.size(),
                                    maxMin_1dXPoints.minX,
                                    maxMin_1dXPoints.maxX,
                                    sampleRate);
      }
      break;

      case E_PLOT_TYPE_2D:
      default:
         // Do nothing.
      break;
   }
}


const double* CurveData::getXPoints()
{
   return &xPoints[0];
}

const double* CurveData::getYPoints()
{
   return &yPoints[0];
}

tLinearXYAxis CurveData::getNormFactor()
{
   return normFactor;
}

void CurveData::getXPoints(dubVect& ioXPoints)
{
   ioXPoints = xPoints;
}
void CurveData::getYPoints(dubVect& ioYPoints)
{
   ioYPoints = yPoints;
}

void CurveData::getXPoints(dubVect& ioXPoints, int startIndex, int stopIndex)
{
   ioXPoints.clear();
   if(startIndex < 0)
      startIndex = 0;
   else if(startIndex >= (int)xPoints.size())
      return;

   if(stopIndex > (int)xPoints.size())
      stopIndex = xPoints.size();
   else if(stopIndex < 0)
      return;

   if(stopIndex > startIndex)
   {
      ioXPoints.assign(&xPoints[startIndex], &xPoints[stopIndex]);
   }
}
void CurveData::getYPoints(dubVect& ioYPoints, int startIndex, int stopIndex)
{
   ioYPoints.clear();
   if(startIndex < 0)
      startIndex = 0;
   else if(startIndex >= (int)yPoints.size())
      return;

   if(stopIndex > (int)yPoints.size())
      stopIndex = yPoints.size();
   else if(stopIndex < 0)
      return;

   if(stopIndex > startIndex)
   {
      ioYPoints.assign(&yPoints[startIndex], &yPoints[stopIndex]);
   }
}

QColor CurveData::getColor()
{
   return appearance.color;
}

ePlotDim CurveData::getPlotDim()
{
   return plotDim;
}

unsigned int CurveData::getNumPoints()
{
   return numPoints;
}

void CurveData::setNumPoints(unsigned int newNumPointsSize)
{
   bool addingPoints = numPoints < newNumPointsSize;
   if(scrollMode)
   {
      unsigned int delta = addingPoints ? newNumPointsSize - numPoints : numPoints - newNumPointsSize;

      numPoints = newNumPointsSize;
      if(addingPoints)
      {
         // Increasing curve size, add "Fill in Point" values to beginning.
         if(plotDim == E_PLOT_DIM_2D)
         {
            yOrigPoints.insert(yOrigPoints.begin(), delta, FILL_IN_POINT_2D);
            xOrigPoints.insert(xOrigPoints.begin(), delta, FILL_IN_POINT_2D);
         }
         else
         {
            yOrigPoints.insert(yOrigPoints.begin(), delta, FILL_IN_POINT_1D);
         }
      }
      else
      {
         // Decreasing curve size, remove samples from beginning.
         yOrigPoints.erase(yOrigPoints.begin(), yOrigPoints.begin()+delta);
         if(plotDim == E_PLOT_DIM_2D)
            xOrigPoints.erase(xOrigPoints.begin(), xOrigPoints.begin()+delta);
      }
   }
   else
   {
      numPoints = newNumPointsSize;
      if(plotDim == E_PLOT_DIM_1D)
      {
         yOrigPoints.resize(numPoints);
      }
      else
      {
         xOrigPoints.resize(numPoints);
         yOrigPoints.resize(numPoints);
      }
   }

   // Make sure all vectors are the same size.
   if(xPoints.size() != xOrigPoints.size())
      xPoints.resize(xOrigPoints.size());
   if(yPoints.size() != yOrigPoints.size())
      yPoints.resize(yOrigPoints.size());

   // If we are shortening the plot size, let the smartMaxMin functionality know.
   if(!addingPoints)
   {
      if(plotDim != E_PLOT_DIM_1D)
      {
         smartMaxMinXPoints.handleShortenedNumPoints();
      }
      smartMaxMinYPoints.handleShortenedNumPoints();
   }

   if(plotDim == E_PLOT_DIM_1D)
      fill1DxPoints();
   performMathOnPoints();
   setCurveSamples();
}

// Find the max and min in the vector, but only allow real numbers to be used
// to determine the max and min (ingore values that are not real, i.e 'Not a Number'
// and Positive Infinity or Negative Inifinty)
void CurveData::findRealMaxMin(const dubVect& inPoints, double& max, double& min)
{
   // Initialize max and min to +/- 1 just in case all the input points are not real numbers.
   max = 1;
   min = -1;

   unsigned int vectSize = inPoints.size();

   if(vectSize > 0)
   {
      unsigned int maxMinStartIndex = 0;

      // Find first point that is a real number.
      for(unsigned int i = maxMinStartIndex; i < vectSize; ++i)
      {
         if(isDoubleValid(inPoints[i])) // Only allow real numbers.
         {
            max = inPoints[i];
            min = inPoints[i];
            maxMinStartIndex = i+1;
            break;
         }
      }

      // Loop through the input value to find the max and min.
      for(unsigned int i = maxMinStartIndex; i < vectSize; ++i)
      {
         if(isDoubleValid(inPoints[i])) // Only allow real numbers.
         {
            if(min > inPoints[i])
               min = inPoints[i];
            if(max < inPoints[i])
               max = inPoints[i];
         }
      }
   } // End if(vectSize > 0)
}


maxMinXY CurveData::getMaxMinXYOfCurve()
{
   return maxMin_finalSamples;
}

maxMinXY CurveData::getMaxMinXYOfData()
{
   return maxMin_beforeScale;
}

// Determines the min/max that will be displayed if one axis has it's range limited.
maxMinXY CurveData::getMaxMinXYOfLimitedCurve(eAxis limitedAxis, double startValue, double stopValue)
{
   maxMinXY retVal = maxMin_beforeScale;
   tSegList fullSegs;
   std::vector<unsigned int> partialPoints;
   if(limitedAxis == E_X_AXIS)
   {
      retVal.maxX = stopValue;
      retVal.minX = startValue;
      retVal.realX = true;
      if(xNormalized)
      {
         startValue = (startValue - normFactor.xAxis.b) / normFactor.xAxis.m;
         stopValue  = (stopValue  - normFactor.xAxis.b) / normFactor.xAxis.m;
      }
      if(plotDim == E_PLOT_DIM_1D)
      {
         // There is no smart min/max for X axis. TODO
      }
      else
      {
         smartMaxMinXPoints.getSegmentsInRange(startValue, stopValue, fullSegs, partialPoints);
         smartMaxMinYPoints.getMaxMinFromSegments(fullSegs, partialPoints, retVal.maxY, retVal.minY, retVal.realY);
      }
      if(yNormalized)
      {
         retVal.maxY = retVal.maxY * normFactor.yAxis.m + normFactor.yAxis.b;
         retVal.minY = retVal.minY * normFactor.yAxis.m + normFactor.yAxis.b;
      }
   }
   else // limitedAxis = E_Y_AXIS
   {
      retVal.maxY = stopValue;
      retVal.minY = startValue;
      retVal.realY = true;
      if(yNormalized)
      {
         startValue = (startValue - normFactor.yAxis.b) / normFactor.yAxis.m;
         stopValue  = (stopValue  - normFactor.yAxis.b) / normFactor.yAxis.m;
      }
      if(plotDim == E_PLOT_DIM_1D)
      {
         // There is no smart min/max for X axis. TODO
      }
      else
      {
         smartMaxMinYPoints.getSegmentsInRange(startValue, stopValue, fullSegs, partialPoints);
         smartMaxMinXPoints.getMaxMinFromSegments(fullSegs, partialPoints, retVal.maxX, retVal.minX, retVal.realX);
      }
      if(xNormalized)
      {
         retVal.maxX = retVal.maxX * normFactor.xAxis.m + normFactor.xAxis.b;
         retVal.minX = retVal.minX * normFactor.xAxis.m + normFactor.xAxis.b;
      }
   }
   return retVal;
}

QString CurveData::getCurveTitle()
{
   return curve->title().text();
}

bool CurveData::isDisplayed()
{
   return attached;
}

void CurveData::findMaxMin()
{
   maxMinXY newMaxMin;
   int vectSize = yPoints.size();
   if(plotDim == E_PLOT_DIM_1D)
   {
      // X points will be in order, use the first/last values for min/max.
      newMaxMin.minX = xPoints[0];
      newMaxMin.maxX = xPoints[vectSize-1];

      smartMaxMinYPoints.getMaxMin(newMaxMin.maxY, newMaxMin.minY, newMaxMin.realY);

#ifdef DEBUG_VALIDATE_SMART_MAX_MIN
      maxMinXY validateMaxMin;
      findRealMaxMin(yPoints, validateMaxMin.maxY, validateMaxMin.minY);
      if(validateMaxMin.maxY != newMaxMin.maxY || validateMaxMin.minY != newMaxMin.minY)
      {
         printf("Smart Max/Min is wrong\n");
      }
#endif

   }
   else
   {
      // X Points may not be in order.
      smartMaxMinXPoints.getMaxMin(newMaxMin.maxX, newMaxMin.minX, newMaxMin.realX);
      smartMaxMinYPoints.getMaxMin(newMaxMin.maxY, newMaxMin.minY, newMaxMin.realY);

#ifdef DEBUG_VALIDATE_SMART_MAX_MIN
      maxMinXY validateMaxMin;
      findRealMaxMin(xPoints, validateMaxMin.maxX, validateMaxMin.minX);
      findRealMaxMin(yPoints, validateMaxMin.maxY, validateMaxMin.minY);
      if( validateMaxMin.maxX != newMaxMin.maxX || validateMaxMin.minX != newMaxMin.minX ||
          validateMaxMin.maxY != newMaxMin.maxY || validateMaxMin.minY != newMaxMin.minY )
      {
         printf("Smart Max/Min is wrong\n");
      }
#endif

   }
   maxMin_beforeScale = newMaxMin;
}

void CurveData::setNormalizeFactor(maxMinXY desiredScale, bool normXAxis, bool normYAxis)
{
   if(desiredScale.maxX == desiredScale.minX) // Max sure desired scale height /width isn't zero.
   {
      desiredScale.maxX += 1.0;
      desiredScale.minX -= 1.0;
   }
   if(desiredScale.maxY == desiredScale.minY)
   {
      desiredScale.maxY += 1.0;
      desiredScale.minY -= 1.0;
   }

   if(!normXAxis || (desiredScale.maxX == maxMin_beforeScale.maxX && desiredScale.minX == maxMin_beforeScale.minX))
   {
      normFactor.xAxis.m = 1.0;
      normFactor.xAxis.b = 0.0;
      xNormalized = false;
   }
   else
   {
      if( (maxMin_beforeScale.maxX - maxMin_beforeScale.minX) > 0)
      {
         normFactor.xAxis.m = (desiredScale.maxX - desiredScale.minX) / (maxMin_beforeScale.maxX - maxMin_beforeScale.minX);
         normFactor.xAxis.b = desiredScale.maxX - (normFactor.xAxis.m * maxMin_beforeScale.maxX);
      }
      else
      {
         // No width. Don't change the scale. Just move to middle.
         normFactor.xAxis.m = 1.0;
         normFactor.xAxis.b = (desiredScale.maxX - desiredScale.minX) / 2.0 + desiredScale.minX - maxMin_beforeScale.minX;
      }
      xNormalized = true;
   }

   if(!normYAxis || (desiredScale.maxY == maxMin_beforeScale.maxY && desiredScale.minY == maxMin_beforeScale.minY))
   {
      normFactor.yAxis.m = 1.0;
      normFactor.yAxis.b = 0.0;
      yNormalized = false;
   }
   else
   {
      if( (maxMin_beforeScale.maxY - maxMin_beforeScale.minY) > 0)
      {
         normFactor.yAxis.m = (desiredScale.maxY - desiredScale.minY) / (maxMin_beforeScale.maxY - maxMin_beforeScale.minY);
         normFactor.yAxis.b = desiredScale.maxY - (normFactor.yAxis.m * maxMin_beforeScale.maxY);
      }
      else
      {
         // No height. Don't change the scale. Just move to middle.
         normFactor.yAxis.m = 1.0;
         normFactor.yAxis.b = (desiredScale.maxY - desiredScale.minY) / 2.0 + desiredScale.minY - maxMin_beforeScale.minY;
      }
      yNormalized = true;
   }

}

void CurveData::resetNormalizeFactor()
{
   normFactor.xAxis.m = 1.0;
   normFactor.xAxis.b = 0.0;
   xNormalized = false;
   
   normFactor.yAxis.m = 1.0;
   normFactor.yAxis.b = 0.0;
   yNormalized = false;
}


// xStartIndex is a return value, inclusive.
// xEndIndex is a return value, exclusive.
// sampPerPixel is a return value.
// The member variable numPoints must be 2 or greater.
void CurveData::getSamplesToSendToGui_1D(dubVect* xPointsForGui, int& xStartIndex, int& xEndIndex, unsigned int& sampPerPixel, bool addMargin)
{
   xStartIndex = 0;
   xEndIndex = numPoints;

   int windowWidthPixels = m_parentPlot->canvas()->width();
   if(windowWidthPixels > 0)
   {
      QwtScaleDiv plotZoomWidthDim = m_parentPlot->axisScaleDiv(QwtPlot::xBottom); // Get plot zoom dimensions.
      double zoomMin = plotZoomWidthDim.lowerBound();
      double zoomMax = plotZoomWidthDim.upperBound();
      double zoomWidth = zoomMax - zoomMin;
      double initialXPoint = (*xPointsForGui)[0];
      double finalXPoint = (*xPointsForGui)[numPoints-1];
      double distBetweenSamples = (finalXPoint - initialXPoint) / (double)(numPoints-1);

      double sampPerPixel_float = zoomWidth / ((double)windowWidthPixels * distBetweenSamples);

      // Make an educated guess as to where the start / stop indexes are that map to the zoom start / stop.
      // These values can reduce the load when determine the true start / stop indexes.
      double xStartIndex_guess = (zoomMin - initialXPoint) / distBetweenSamples;
      double xEndIndex_guess   = (zoomMax - initialXPoint) / distBetweenSamples;

      // Find the actual points that map to the zoom start / stop (use educated guess as a starting point).
      double xStartIndex_float = findFirstSampleGreaterThan(xPointsForGui, xStartIndex_guess, zoomMin) - 1;
      double xEndIndex_float = findFirstSampleGreaterThan(xPointsForGui, xEndIndex_guess, zoomMax);

      if(addMargin)
      {
         // Add some margin so we plot a little before / after the zoom coordinates.
         double margin = sampPerPixel_float > 0 ? 2.0*sampPerPixel_float : 2.0;
         xStartIndex_float -= margin;
         xEndIndex_float   += margin;
      }

      // Bound float based index when converting to integer (just in case the floating point version is bad... avoid overflow)
      double limitBasedOnMargin = addMargin ? 0 : -1; // If we are not adding any margin, need to use -1 to indicate the first sample is included.
      if(xStartIndex_float < limitBasedOnMargin)
         xStartIndex = limitBasedOnMargin;
      else if(xStartIndex_float >= (double)numPoints)
         xStartIndex = numPoints-1;
      else
         xStartIndex = xStartIndex_float; // could call std::floor, but cast to int rounds down anyway.

      if(xEndIndex_float < 0)
         xEndIndex = 0;
      else if(xEndIndex_float >= (double)numPoints)
         xEndIndex = numPoints;
      else
         xEndIndex = (int)std::ceil(xEndIndex_float);

      if(xEndIndex <= xStartIndex)
      {
         // Calculated indexes are not valid, reset all values to default and let the calling function deal with it.
         sampPerPixel = 0;
         xStartIndex = 0;
         xEndIndex = numPoints;
         return;
      }

      double sampPerPixel_scaleCheck = 4.0;
      if( (sampPerPixel_float * sampPerPixel_scaleCheck) > (double)numPoints)
      {
         // If 4 pixels worth of samples is greater than all the samples we are plotting,
         // set samples per pixels to 1/4 of the number of samples.
         // The idea is since we will only be sending a few samples to the GUI (since the
         // sample per pixel is so low), we may as well slightly increase the number
         // of samples per pixel to avoid low resolution issues.
         sampPerPixel = (double)numPoints / sampPerPixel_scaleCheck;
      }
      else
      {
         sampPerPixel = sampPerPixel_float; // Convert from float to int (round down).
      }
   }
}


maxMinXY CurveData::get1dDisplayedIndexes()
{
   maxMinXY retVal;
   retVal.minX = -1;
   retVal.maxX = -1;

   if(plotDim == E_PLOT_DIM_1D && numPoints > 1)
   {
      dubVect* xPointsForGui = xNormalized ? &normX : &xPoints;

      unsigned int sampPerPixel = 0;
      int xStartIndex = 0;
      int xEndIndex = numPoints;

      getSamplesToSendToGui_1D(xPointsForGui, xStartIndex, xEndIndex, sampPerPixel, false);

      retVal.minX = xStartIndex;
      retVal.maxX = xEndIndex;
   }

   return retVal;
}

maxMinXY CurveData::get2dDisplayedIndexes(unsigned &numNonContiguousSamples)
{
   numNonContiguousSamples = 0;
   int minIndex = -1;
   int maxIndex = -1;

   if(plotDim == E_PLOT_DIM_2D && numPoints > 1)
   {
      dubVect& xPointsForGui = xNormalized ? normX : xPoints;
      dubVect& yPointsForGui = yNormalized ? normY : yPoints;

      maxMinXY zoomDim;
      QwtScaleDiv plotZoomWidthDim = m_parentPlot->axisScaleDiv(QwtPlot::xBottom); // Get plot zoom dimensions.
      QwtScaleDiv plotZoomHeightDim = m_parentPlot->axisScaleDiv(QwtPlot::yLeft); // Get plot zoom dimensions.
      zoomDim.minX = plotZoomWidthDim.lowerBound();
      zoomDim.maxX = plotZoomWidthDim.upperBound();
      zoomDim.minY = plotZoomHeightDim.lowerBound();
      zoomDim.maxY = plotZoomHeightDim.upperBound();

      bool pointInZoomWindowFound = false;
      for(unsigned i = 0; i < numPoints; ++i)
      {
         if( (xPointsForGui[i] >= zoomDim.minX && xPointsForGui[i] <= zoomDim.maxX) &&
             (yPointsForGui[i] >= zoomDim.minY && yPointsForGui[i] <= zoomDim.maxY) )
         {
            // The point is being displayed.
            if(!pointInZoomWindowFound)
            {
               // First point found. Set min/max to this point
               minIndex = i;
               maxIndex = i;
               pointInZoomWindowFound = true;
            }
            else
            {
               // Already have found some points.
               unsigned sampleDelta = i - (unsigned)maxIndex;
               numNonContiguousSamples += (sampleDelta - 1); // sampleDelta should be 1 if the samples are contiguous
               maxIndex = i;
            }
         }
      }
   }

   // The return value type is maxMinXY, but really only the minX / minY values are used to specify the indexes.
   maxMinXY retVal;
   retVal.minX = minIndex;
   retVal.maxX = maxIndex;
   return retVal;
}

void CurveData::get2dDisplayedPoints(dubVect& xAxis, dubVect& yAxis)
{
   xAxis.clear();
   yAxis.clear();

   if(plotDim == E_PLOT_DIM_2D)
   {
      dubVect& xPointsForGui = xNormalized ? normX : xPoints;
      dubVect& yPointsForGui = yNormalized ? normY : yPoints;

      maxMinXY zoomDim;
      QwtScaleDiv plotZoomWidthDim = m_parentPlot->axisScaleDiv(QwtPlot::xBottom); // Get plot zoom dimensions.
      QwtScaleDiv plotZoomHeightDim = m_parentPlot->axisScaleDiv(QwtPlot::yLeft); // Get plot zoom dimensions.
      zoomDim.minX = plotZoomWidthDim.lowerBound();
      zoomDim.maxX = plotZoomWidthDim.upperBound();
      zoomDim.minY = plotZoomHeightDim.lowerBound();
      zoomDim.maxY = plotZoomHeightDim.upperBound();

      // Make room for all the points to avoid copies being done on each push_back
      xAxis.reserve(xPoints.size());
      yAxis.reserve(yPoints.size());

      for(unsigned i = 0; i < numPoints; ++i)
      {
         if( (xPointsForGui[i] >= zoomDim.minX && xPointsForGui[i] <= zoomDim.maxX) &&
             (yPointsForGui[i] >= zoomDim.minY && yPointsForGui[i] <= zoomDim.maxY) )
         {
            // The point is being displayed.
            xAxis.push_back(xPoints[i]);
            yAxis.push_back(yPoints[i]);
         }
      }
   }
}

void CurveData::setDisplayedPoints(double val)
{
   if(plotDim == E_PLOT_DIM_1D && numPoints > 1)
   {
      maxMinXY indexes = get1dDisplayedIndexes();

      // Indexes might be out of bounds.
      indexes.minX = std::max(indexes.minX, double(0));
      indexes.maxX = std::min(indexes.maxX, double(numPoints));

      for(unsigned i = indexes.minX; i < indexes.maxX; ++i)
      {
         yPoints[i] = val;
      }
   }
   else if(plotDim == E_PLOT_DIM_2D && numPoints > 1)
   {
      // Not sure how useful this is. (i.e. why would I want to set both x and y axes to the same value?)
      dubVect& xPointsForGui = xNormalized ? normX : xPoints;
      dubVect& yPointsForGui = yNormalized ? normY : yPoints;

      maxMinXY zoomDim;
      QwtScaleDiv plotZoomWidthDim = m_parentPlot->axisScaleDiv(QwtPlot::xBottom); // Get plot zoom dimensions.
      QwtScaleDiv plotZoomHeightDim = m_parentPlot->axisScaleDiv(QwtPlot::yLeft); // Get plot zoom dimensions.
      zoomDim.minX = plotZoomWidthDim.lowerBound();
      zoomDim.maxX = plotZoomWidthDim.upperBound();
      zoomDim.minY = plotZoomHeightDim.lowerBound();
      zoomDim.maxY = plotZoomHeightDim.upperBound();

      for(unsigned i = 0; i < numPoints; ++i)
      {
         if( (xPointsForGui[i] >= zoomDim.minX && xPointsForGui[i] <= zoomDim.maxX) &&
             (yPointsForGui[i] >= zoomDim.minY && yPointsForGui[i] <= zoomDim.maxY) )
         {
            // The point is being displayed.
            xPoints[i] = val;
            yPoints[i] = val;
         }
      }
   }
}

int CurveData::findFirstSampleGreaterThan(dubVect* xPointsForGui, double startSearchIndex, double compareValue)
{
   if((*xPointsForGui)[0] > compareValue)
      return 0;
   if((*xPointsForGui)[numPoints - 1] <= compareValue)
      return numPoints;

   int retVal = -1;
   double compareWidth = 1;

   while(retVal < 0)
   {
      double startIndex_float = startSearchIndex - compareWidth;
      double endIndex_float = startSearchIndex + compareWidth;

      // Convert from floating point to int (bound to ensure we don't overflow when converting from floating point).
      int startIndex;
      if(startIndex_float < 0)
         startIndex = 0;
      else if(startIndex_float >= (double)numPoints)
         startIndex = numPoints - 1;
      else
         startIndex = (int)startIndex_float; // could call std::floor, but cast to int rounds down anyway.

      // Convert from floating point to int (bound to ensure we don't overflow when converting from floating point).
      int endIndex;
      if(endIndex_float < 0)
         endIndex = 0;
      else if(endIndex_float >= (double)numPoints)
         endIndex = numPoints;
      else
         endIndex = (int)std::ceil(endIndex_float);

      bool startIsLessOrEqual = (*xPointsForGui)[startIndex] <= compareValue;
      bool endIsGreater = (*xPointsForGui)[endIndex-1] > compareValue;
      if(startIsLessOrEqual && endIsGreater)
      {
         for(int i = startIndex; i < endIndex && retVal < 0; ++i)
         {
            if((*xPointsForGui)[i] > compareValue)
            {
               retVal = i;
            }
         }
      }
      else
      {
         // TODO: should just set next startIndex_float and endIndex_float indexes here, to ensure we don't recheck indexes we have already checked.
         if(!startIsLessOrEqual)
         {
            startSearchIndex = startIndex_float - compareWidth;
         }
         else
         {
            startSearchIndex = endIndex_float + compareWidth;
         }
         compareWidth *= 2;
      }
   }
   return retVal;
}

void CurveData::setCurveDataGuiPoints(bool onlyNeedToUpdate1D)
{
   dubVect* xPointsForGui = xNormalized ? &normX : &xPoints;
   dubVect* yPointsForGui = yNormalized ? &normY : &yPoints; // fastMonotonicMaxMin works today because it uses yPoints for max/min then redoes the normalization. If a change brings in more difference between yPoints and yPointsForGui that might break fastMonotonicMaxMin.

   if(numPoints <= 0)
   {
      return;
   }

   // For now, I don't know how to reduce 2D plots, so just plot all samples in that case.
   // 1D sample reduce only works if there is more than 1 sample, so just plot all samples if there is only 1 sample.
   // Also, 1D sample reduce can cause confusion when using the Dots Curve Style, don't use the 1D reduce for Dots.
   if(plotDim != E_PLOT_DIM_1D || numPoints == 1 || appearance.style == QwtPlotCurve::Dots)
   {
      if(onlyNeedToUpdate1D == false)
      {
         curve->setSamples( &(*xPointsForGui)[0],
                            &(*yPointsForGui)[0],
                            numPoints);
      }
      return;
   }

   // All situations where we wouldn't want to use the 1D sample reduction code have been checked for (see the previous if statement).
   // Assume E_PLOT_DIM_1D from here on out.
   // This method only works if the X axis samples are monotonically increasing at a constant rate,
   // which will be true for E_PLOT_DIM_1D.

   unsigned int sampPerPixel = 0;
   int xStartIndex = 0;
   int xEndIndex = numPoints;

   // Calculate xStartIndex, xEndIndex, sampPerPixel values.
   getSamplesToSendToGui_1D(xPointsForGui, xStartIndex, xEndIndex, sampPerPixel, true);

   // Don't plot NAN points at the beginning / end of curve data.
   {
      int xStartIndexLimit, xEndIndexLimit;
      smartMaxMinYPoints.getFirstLastReal(xStartIndexLimit, xEndIndexLimit);
      xEndIndexLimit++; // Convert inclusive to exclusive.

      // Update indexes to avoid plotting NAN point at the beginning / end,
      // but only if they don't cause the indexes to get out of order.
      if(xStartIndexLimit > xStartIndex && xStartIndexLimit < xEndIndex)
         xStartIndex = xStartIndexLimit;
      if(xEndIndexLimit < xEndIndex && xEndIndexLimit > xStartIndex)
         xEndIndex = xEndIndexLimit;
   }

   if( (sampPerPixel <= 3) ||
       (numPoints < (sampPerPixel+2)) )
   {
      curve->setSamples( &(*xPointsForGui)[xStartIndex],
                         &(*yPointsForGui)[xStartIndex],
                         xEndIndex-xStartIndex);
   }
   else
   {
      // TODO should be able to predict how large this needs to be, rather than just setting it to the max.
      reducedXPoints.resize(xPointsForGui->size());
      reducedYPoints.resize(yPointsForGui->size());
      fastMonotonicMaxMin fastMinMax(smartMaxMinYPoints);

      unsigned int sampCount = 0;

      reducedXPoints[sampCount] = (*xPointsForGui)[xStartIndex];
      reducedYPoints[sampCount] = (*yPointsForGui)[xStartIndex];
      sampCount++;

      for(int i = (xStartIndex+1); i < (xEndIndex-1); i += sampPerPixel)
      {
         unsigned int sampToProcess = std::min((int)sampPerPixel, (xEndIndex-1) - i);
         tMaxMinSegment maxMin = fastMinMax.getMinMaxInRange(i, sampToProcess);

         if(yNormalized)
         {
            maxMin.maxValue = (normFactor.yAxis.m * maxMin.maxValue) + normFactor.yAxis.b;
            maxMin.minValue = (normFactor.yAxis.m * maxMin.minValue) + normFactor.yAxis.b;
         }

         if(!maxMin.realPoints)
         {
            reducedXPoints[sampCount] = (*xPointsForGui)[i]; // No valid points, set to start index in the range.
            reducedYPoints[sampCount] = NAN;
            sampCount++;
         }
         else if(maxMin.minIndex < maxMin.maxIndex)
         {
            reducedXPoints[sampCount] = (*xPointsForGui)[maxMin.minIndex];
            reducedYPoints[sampCount] = maxMin.minValue;
            sampCount++;
            reducedXPoints[sampCount] = (*xPointsForGui)[maxMin.maxIndex];
            reducedYPoints[sampCount] = maxMin.maxValue;
            sampCount++;
         }
         else if(maxMin.minIndex > maxMin.maxIndex)
         {
            reducedXPoints[sampCount] = (*xPointsForGui)[maxMin.maxIndex];
            reducedYPoints[sampCount] = maxMin.maxValue;
            sampCount++;
            reducedXPoints[sampCount] = (*xPointsForGui)[maxMin.minIndex];
            reducedYPoints[sampCount] = maxMin.minValue;
            sampCount++;
         }
         else
         {
            reducedXPoints[sampCount] = (*xPointsForGui)[maxMin.maxIndex];
            reducedYPoints[sampCount] = maxMin.maxValue;
            sampCount++;
         }
      }

      reducedXPoints[sampCount] = (*xPointsForGui)[xEndIndex-1];
      reducedYPoints[sampCount] = (*yPointsForGui)[xEndIndex-1];
      sampCount++;

      curve->setSamples( &reducedXPoints[0],
                         &reducedYPoints[0],
                         sampCount);
   }
}

void CurveData::setCurveSamples()
{
   // To save on processing, calculate final max/min.
   maxMinXY finalMaxMin;
   finalMaxMin.minY  = maxMin_beforeScale.minY;
   finalMaxMin.maxY  = maxMin_beforeScale.maxY;
   finalMaxMin.realY = maxMin_beforeScale.realY;
   if(plotDim == E_PLOT_DIM_1D)
   {
      finalMaxMin.minX  = maxMin_1dXPoints.minX;
      finalMaxMin.maxX  = maxMin_1dXPoints.maxX;
      finalMaxMin.realX = true; // 1D X Point are generated, thus they are always valid.
   }
   else
   {
      finalMaxMin.minX  = maxMin_beforeScale.minX;
      finalMaxMin.maxX  = maxMin_beforeScale.maxX;
      finalMaxMin.realX = maxMin_beforeScale.realX;
   }

   if(xNormalized)
   {
      normX.resize(numPoints);
      for(unsigned int i = 0; i < numPoints; ++i)
      {
         normX[i] = (normFactor.xAxis.m * xPoints[i]) + normFactor.xAxis.b;
      }

      // To save on processing, calculate final max/min.
      finalMaxMin.minX = (normFactor.xAxis.m * finalMaxMin.minX) + normFactor.xAxis.b;
      finalMaxMin.maxX = (normFactor.xAxis.m * finalMaxMin.maxX) + normFactor.xAxis.b;
   }
   if(yNormalized)
   {
      normY.resize(numPoints);
      for(unsigned int i = 0; i < numPoints; ++i)
      {
         normY[i] = (normFactor.yAxis.m * yPoints[i]) + normFactor.yAxis.b;
      }

      // To save on processing, calculate final max/min.
      finalMaxMin.minY = (normFactor.yAxis.m * finalMaxMin.minY) + normFactor.yAxis.b;
      finalMaxMin.maxY = (normFactor.yAxis.m * finalMaxMin.maxY) + normFactor.yAxis.b;
   }

   setCurveDataGuiPoints(false); // Need to set GUI points regardless of 1D vs 2D.

   maxMin_finalSamples = finalMaxMin;
}

void CurveData::ResetCurveSamples(const UnpackPlotMsg* data)
{
   plotDim = plotActionToPlotDim(data->m_plotAction);

   if(plotDim == E_PLOT_DIM_1D)
   {
      yOrigPoints = data->m_yAxisValues;
      numPoints = yOrigPoints.size();
      fill1DxPoints();
   }
   else
   {
      xOrigPoints = data->m_xAxisValues;
      yOrigPoints = data->m_yAxisValues;
      numPoints = std::min(xOrigPoints.size(), yOrigPoints.size());
      if(xOrigPoints.size() > numPoints)
         xOrigPoints.resize(numPoints);
      if(yOrigPoints.size() > numPoints)
         yOrigPoints.resize(numPoints);
   }

   handleNewSampleMsg(0, numPoints);

   performMathOnPoints();
   setCurveSamples();
   storeLastMsgStats(data);
}


void CurveData::swapSamples(dubVect& samples, int swapIndex)
{
   int numPointsAtEnd = numPoints - swapIndex;
   int numPointsAtBeg = numPoints - numPointsAtEnd;
   size_t pointSize = sizeof(samples[0]);

   if(numPointsAtEnd > 0 && numPointsAtBeg > 0)
   {
      dubVect tempPoints;
      
      // Swap Beginning / End Points.
      tempPoints.resize(numPointsAtBeg);
      memcpy(&tempPoints[0], &samples[0], pointSize * numPointsAtBeg); // Store off the Beginning Points.
      
      memmove(&samples[0], &samples[numPointsAtBeg], pointSize * numPointsAtEnd);   // Move End Points
      memcpy(&samples[numPointsAtEnd], &tempPoints[0], pointSize * numPointsAtBeg); // Move Beginning Points.
   }
}

void CurveData::handleScrollModeTransitions(bool plotScrollMode)
{
   if(plotScrollMode != scrollMode)
   {
      if(!plotScrollMode)
      {
         // Toggling out of scroll mode. If num points has been changed, set it back before reverting out of scroll mode.
         if(plotSize_nonScrollModeVersion != numPoints && plotSize_nonScrollModeVersion > 0)
         {
            setNumPoints(plotSize_nonScrollModeVersion);
         }
      }

      int swapPoint = plotScrollMode ? oldestPoint_nonScrollModeVersion : numPoints - oldestPoint_nonScrollModeVersion;

      swapSamples(yOrigPoints, swapPoint);
      swapSamples(yPoints,     swapPoint);
      smartMaxMinYPoints.updateMaxMin(0, yOrigPoints.size());
      if(plotDim == E_PLOT_DIM_2D)
      {
         swapSamples(xOrigPoints, swapPoint);
         swapSamples(xPoints,     swapPoint);
         smartMaxMinXPoints.updateMaxMin(0, xOrigPoints.size());
      }
      scrollMode = plotScrollMode; // Store off Scroll Mode state.
   }
}

void CurveData::UpdateCurveSamples(const UnpackPlotMsg* data)
{
   if(plotDim == plotActionToPlotDim(data->m_plotAction))
   {
      if(plotDim == E_PLOT_DIM_1D)
      {
         UpdateCurveSamples(data->m_yAxisValues, data->m_sampleStartIndex, false);
      }
      else
      {
         UpdateCurveSamples(data->m_xAxisValues, data->m_yAxisValues, data->m_sampleStartIndex, false);
      }
      storeLastMsgStats(data);
   }
}

void CurveData::UpdateCurveSamples(const dubVect& newYPoints, unsigned int sampleStartIndex, bool modifySpecificPoints)
{
   if(plotDim == E_PLOT_DIM_1D)
   {
      bool updateAsScrollMode = scrollMode && !modifySpecificPoints;

      const dubVect* newPointsToUse = &newYPoints;
      dubVect fftSpecAnPoints;

      // Check if the input points should be overwritten with values from the FFT Spectrum Analyzer Functionality.
      if(fftSpecAn.isFftPlot() && !modifySpecificPoints)
      {
         fftSpecAn.update(newYPoints, fftSpecAnTraceType);
         if(fftSpecAnTraceType == fftSpecAnFunc::E_MAX_HOLD)
         {
            fftSpecAn.getMaxHoldPoints(fftSpecAnPoints);
         }
         else if(fftSpecAnTraceType == fftSpecAnFunc::E_AVERAGE)
         {
            fftSpecAn.getAveragePoints(fftSpecAnPoints);
         }

         // If samples have been written to fftSpecAnPoints, then we should use them.
         if(fftSpecAnPoints.size() > 0)
         {
            newPointsToUse = &fftSpecAnPoints;
         }
      }
      
      int newPointsSize = newPointsToUse->size();

      handleNewSampleMsg(sampleStartIndex, newPointsSize);

      bool resized = false;

      if(updateAsScrollMode == false)
      {
         // Check if the current size can handle the new samples.
         if(yOrigPoints.size() < (sampleStartIndex + newPointsSize))
         {
            resized = true;
            yOrigPoints.resize(sampleStartIndex + newPointsSize);
         }
         memcpy(&yOrigPoints[sampleStartIndex], &(*newPointsToUse)[0], sizeof(yOrigPoints[0]) * newPointsSize);
      }
      else
      {
         /////////////////////////////
         // Scroll Mode
         /////////////////////////////
         if(yOrigPoints.size() < (size_t)newPointsSize)
         {
            // Current scroll mode curve size is less than the number of samples in this new curve data message.
            // Resize the curve to fit all the new data.
            resized = true;
            yOrigPoints = (*newPointsToUse);
         }
         else
         {
            int numOrigPointsToKeep = yOrigPoints.size() - newPointsSize;

            // Move old Y Points to their new position.
            memmove(&yOrigPoints[0], &yOrigPoints[newPointsSize], sizeof(yOrigPoints[0]) * numOrigPointsToKeep);
            memmove(&yPoints[0], &yPoints[newPointsSize], sizeof(yPoints[0]) * numOrigPointsToKeep);
            smartMaxMinYPoints.scrollModeShift(newPointsSize);

            // Copy new Y Points in.
            memcpy(&yOrigPoints[numOrigPointsToKeep], &(*newPointsToUse)[0], sizeof(yOrigPoints[0]) * newPointsSize);
         }
      }

      if(modifySpecificPoints == false)
      {
         oldestPoint_nonScrollModeVersion = sampleStartIndex + newPointsSize;
         plotSize_nonScrollModeVersion = std::max(plotSize_nonScrollModeVersion, oldestPoint_nonScrollModeVersion);
      }

      if(resized == true)
      {
         // yPoints was resized to be bigger, need to update the xPoints.
         fill1DxPoints();
      }

      numPoints = yOrigPoints.size();

      if(updateAsScrollMode == false)
      {
         performMathOnPoints(sampleStartIndex, newPointsSize);
      }
      else
      {
         // New samples are at the end.
         performMathOnPoints(numPoints - newPointsSize, newPointsSize);
      }
      setCurveSamples();
   }
}

void CurveData::UpdateCurveSamples(const dubVect& newXPoints, const dubVect& newYPoints, unsigned int sampleStartIndex, bool modifySpecificPoints)
{
   if(plotDim == E_PLOT_DIM_2D)
   {
      bool updateAsScrollMode = scrollMode && !modifySpecificPoints;

      unsigned int newPointsSize = std::min(newXPoints.size(), newYPoints.size());

      handleNewSampleMsg(sampleStartIndex, newPointsSize);

      if(updateAsScrollMode == false)
      {
         if(xOrigPoints.size() < (sampleStartIndex + newPointsSize))
         {
            xOrigPoints.resize(sampleStartIndex + newPointsSize);
         }

         if(yOrigPoints.size() < (sampleStartIndex + newPointsSize))
         {
            yOrigPoints.resize(sampleStartIndex + newPointsSize);
         }

         memcpy(&xOrigPoints[sampleStartIndex], &newXPoints[0], sizeof(newXPoints[0]) * newPointsSize);
         memcpy(&yOrigPoints[sampleStartIndex], &newYPoints[0], sizeof(newYPoints[0]) * newPointsSize);
      }
      else
      {
         /////////////////////////////
         // Scroll Mode
         /////////////////////////////
         if(numPoints < newPointsSize)
         {
            // Current scroll mode curve size is less than the number of samples in this new curve data message.
            // Resize the curve to fit all the new data.
            xOrigPoints.resize(newPointsSize);
            yOrigPoints.resize(newPointsSize);
            memcpy(&xOrigPoints[0], &newXPoints[0], sizeof(newXPoints[0]) * newPointsSize);
            memcpy(&yOrigPoints[0], &newYPoints[0], sizeof(newYPoints[0]) * newPointsSize);
         }
         else
         {
            int numOrigPoints = yOrigPoints.size(); // Assume num X points and Y points are equal.
            int numOrigPointsToKeep = numOrigPoints - newPointsSize;

            // Move old Points to their new position.
            memmove(&xOrigPoints[0], &xOrigPoints[newPointsSize], sizeof(newXPoints[0]) * numOrigPointsToKeep);
            memmove(&xPoints[0],     &xPoints[newPointsSize],     sizeof(newXPoints[0]) * numOrigPointsToKeep);
            memmove(&yOrigPoints[0], &yOrigPoints[newPointsSize], sizeof(newYPoints[0]) * numOrigPointsToKeep);
            memmove(&yPoints[0],     &yPoints[newPointsSize],     sizeof(newYPoints[0]) * numOrigPointsToKeep);
            smartMaxMinXPoints.scrollModeShift(newPointsSize);
            smartMaxMinYPoints.scrollModeShift(newPointsSize);

            // Copy new Points in.
            memcpy(&xOrigPoints[numOrigPointsToKeep], &newXPoints[0], sizeof(newXPoints[0]) * newPointsSize);
            memcpy(&yOrigPoints[numOrigPointsToKeep], &newYPoints[0], sizeof(newYPoints[0]) * newPointsSize);
         }
      }

      if(modifySpecificPoints == false)
      {
         oldestPoint_nonScrollModeVersion = sampleStartIndex + newPointsSize;
         plotSize_nonScrollModeVersion = std::max(plotSize_nonScrollModeVersion, oldestPoint_nonScrollModeVersion);
      }

      numPoints = std::min(xOrigPoints.size(), yOrigPoints.size()); // These should never be unequal, but take min anyway.

      if(updateAsScrollMode == false)
      {
         performMathOnPoints(sampleStartIndex, newPointsSize);
      }
      else
      {
         // New samples are at the end.
         performMathOnPoints(numPoints - newPointsSize, newPointsSize);
      }
      setCurveSamples();
   }
}

bool CurveData::setSampleRate(double inSampleRate, bool userSpecified)
{
   bool changed = false;

   // Do not overwrite the sample rate if the user has already specified the sample rate via the GUI
   // and this function is being called because a parent curve has changed.
   if( (userSpecified == true) || (sampleRateIsUserSpecified == false) )
   {
      if(sampleRate != inSampleRate)
      {
         sampleRate = inSampleRate;
         if(sampleRate != 0.0)
         {
            samplePeriod = (double)1.0 / sampleRate;
         }
         else
         {
            samplePeriod = 0.0;
         }

         fill1DxPoints();
         performMathOnPoints();
         setCurveSamples();

         changed = true;
      }

      sampleRateIsUserSpecified = sampleRateIsUserSpecified || userSpecified;
   }

   return changed;
}


bool CurveData::setMathOps(tMathOpList& mathOpsIn, eAxis axis)
{
   bool changed = false;
   tMathOpList* axisMathOps;
   if(axis == E_X_AXIS)
      axisMathOps = &mathOpsXAxis;
   else
      axisMathOps = &mathOpsYAxis;

   if(mathOpsIn.size() == axisMathOps->size())
   {
      tMathOpList::iterator inIter = mathOpsIn.begin();
      tMathOpList::iterator thisIter = axisMathOps->begin();

      // Check if the input and destination are the same.
      while(changed == false && inIter != mathOpsIn.end() && thisIter != axisMathOps->end())
      {
         if(inIter->op != thisIter->op || inIter->num != thisIter->num)
         {
            changed = true;
         }
         else
         {
            ++inIter;
            ++thisIter;
         }
      }
   }
   else
   {
      changed = true;
   }

   if(changed)
   {
      *axisMathOps = mathOpsIn;
      performMathOnPoints();
      setCurveSamples();
   }
   return changed;
}


tMathOpList CurveData::getMathOps(eAxis axis)
{
   if(axis == E_X_AXIS)
      return mathOpsXAxis;
   else
      return mathOpsYAxis;
}

void CurveData::performMathOnPoints()
{
   performMathOnPoints(0, yOrigPoints.size());
}

void CurveData::performMathOnPoints(unsigned int sampleStartIndex, unsigned int numSamples)
{
   unsigned int finalNewSampPosition = sampleStartIndex + numSamples;

   unsigned int origXSize = xPoints.size();

   // Copy new samples.
   if(xPoints.size() < finalNewSampPosition)
   {
      xPoints.resize(finalNewSampPosition);
   }
   if(yPoints.size() < finalNewSampPosition)
   {
      yPoints.resize(finalNewSampPosition);
   }
   memcpy(&xPoints[sampleStartIndex], &xOrigPoints[sampleStartIndex], sizeof(xOrigPoints[0]) * numSamples);
   memcpy(&yPoints[sampleStartIndex], &yOrigPoints[sampleStartIndex], sizeof(yOrigPoints[0]) * numSamples);

   // Make sure the generated X axis points are kept up to date.
   if(plotDim == E_PLOT_DIM_1D && origXSize < sampleStartIndex)
   {
      // There is a gap between last old sample and the first new sample. Make sure to copy the
      // generated 1D X axis points in between.
      memcpy(&xPoints[origXSize], &xOrigPoints[origXSize], sizeof(xOrigPoints[0]) * (sampleStartIndex - origXSize));
   }

   // If FM demod and sample rate is specified, convert phase delta to frequency (Hz)
   if(plotType == E_PLOT_TYPE_FM_DEMOD && sampleRate != 0.0)
   {
      // Convert from radians per sample to cycles per second(Hz)
      // The equation is (Rad/Samp) * (Samp/Sec) * (1 Cycle/2Pi Rad) = Cycles/Sec = Hz
      // Rad/Sec is yPoints[i], Samp/Sec is sampleRate, 1 Cycle/2Pi Rad is 1/M_2X_PI

      // Get the (Samp/Sec) * (1 Cycle/2Pi Rad) part
      double multiplier = (double)sampleRate / ((double)M_2X_PI);
      for(unsigned int i = sampleStartIndex; i < finalNewSampPosition; ++i)
      {
         // Convert to Hz.
         yPoints[i] *= multiplier;
      }
   }

   doMathOnCurve(xPoints, mathOpsXAxis, sampleStartIndex, numSamples);
   doMathOnCurve(yPoints, mathOpsYAxis, sampleStartIndex, numSamples);

   if(plotDim == E_PLOT_DIM_1D)
   {
      smartMaxMinYPoints.updateMaxMin(sampleStartIndex, numSamples);
   }
   else
   {
      smartMaxMinXPoints.updateMaxMin(sampleStartIndex, numSamples);
      smartMaxMinYPoints.updateMaxMin(sampleStartIndex, numSamples);
   }

   findMaxMin();

   if(plotDim == E_PLOT_DIM_1D)
   // calculate linear conversion from 1D (xMin .. xMax) to (0 .. NumSamples-1)
   {
       linearXAxisCorrection.m = ((double)(numPoints - 1)) / (maxMin_beforeScale.maxX - maxMin_beforeScale.minX);
       linearXAxisCorrection.b = -linearXAxisCorrection.m * maxMin_beforeScale.minX;
   }
}

void CurveData::doMathOnCurve(dubVect& data, tMathOpList& mathOp, unsigned int sampleStartIndex, unsigned int numSamples)
{
   double* dataBasePtr = &data[sampleStartIndex];
   if(mathOp.size() > 0)
   {
      tMathOpList::iterator mathIter;
      for(unsigned int i = 0; i < numSamples; ++i)
      {
         double* dataIter = &dataBasePtr[i];
         for(mathIter = mathOp.begin(); mathIter != mathOp.end(); ++mathIter)
         {
            switch(mathIter->op)
            {
               case E_ADD:
                  (*dataIter) += mathIter->num;
               break;
               case E_MULTIPLY:
                  (*dataIter) *= mathIter->num;
               break;
               case E_DIVIDE:
                  (*dataIter) /= mathIter->num;
               break;
               case E_SHIFT_UP:
               {
                  if(*dataIter != 0.0)
                  {
                     // Put shift value in double exponent. ( 52 bits mantessa, 11 bits exponent, 1 bit sign)
                     short* shiftValPtr = ((short*)&(*dataIter))+3; // point to 16 MSBs of 64 bit double (where the exponent is)

                     short curExponent = ((*shiftValPtr) & (0x7FF0)) >> 4;
                     curExponent += (short)mathIter->num;
                     *shiftValPtr = (*shiftValPtr & 0x800F) | ((curExponent << 4) & (0x7FF0));
                  }
               }
               break;
               case E_SHIFT_DOWN:
               {
                  if(*dataIter != 0.0)
                  {
                     // Put shift value in double exponent. ( 52 bits mantessa, 11 bits exponent, 1 bit sign)
                     short* shiftValPtr = ((short*)&(*dataIter))+3; // point to 16 MSBs of 64 bit double (where the exponent is)

                     short curExponent = ((*shiftValPtr) & (0x7FF0)) >> 4;
                     curExponent -= (short)mathIter->num;
                     *shiftValPtr = (*shiftValPtr & 0x800F) | ((curExponent << 4) & (0x7FF0));
                  }
               }
               break;
               case E_POWER:
                  (*dataIter) = pow((*dataIter), mathIter->num);
               break;
               case E_LOG:
                  (*dataIter) = log(*dataIter) / mathIter->helperNum;
               break;
               case E_ALOG:
                  (*dataIter) = pow(mathIter->num, *dataIter);
               break;
               case E_MOD:
                  (*dataIter) = fmod(*dataIter, mathIter->num);
                  if((*dataIter) < 0)
                     (*dataIter) += mathIter->num;
               break;
               case E_ABS:
                  (*dataIter) = fabs((*dataIter));
               break;
               case E_ROUND:
                  if(mathIter->num == 0)
                  {
                     // Most of the time this will be just a round to whole integer number, in that case use the fast math.
                     (*dataIter) = round((*dataIter));
                  }
                  else
                  {
                     // Rounding to not an integer, use the slower math.
                     double decimal = pow(10, mathIter->num);
                     (*dataIter) = round((*dataIter) * decimal) / decimal;
                  }
               break;
               case E_ROUND_UP:
                  if(mathIter->num == 0)
                  {
                     // Most of the time this will be just a round to whole integer number, in that case use the fast math.
                     (*dataIter) = ceil((*dataIter));
                  }
                  else
                  {
                     // Rounding to not an integer, use the slower math.
                     double decimal = pow(10, mathIter->num);
                     (*dataIter) = ceil((*dataIter) * decimal) / decimal;
                  }
               break;
               case E_ROUND_DOWN:
                  if(mathIter->num == 0)
                  {
                     // Most of the time this will be just a round to whole integer number, in that case use the fast math.
                     (*dataIter) = floor((*dataIter));
                  }
                  else
                  {
                     // Rounding to not an integer, use the slower math.
                     double decimal = pow(10, mathIter->num);
                     (*dataIter) = floor((*dataIter) * decimal) / decimal;
                  }
               break;
               case E_LIMIT_UPPER:
                  if((*dataIter) > mathIter->num)
                  {
                     (*dataIter) = mathIter->num;
                  }
               break;
               case E_LIMIT_LOWER:
                  if((*dataIter) < mathIter->num)
                  {
                     (*dataIter) = mathIter->num;
                  }
               break;
               case E_SIN:
                  (*dataIter) = sin((*dataIter));
               break;
               case E_COS:
                  (*dataIter) = cos((*dataIter));
               break;
               case E_TAN:
                  (*dataIter) = tan((*dataIter));
               break;
               case E_ASIN:
                  (*dataIter) = asin((*dataIter));
               break;
               case E_ACOS:
                  (*dataIter) = acos((*dataIter));
               break;
               case E_ATAN:
                  (*dataIter) = atan((*dataIter));
               break;
            }
         }
      }
   }

}


bool CurveData::setVisible(bool isVisible)
{
   return setVisibleHidden(isVisible, hidden);
}

bool CurveData::setHidden(bool isHidden)
{
   return setVisibleHidden(visible, isHidden);
}

bool CurveData::setVisibleHidden(bool isVisible, bool isHidden)
{
   bool changed = isVisible != visible || isHidden != hidden;
   if(changed)
   {
      if(isVisible == true && isHidden == false)
      {
         // Display the curve on the parent plot.
         if(attached == false)
         {
            curve->attach(m_parentPlot);
            attached = true;
         }
      }
      else
      {
         // Do not display the curve on the parent plot.
         if(attached == true)
         {
            curve->detach();
            attached = false;
         }
      }

      visible = isVisible;
      hidden = isHidden;
   }
   return changed;
}

void CurveData::setCurveAppearance(CurveAppearance curveAppearance)
{
   appearance = curveAppearance;

   curve->setPen(appearance.color, appearance.width);
   curve->setStyle(appearance.style);

   // The samples sent to the QWT plot API can change based on curve appearance.
   // Since the curve appearance is changing, resend the samples.
   setCurveDataGuiPoints(false);

   m_parentPlot->replot();
}

CurveAppearance CurveData::getCurveAppearance()
{
   return appearance;
}

unsigned int CurveData::removeInvalidPoints()
{
   unsigned int numPointsInCurve = numPoints;
   if( !isDoubleValid(maxMin_beforeScale.maxX) || !isDoubleValid(maxMin_beforeScale.maxY) ||
       !isDoubleValid(maxMin_beforeScale.minX) || !isDoubleValid(maxMin_beforeScale.minY) )
   {
      // One of the values is invalid. Need to remove invalid points.
      unsigned int writeIndex = 0;
      for(unsigned int readIndex = 0; readIndex < numPoints; ++readIndex)
      {
         if( isDoubleValid(xPoints[readIndex]) && isDoubleValid(yPoints[readIndex]) )
         {
            // Point is valid, keep it.
            xPoints[writeIndex] = xPoints[readIndex];
            yPoints[writeIndex] = yPoints[readIndex];
            ++writeIndex;
         }
      }
      if(writeIndex < numPoints)
      {
         numPointsInCurve = writeIndex;
         xPoints.resize(numPointsInCurve);
         yPoints.resize(numPointsInCurve);
      }
   }

   return numPointsInCurve;
}

void CurveData::storeLastMsgStats(const UnpackPlotMsg* data)
{
   lastMsgIpAddr = data->m_ipAddr;
   lastMsgXAxisType = data->m_xAxisDataType;
   lastMsgYAxisType = data->m_yAxisDataType;
}

// This function will be called whenever a new sample message has been received.
void CurveData::handleNewSampleMsg(unsigned int sampleStartIndex, unsigned int numSamples)
{
   maxNumPointsFromPlotMsg = std::max(maxNumPointsFromPlotMsg, sampleStartIndex + numSamples);
   sampleRateCalculator.newSamples(numSamples);
}

void CurveData::setPointValue(unsigned int index, double value)
{
   setPointValue(index, value, value);
}

void CurveData::setPointValue(unsigned int index, double xValue, double yValue)
{
   if(index < getNumPoints())
   {
      if(plotDim == E_PLOT_DIM_1D)
      {
         dubVect yPoints;
         yPoints.push_back(yValue);
         UpdateCurveSamples(yPoints, index, true);
      }
      else
      {
         dubVect xPoints;
         dubVect yPoints;
         xPoints.push_back(xValue);
         yPoints.push_back(yValue);
         UpdateCurveSamples(xPoints, yPoints, index, true);
      }
   }
}

void CurveData::specAn_reset()
{
   fftSpecAn.reset();
}

void CurveData::specAn_setTraceType(fftSpecAnFunc::eFftSpecAnTraceType newTraceType)
{
   if(newTraceType != fftSpecAnTraceType)
   {
      specAn_reset();
   }
   fftSpecAnTraceType = newTraceType;
}

void CurveData::specAn_setAvgSize(int newAvgSize)
{
   fftSpecAn.setAvgSize(newAvgSize);
}
