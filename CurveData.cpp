/* Copyright 2013 - 2017 Dan Williams. All Rights Reserved.
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
   lastMsgYAxisType(E_INVALID_DATA_TYPE)
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

   if(plotDim == E_PLOT_DIM_1D)
   {
      fill1DxPoints();
   }

   performMathOnPoints();
   initCurve();
   attach();
   storeLastMsgStats(data);
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
   displayed = false;
   hidden = false;

   samplePeriod = 0.0;
   sampleRate = 0.0;
   sampleRateIsUserSpecified = false;

   linearXAxisCorrection.m = 1.0;
   linearXAxisCorrection.b = 0.0;

   resetNormalizeFactor();
}

void CurveData::initCurve()
{
   curve->setPen(appearance.color);
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

void CurveData::setNumPoints(unsigned int newNumPointsSize, bool scrollMode)
{
   if(scrollMode)
   {
      bool addingPoints = numPoints < newNumPointsSize;
      unsigned int delta = addingPoints ? newNumPointsSize - numPoints : numPoints - newNumPointsSize;

      numPoints = newNumPointsSize;
      if(addingPoints)
      {
         // Increasing curve size, add zero's to beginning.
         yOrigPoints.insert(yOrigPoints.begin(), delta, 0);
         if(plotDim == E_PLOT_DIM_2D)
            xOrigPoints.insert(xOrigPoints.begin(), delta, 0);
      }
      else
      {
         // Decreasing curve size, remove samples from beginning.
         yOrigPoints.erase(yOrigPoints.begin(), yOrigPoints.begin()+delta);
         if(plotDim == E_PLOT_DIM_2D)
            xOrigPoints.erase(yOrigPoints.begin(), xOrigPoints.begin()+delta);
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

QString CurveData::getCurveTitle()
{
   return curve->title().text();
}

bool CurveData::isDisplayed()
{
   return displayed == true && hidden == false;
}

void CurveData::attach()
{
   if(displayed == false)
   {
      curve->attach(m_parentPlot);
      displayed = true;
   }
}

void CurveData::detach()
{
   if(displayed == true)
   {
      curve->detach();
      displayed = false;
   }
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

void CurveData::setNormalizeFactor(maxMinXY desiredScale)
{
   if(desiredScale.maxX == maxMin_beforeScale.maxX && desiredScale.minX == maxMin_beforeScale.minX)
   {
      normFactor.xAxis.m = 1.0;
      normFactor.xAxis.b = 0.0;
      xNormalized = false;
   }
   else
   {
      normFactor.xAxis.m = (desiredScale.maxX - desiredScale.minX) / (maxMin_beforeScale.maxX - maxMin_beforeScale.minX);
      normFactor.xAxis.b = desiredScale.maxX - (normFactor.xAxis.m * maxMin_beforeScale.maxX);
      xNormalized = true;
   }
   if(desiredScale.maxY == maxMin_beforeScale.maxY && desiredScale.minY == maxMin_beforeScale.minY)
   {
      normFactor.yAxis.m = 1.0;
      normFactor.yAxis.b = 0.0;
      yNormalized = false;
   }
   else
   {
      normFactor.yAxis.m = (desiredScale.maxY - desiredScale.minY) / (maxMin_beforeScale.maxY - maxMin_beforeScale.minY);
      normFactor.yAxis.b = desiredScale.maxY - (normFactor.yAxis.m * maxMin_beforeScale.maxY);
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
   
   if(xNormalized && yNormalized)
   {
      curve->setSamples( &normX[0],
                         &normY[0],
                         numPoints);
   }
   else if(xNormalized)
   {
      curve->setSamples( &normX[0],
                         &yPoints[0],
                         numPoints);
   }
   else if(yNormalized)
   {
      curve->setSamples( &xPoints[0],
                         &normY[0],
                         numPoints);
   }
   else
   {
      curve->setSamples( &xPoints[0],
                         &yPoints[0],
                         numPoints);
   }

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
   performMathOnPoints();
   setCurveSamples();
   storeLastMsgStats(data);
}

void CurveData::UpdateCurveSamples(const UnpackPlotMsg* data, bool scrollMode)
{
   if(plotDim == plotActionToPlotDim(data->m_plotAction))
   {
      if(plotDim == E_PLOT_DIM_1D)
      {
         UpdateCurveSamples(data->m_yAxisValues, data->m_sampleStartIndex, scrollMode);
      }
      else
      {
         UpdateCurveSamples(data->m_xAxisValues, data->m_yAxisValues, data->m_sampleStartIndex, scrollMode);
      }
      storeLastMsgStats(data);
   }
}

void CurveData::UpdateCurveSamples(const dubVect& newYPoints, unsigned int sampleStartIndex, bool scrollMode)
{
   if(plotDim == E_PLOT_DIM_1D)
   {
      int newPointsSize = newYPoints.size();
      bool resized = false;

      if(scrollMode == false)
      {
         // Check if the current size can handle the new samples.
         if(yOrigPoints.size() < (sampleStartIndex + newPointsSize))
         {
            resized = true;
            yOrigPoints.resize(sampleStartIndex + newPointsSize);
         }
         memcpy(&yOrigPoints[sampleStartIndex], &newYPoints[0], sizeof(newYPoints[0]) * newPointsSize);
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
            yOrigPoints = newYPoints;
         }
         else
         {
            int numOrigPointsToKeep = yOrigPoints.size() - newPointsSize;

            // Move old Y Points to their new position.
            memmove(&yOrigPoints[0], &yOrigPoints[newPointsSize], sizeof(newYPoints[0]) * numOrigPointsToKeep);
            memmove(&yPoints[0], &yPoints[newPointsSize], sizeof(newYPoints[0]) * numOrigPointsToKeep);
            smartMaxMinYPoints.scrollModeShift(newPointsSize);

            // Copy new Y Points in.
            memcpy(&yOrigPoints[numOrigPointsToKeep], &newYPoints[0], sizeof(newYPoints[0]) * newPointsSize);
         }
      }

      if(resized == true)
      {
         // yPoints was resized to be bigger, need to update the xPoints.
         fill1DxPoints();
      }

      numPoints = yOrigPoints.size();

      if(scrollMode == false)
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

void CurveData::UpdateCurveSamples(const dubVect& newXPoints, const dubVect& newYPoints, unsigned int sampleStartIndex, bool scrollMode)
{
   if(plotDim == E_PLOT_DIM_2D)
   {
      unsigned int newPointsSize = std::min(newXPoints.size(), newYPoints.size());

      if(scrollMode == false)
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


      numPoints = std::min(xOrigPoints.size(), yOrigPoints.size()); // These should never be unequal, but take min anyway.

      if(scrollMode == false)
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
               case E_MOD:
                  (*dataIter) = fmod(*dataIter, mathIter->num);
                  if((*dataIter) < 0)
                     (*dataIter) += mathIter->num;
               break;
               case E_ABS:
                  (*dataIter) = fabs((*dataIter));
               break;
            }
         }
      }
   }

}


bool CurveData::setHidden(bool isHidden)
{
   bool changed = false;
   if(isHidden != hidden)
   {
      if(isHidden && displayed)
      {
         curve->detach();
         changed = true;
      }
      else if(!isHidden && displayed)
      {
         curve->attach(m_parentPlot);
         changed = true;
      }
      hidden = isHidden;
   }
   return changed;
}

void CurveData::setCurveAppearance(CurveAppearance curveAppearance)
{
   appearance = curveAppearance;
   qreal width = appearance.style == QwtPlotCurve::Dots ? 3.0 : 0.0;

   curve->setPen(appearance.color, width);
   curve->setStyle(appearance.style);
   m_parentPlot->replot();
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

