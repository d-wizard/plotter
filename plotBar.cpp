/* Copyright 2016, 2019 Dan Williams. All Rights Reserved.
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
#include "plotBar.h"


plotBar::plotBar( QwtPlot* parentPlot,
                  QString barName,
                  eAxis barAxis,
                  QColor color,
                  qreal width,
                  Qt::PenStyle penStyle,
                  QwtPlotCurve::CurveStyle curveStyle)
{
   init( parentPlot,
         barName,
         barAxis,
         color,
         width,
         penStyle,
         curveStyle,
         2);
}


plotBar::~plotBar()
{
   delete m_curve;
}

void plotBar::init( QwtPlot* parentPlot,
                    QString barName,
                    eAxis barAxis,
                    QColor color, 
                    qreal width, 
                    Qt::PenStyle penStyle,
                    QwtPlotCurve::CurveStyle curveStyle,
                    double barSelectThresh_pixels)
{
   m_parentPlot = parentPlot;
   
   m_curve = new QwtPlotCurve(barName);
   m_curve->setPen(color, width, penStyle);
   m_curve->setStyle(curveStyle);
   

   m_barAxis = barAxis;
   m_isVisable = false;
   
   m_barSelectThresh_pixels = barSelectThresh_pixels;

   m_curBarPos = 0;
   m_zoomDim.maxX = 0;
   m_zoomDim.minX = 0;
   m_zoomDim.maxY = 0;
   m_zoomDim.minY = 0;
   m_plotDim = m_zoomDim; // Set all plot dimensions to 0.

}


void plotBar::show()
{
   boundBarPosition(m_curBarPos);

   m_curve->setSamples(m_xPoints, m_yPoints, 2);
   
   m_curve->attach(m_parentPlot);
   
   m_isVisable = true;
}

void plotBar::hide()
{
   m_curve->detach();
   m_isVisable = false;
}

bool plotBar::isVisable()
{
   return m_isVisable;
}

void plotBar::moveToFront()
{
   if(m_isVisable)
   {
      m_curve->detach();
      m_curve->attach(m_parentPlot);
   }
}

bool plotBar::isSelectionCloseToBar( const QPointF& pos, 
                            const maxMinXY& zoomDim, 
                            const int canvasWidth_pixels,
                            const int canvasHeight_pixels,
                            double* delta)
{
   m_zoomDim = zoomDim;

   double selectionPosition;
   double numPixels;
   double zoomMin;
   double zoomMax;
   double zoomLen;

   if(m_barAxis == E_X_AXIS)
   {
      selectionPosition = pos.x();
      numPixels = canvasWidth_pixels;
      zoomMin = m_zoomDim.minX;
      zoomMax = m_zoomDim.maxX;
   }
   else
   {
      selectionPosition = pos.y();
      numPixels = canvasHeight_pixels;
      zoomMin = m_zoomDim.minY;
      zoomMax = m_zoomDim.maxY;
   }
   zoomLen = zoomMax - zoomMin;

   double selectionDelta_pixels = fabs(selectionPosition - m_curBarPos) * numPixels / zoomLen;

   *delta = selectionDelta_pixels;

   return selectionDelta_pixels <= m_barSelectThresh_pixels;
}

void plotBar::moveBar(const QPointF& pos)
{
   double newBarPos = m_barAxis == E_X_AXIS ? pos.x() : pos.y();
   boundBarPosition(newBarPos);
   m_curve->setSamples(m_xPoints, m_yPoints, 2);
}

void plotBar::updateZoomDim(const maxMinXY& zoomDim)
{
   m_zoomDim = zoomDim;
   setBarEndPoints();
   m_curve->setSamples(m_xPoints, m_yPoints, 2);
}

void plotBar::updatePlotDim(const maxMinXY& plotDim)
{
   m_plotDim = plotDim;
   setBarEndPoints();
   m_curve->setSamples(m_xPoints, m_yPoints, 2);
}

void plotBar::setBarEndPoints()
{
   // Update bar end points to match max of plot/zoom dimensions.
   maxMinXY limit = m_plotDim;
   if(m_barAxis == E_X_AXIS)
   {
      if(m_zoomDim.minY < limit.minY)
         limit.minY = m_zoomDim.minY;
      if(m_zoomDim.maxY > limit.maxY)
         limit.maxY = m_zoomDim.maxY;

      m_yPoints[0] = limit.minY;
      m_yPoints[1] = limit.maxY;
   }
   else
   {
      if(m_zoomDim.minX < limit.minX)
         limit.minX = m_zoomDim.minX;
      if(m_zoomDim.maxX > limit.maxX)
         limit.maxX = m_zoomDim.maxX;

      m_xPoints[0] = limit.minX;
      m_xPoints[1] = limit.maxX;
   }
}

void plotBar::boundBarPosition(double newBarPos)
{
   setBarEndPoints();
   if(m_barAxis == E_X_AXIS)
   {
      if(newBarPos < m_zoomDim.minX)
         newBarPos = m_zoomDim.minX;
      else if(newBarPos > m_zoomDim.maxX)
         newBarPos = m_zoomDim.maxX;
      m_xPoints[0] = newBarPos;
      m_xPoints[1] = newBarPos;
      
      m_curBarPos = newBarPos;
   }
   else
   {
      if(newBarPos < m_zoomDim.minY)
         newBarPos = m_zoomDim.minY;
      else if(newBarPos > m_zoomDim.maxY)
         newBarPos = m_zoomDim.maxY;
      m_yPoints[0] = newBarPos;
      m_yPoints[1] = newBarPos;
      
      m_curBarPos = newBarPos;
   }
}


   
