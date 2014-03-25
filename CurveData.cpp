/* Copyright 2013 - 2014 Dan Williams. All Rights Reserved.
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
#include "CurveData.h"
#include "fftHelper.h"

CurveData::CurveData( QwtPlot *parentPlot,
                      const QString& curveName,
                      const ePlotType newPlotType,
                      const dubVect &newYPoints,
                      const QColor &curveColor):
   m_parentPlot(parentPlot),
   yOrigPoints(newYPoints),
   plotDim(E_PLOT_DIM_1D),
   plotType(newPlotType),
   color(curveColor),
   curve(new QwtPlotCurve(curveName))
{
   performMathOnPoints();
   init();
   numPoints = yPoints.size();
   fill1DxPoints();
   findMaxMin();
   initCurve();
   attach();
}

CurveData::CurveData( QwtPlot* parentPlot,
                      const QString& curveName,
                      const dubVect& newXPoints,
                      const dubVect& newYPoints,
                      const QColor& curveColor):
   m_parentPlot(parentPlot),
   xOrigPoints(newXPoints),
   yOrigPoints(newYPoints),
   plotDim(E_PLOT_DIM_2D),
   plotType(E_PLOT_TYPE_2D),
   color(curveColor),
   curve(new QwtPlotCurve(curveName))
{
   performMathOnPoints();
   init();
   if(xPoints.size() > yPoints.size())
   {
      xPoints.resize(yPoints.size());
   }
   else if(xPoints.size() < yPoints.size())
   {
      yPoints.resize(xPoints.size());
   }
   
   numPoints = yPoints.size();
   findMaxMin();
   initCurve();
   attach();
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

   samplePeriod = 1.0;

   resetNormalizeFactor();
}

void CurveData::initCurve()
{
   curve->setPen(color);
   setCurveSamples();
}

void CurveData::fill1DxPoints()
{
   xPoints.resize(yPoints.size());
   switch(plotType)
   {
      case E_PLOT_TYPE_1D:
      case E_PLOT_TYPE_REAL_FFT:
      {
         if(samplePeriod == 0.0 || samplePeriod == 1.0)
         {
             for(unsigned int i = 0; i < xPoints.size(); ++i)
             {
                xPoints[i] = (double)i;
             }
         }
         else
         {
             for(unsigned int i = 0; i < xPoints.size(); ++i)
             {
                xPoints[i] = (double)i * samplePeriod;
             }
         }
      }
      break;

      case E_PLOT_TYPE_COMPLEX_FFT:
      {
         getFFTXAxisValues(xPoints, yPoints.size());
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

QColor CurveData::getColor()
{
   return color;
}

ePlotDim CurveData::getPlotDim()
{
   return plotDim;
}

unsigned int CurveData::getNumPoints()
{
   return numPoints;
}

maxMinXY CurveData::getMaxMinXYOfCurve()
{
   maxMinXY retVal;
   retVal.maxX = curve->maxXValue();
   retVal.minX = curve->minXValue();
   retVal.maxY = curve->maxYValue();
   retVal.minY = curve->minYValue();
   return retVal;
}

maxMinXY CurveData::getMaxMinXYOfData()
{
   return maxMin;
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
   int vectSize = yPoints.size();
   if(plotDim == E_PLOT_DIM_1D)
   {
      maxMin.minX = xPoints[0];
      maxMin.maxX = xPoints[vectSize-1];
      maxMin.minY = yPoints[0];
      maxMin.maxY = yPoints[0];
      
      for(int i = 1; i < vectSize; ++i)
      {
         if(maxMin.minY > yPoints[i])
         {
            maxMin.minY = yPoints[i];
         }
         if(maxMin.maxY < yPoints[i])
         {
            maxMin.maxY = yPoints[i];
         }
      }
   }
   else if(plotDim == E_PLOT_DIM_2D)
   {
      maxMin.minX = xPoints[0];
      maxMin.maxX = xPoints[0];
      maxMin.minY = yPoints[0];
      maxMin.maxY = yPoints[0];
      
      for(int i = 1; i < vectSize; ++i)
      {
         if(maxMin.minX > xPoints[i])
         {
            maxMin.minX = xPoints[i];
         }
         if(maxMin.maxX < xPoints[i])
         {
            maxMin.maxX = xPoints[i];
         }
         
         if(maxMin.minY > yPoints[i])
         {
            maxMin.minY = yPoints[i];
         }
         if(maxMin.maxY < yPoints[i])
         {
            maxMin.maxY = yPoints[i];
         }
      }
   }
}

void CurveData::setNormalizeFactor(maxMinXY desiredScale)
{
   if(desiredScale.maxX == maxMin.maxX && desiredScale.minX == maxMin.minX)
   {
      normFactor.xAxis.m = 1.0;
      normFactor.xAxis.b = 0.0;
      xNormalized = false;
   }
   else
   {
      normFactor.xAxis.m = (desiredScale.maxX - desiredScale.minX) / (maxMin.maxX - maxMin.minX);
      normFactor.xAxis.b = desiredScale.maxX - (normFactor.xAxis.m * maxMin.maxX);
      xNormalized = true;
   }
   if(desiredScale.maxY == maxMin.maxY && desiredScale.minY == maxMin.minY)
   {
      normFactor.yAxis.m = 1.0;
      normFactor.yAxis.b = 0.0;
      yNormalized = false;
   }
   else
   {
      normFactor.yAxis.m = (desiredScale.maxY - desiredScale.minY) / (maxMin.maxY - maxMin.minY);
      normFactor.yAxis.b = desiredScale.maxY - (normFactor.yAxis.m * maxMin.maxY);
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
   dubVect normX;
   dubVect normY;
   if(xNormalized)
   {
      normX.resize(numPoints);
      for(unsigned int i = 0; i < numPoints; ++i)
      {
         normX[i] = (normFactor.xAxis.m * xPoints[i]) + normFactor.xAxis.b;
      }
   }
   if(yNormalized)
   {
      normY.resize(numPoints);
      for(unsigned int i = 0; i < numPoints; ++i)
      {
         normY[i] = (normFactor.yAxis.m * yPoints[i]) + normFactor.yAxis.b;
      }
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
}

void CurveData::ResetCurveSamples(dubVect& newYPoints)
{
   plotDim = E_PLOT_DIM_1D;
   yOrigPoints = newYPoints;
   performMathOnPoints();
   numPoints = yOrigPoints.size();
   fill1DxPoints();
   findMaxMin();
   setCurveSamples();
}
void CurveData::ResetCurveSamples(dubVect& newXPoints, dubVect& newYPoints)
{
   plotDim = E_PLOT_DIM_2D;
   xOrigPoints = newXPoints;
   yOrigPoints = newYPoints;
   performMathOnPoints();
   numPoints = std::min(xPoints.size(), yPoints.size());
   if(xPoints.size() > numPoints)
   {
      xPoints.resize(numPoints);
   }
   if(yPoints.size() > numPoints)
   {
      yPoints.resize(numPoints);
   }
   findMaxMin();
   setCurveSamples();
}


void CurveData::UpdateCurveSamples(dubVect& newYPoints, unsigned int sampleStartIndex)
{
   if(plotDim == E_PLOT_DIM_1D)
   {
      bool resized = false;
      if(yPoints.size() < (sampleStartIndex + newYPoints.size()))
      {
         resized = true;
         yPoints.resize(sampleStartIndex + newYPoints.size());
      }
      memcpy(&yPoints[sampleStartIndex], &newYPoints[0], sizeof(newYPoints[0]) * newYPoints.size());

      if(resized == true)
      {
         // yPoints was resized to be bigger, need to update the xPoints.
         fill1DxPoints();
      }

      numPoints = yPoints.size();

      findMaxMin();
      setCurveSamples();
   }
}

void CurveData::UpdateCurveSamples(dubVect& newXPoints, dubVect& newYPoints, unsigned int sampleStartIndex)
{
   if(plotDim == E_PLOT_DIM_2D)
   {
      unsigned int newPointsSize = std::min(newXPoints.size(), newYPoints.size());

      if(xPoints.size() < (sampleStartIndex + newPointsSize))
      {
         xPoints.resize(sampleStartIndex + newPointsSize);
      }
      memcpy(&xPoints[sampleStartIndex], &newXPoints[0], sizeof(newXPoints[0]) * newPointsSize);

      if(yPoints.size() < (sampleStartIndex + newPointsSize))
      {
         yPoints.resize(sampleStartIndex + newPointsSize);
      }
      memcpy(&yPoints[sampleStartIndex], &newYPoints[0], sizeof(newYPoints[0]) * newPointsSize);

      numPoints = std::min(xPoints.size(), yPoints.size()); // These should never be unequal, but take min anyway.

      findMaxMin();
      setCurveSamples();
   }
}

void CurveData::setMath(double sampleRate)
{
   samplePeriod = (double)1.0 / sampleRate;
}

void CurveData::performMathOnPoints()
{
   xPoints = xOrigPoints;
   yPoints = yOrigPoints;
}



