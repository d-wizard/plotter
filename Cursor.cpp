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
#include "Cursor.h"
#include <math.h>


Cursor::Cursor():
   m_xPoint(0),
   m_yPoint(0),
   m_parentCurve(NULL),
   m_pointIndex(0),
   isAttached(false),
   outOfRange(false),
   m_plot(NULL),
   m_curve(NULL)
{
   m_curve = new QwtPlotCurve("");
}

Cursor::Cursor(QwtPlot* plot, QwtSymbol::Style style):
   m_xPoint(0),
   m_yPoint(0),
   m_parentCurve(NULL),
   m_pointIndex(0),
   isAttached(false),
   outOfRange(false),
   m_plot(plot),
   m_curve(NULL),
   m_style(style)
{
   m_curve = new QwtPlotCurve("");
}


Cursor::~Cursor()
{
   if(m_curve != NULL)
   {
      delete m_curve;
      m_curve = NULL;
   }
}


void Cursor::setCurve(CurveData* curve)
{
   m_parentCurve = curve;
   if(isAttached && m_pointIndex >= m_parentCurve->getNumPoints())
   {
      outOfRange = true;
      hideCursor();
      isAttached = true;
   }
}


CurveData* Cursor::getCurve()
{
   return m_parentCurve;
}

double Cursor::determineClosestPointIndex(QPointF pos, maxMinXY maxMin, double displayRatio)
{
   ePlotDim parentCurveDim = m_parentCurve->getPlotDim();

   // As long as the parent curve is 1D and not normalized, we can speed up the search with some simple math.
   bool useFast1dSearch = parentCurveDim == E_PLOT_DIM_1D && !m_parentCurve->isXNormalized();

   double xPos = pos.x();
   double yPos = pos.y();

   const double* xPoints = m_parentCurve->isXNormalized() ? m_parentCurve->getNormXPoints() : m_parentCurve->getXPoints();
   const double* yPoints = m_parentCurve->isYNormalized() ? m_parentCurve->getNormYPoints() : m_parentCurve->getYPoints();

   // The data will not be displayed on the plot 1:1, need to adjust
   // delta calculation to make the x and y delta ratio 1:1
   double width = (maxMin.maxX - maxMin.minX);
   double height = (maxMin.maxY - maxMin.minY) * displayRatio;
   double inverseWidth = 1.0/width;
   double inverseHeight = 1.0/height;

   // Initialize for 2D plot
   int minPointIndex = 0;

   // For 1D curves, we can start with a good guess as to which point will be the closest.
   if(useFast1dSearch)
   {
      // Since 1D plot, assume the X position the user selected is close to the curve,
      // convert input position to actual X position.
      tLinear inXtoPlotX = m_parentCurve->getLinearXAxisCorrection();
      minPointIndex = (int)( ((pos.x() * inXtoPlotX.m) + inXtoPlotX.b) + 0.5);
      if(minPointIndex < 0)
         minPointIndex = 0;
      else if(minPointIndex >= (int)m_parentCurve->getNumPoints())
         minPointIndex = m_parentCurve->getNumPoints() - 1;
   }

   double xDelta = fabs(xPoints[minPointIndex] - xPos)*inverseWidth;
   double yDelta = fabs(yPoints[minPointIndex] - yPos)*inverseHeight;
   double minDist = std::numeric_limits<double>::max(); // Initialize to maximum possible double value.
   bool validMinDist = isDoubleValid(xDelta) && isDoubleValid(yDelta);

   if(validMinDist)
   {
      minDist = sqrt((xDelta*xDelta) + (yDelta*yDelta)); // Only set the actual vaulue if both x and y points are valid.
   }

   // Initialize for 2D plot
   int startIndex = validMinDist ? 1 : 0;
   int endIndex = m_parentCurve->getNumPoints();

   // For 1D curves, we can reduce the number of points to search over.
   if(useFast1dSearch && validMinDist)
   {
      int roundDownMinDist = (int)(minDist * width * m_parentCurve->getLinearXAxisCorrection().m) + 1;
      startIndex = minPointIndex - roundDownMinDist;
      endIndex = minPointIndex + roundDownMinDist;
      if(startIndex < 0)
      {
         startIndex = 0;
      }
      if(endIndex > (int)m_parentCurve->getNumPoints())
      {
         endIndex = (int)m_parentCurve->getNumPoints();
      }
   }

   for(int i = startIndex; i < endIndex; ++i)
   {
      xDelta = fabs(xPoints[i] - xPos)*inverseWidth;
      yDelta = fabs(yPoints[i] - yPos)*inverseHeight;

      // For 2D plots, never select 'Not a Number' points (only need to do this for 2D because the extra 1D specific code
      // above seems to work pretty well with invalid points).
      if(parentCurveDim != E_PLOT_DIM_1D)
      {
         // Check if we still need to find a valid 'minDist' value. Only set 'minDist' if the x and y deltas are valid.
         if(!isDoubleValid(minDist) && isDoubleValid(xDelta) && isDoubleValid(yDelta))
         {
            minDist = sqrt((xDelta*xDelta) + (yDelta*yDelta));
            minPointIndex = i;
         }
      }

      // Don't do sqrt if its already known that it won't be smaller than minDist
      if(xDelta < minDist && yDelta < minDist)
      {
         // For 2D plots, only update 'minDist' if the x/y deltas are valid.
         bool invalid2dPoint = parentCurveDim != E_PLOT_DIM_1D && (!isDoubleValid(xDelta) || !isDoubleValid(yDelta));

         if(!invalid2dPoint)
         {
            double curPointDist = sqrt((xDelta*xDelta) + (yDelta*yDelta));
            if(curPointDist < minDist)
            {
               minDist = curPointDist;
               minPointIndex = i;
            }
         }
      }
   }

   m_pointIndex = minPointIndex;

   return minDist;
}

