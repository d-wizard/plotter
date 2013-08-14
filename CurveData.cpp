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

CurveData::CurveData( QwtPlot *parentPlot,
                      const QString& curveName,
                      const dubVect& newYPoints,
                      const QColor& curveColor):
   m_parentPlot(parentPlot),
   yPoints(newYPoints),
   plotType(E_PLOT_TYPE_1D),
   color(curveColor),
   curve(new QwtPlotCurve(curveName))
{
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
   xPoints(newXPoints),
   yPoints(newYPoints),
   plotType(E_PLOT_TYPE_2D),
   color(curveColor),
   curve(new QwtPlotCurve(curveName))
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
   for(unsigned int i = 0; i < xPoints.size(); ++i)
   {
      xPoints[i] = (double)i;
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

ePlotType CurveData::getPlotType()
{
   return plotType;
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
   return displayed;
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
