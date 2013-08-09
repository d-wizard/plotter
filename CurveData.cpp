/* Copyright 2013 Dan Williams. All Rights Reserved.
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

CurveData::CurveData(const QString& curveName, const dubVect& newYPoints, const QColor& curveColor):
   plotType(E_PLOT_TYPE_1D),
   curve(new QwtPlotCurve(curveName)),
   color(curveColor),
   yPoints(newYPoints)
{
   init();
   numPoints = yPoints.size();
   fill1DxPoints();
   findMaxMin();
   initCurve();
}

CurveData::CurveData(const QString& curveName, const dubVect& newXPoints, const dubVect& newYPoints, const QColor& curveColor):
   plotType(E_PLOT_TYPE_2D),
   curve(new QwtPlotCurve(curveName)),
   color(curveColor),
   xPoints(newXPoints),
   yPoints(newYPoints)
{
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

QwtPlotCurve* CurveData::getCurve()
{
   return curve;
}

const double* CurveData::getXPoints()
{
   return &xPoints[0];
}

const double* CurveData::getYPoints()
{
   return &yPoints[0];
}

void CurveData::fill1DxPoints()
{
   xPoints.resize(yPoints.size());
   for(unsigned int i = 0; i < xPoints.size(); ++i)
   {
      xPoints[i] = (double)i;
   }
}

void CurveData::findMaxMin()
{
   int vectSize = yPoints.size();
   if(plotType == E_PLOT_TYPE_1D)
   {
      maxMin.minX = 0;
      maxMin.maxX = vectSize-1;
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
   else if(plotType == E_PLOT_TYPE_2D)
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

void CurveData::initCurve()
{
   curve->setPen(color);
   setCurveSamples();
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

maxMinXY CurveData::GetMaxMinXYOfCurve()
{
   maxMinXY retVal;
   retVal.maxX = curve->maxXValue();
   retVal.minX = curve->minXValue();
   retVal.maxY = curve->maxYValue();
   retVal.minY = curve->minYValue();
   return retVal;
}

maxMinXY CurveData::GetMaxMinXYOfData()
{
   return maxMin;
}

QString CurveData::GetCurveTitle()
{
   return curve->title().text();
}

void CurveData::SetNewCurveSamples(dubVect& newYPoints)
{
   plotType = E_PLOT_TYPE_1D;
   yPoints = newYPoints;
   numPoints = yPoints.size();
   fill1DxPoints();
   findMaxMin();
   setCurveSamples();
}
void CurveData::SetNewCurveSamples(dubVect& newXPoints, dubVect& newYPoints)
{
   plotType = E_PLOT_TYPE_2D;
   xPoints = newXPoints;
   yPoints = newYPoints;
   numPoints = yPoints.size();
   findMaxMin();
   setCurveSamples();
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

void CurveData::init()
{
   numPoints = 0;
   pointLabel = NULL;
   curveAction = NULL;
   mapper = NULL;
   displayed = false;
   
   resetNormalizeFactor();
}