bool Cursor::peakSearch(maxMinXY searchWindow)
{
   bool validPeakFound = false;
   int peakIndex = -1;
   double maxValue = 0;

   if(m_parentCurve != NULL)
   {
      const double* xPoints = m_parentCurve->isXNormalized() ? m_parentCurve->getNormXPoints() : m_parentCurve->getXPoints();
      const double* yPoints = m_parentCurve->isYNormalized() ? m_parentCurve->getNormYPoints() : m_parentCurve->getYPoints();
      int numPoints = (int)m_parentCurve->getNumPoints();

      for(int i = 0; i < numPoints; ++i)
      {
         // Check if the point is within the search window (search window is probably the current zoom).
         if( yPoints[i] <= searchWindow.maxY && yPoints[i] >= searchWindow.minY &&
             xPoints[i] <= searchWindow.maxX && xPoints[i] >= searchWindow.minX )
         {
            if(!validPeakFound)
            {
               validPeakFound = true;
               peakIndex = i;
               maxValue = yPoints[i];
            }
            else if(yPoints[i] > maxValue)
            {
               peakIndex = i;
               maxValue = yPoints[i];
            }
         }
      }
   }

   if(validPeakFound)
   {
      m_pointIndex = peakIndex;
   }
   return validPeakFound;
}

void Cursor::showCursor()
{
   hideCursor();
   if(m_pointIndex < m_parentCurve->getNumPoints())
   {
      // Make sure to check if cursor point needs to account for normalization.
      const double* xPoints = m_parentCurve->isXNormalized() ? m_parentCurve->getNormXPoints() : m_parentCurve->getXPoints();
      const double* yPoints = m_parentCurve->isYNormalized() ? m_parentCurve->getNormYPoints() : m_parentCurve->getYPoints();

      // Store off the non-normalized seleted point.
      m_xPoint = m_parentCurve->getXPoints()[m_pointIndex];
      m_yPoint = m_parentCurve->getYPoints()[m_pointIndex];

      // QwtPlotCurve wants to be called with new and deletes the symbol on its own.
      m_curve->setSymbol( new QwtSymbol( m_style,
                                         QBrush(m_parentCurve->getColor()),
                                         QPen( Qt::black, 1),
                                         QSize(9, 9) ) );

      m_curve->setSamples( &xPoints[m_pointIndex], &yPoints[m_pointIndex], 1);

      outOfRange = false;

      m_curve->attach(m_plot);
   }
   else
   {
      outOfRange = true;
   }
   isAttached = true;
}

void Cursor::hideCursor()
{
   if(isAttached)
   {
      m_curve->detach();
      isAttached = false;
   }
}
