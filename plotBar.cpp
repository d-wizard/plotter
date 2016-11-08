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
#include "plotBar.h"


plotBar::plotBar( QwtPlot* parentPlot,
                  eAxis barAxis,
                  QColor color, 
                  qreal width, 
                  Qt::PenStyle penStyle,
                  QwtPlotCurve::CurveStyle curveStyle)
{
   init(parentPlot,
         barAxis,
         color,
         width,
         penStyle,
         curveStyle,
         3);
}


plotBar::~plotBar()
{
   delete m_curve;
}

void plotBar::init( QwtPlot* parentPlot,
                    eAxis barAxis,
                    QColor color, 
                    qreal width, 
                    Qt::PenStyle penStyle,
                    QwtPlotCurve::CurveStyle curveStyle,
                    double barSelectThresh_pixels)
{
   m_parentPlot = parentPlot;
   
   m_curve = new QwtPlotCurve;
   m_curve->setPen(color, width, penStyle);
   m_curve->setStyle(curveStyle);
   

   m_barAxis = barAxis;
   m_isVisable = false;
   
   m_barSelectThresh_pixels = barSelectThresh_pixels;

   m_zoomDim.maxX = 0;
   m_zoomDim.minX = 0;
   m_zoomDim.maxY = 0;
   m_zoomDim.minY = 0;

}


void plotBar::show(const maxMinXY& zoomDim)
{
   m_zoomDim = zoomDim;
   setBarPoints(m_curBarPos, m_zoomDim);

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

bool plotBar::isSelectionCloseToBar( const QPointF& pos, 
                            const maxMinXY& zoomDim, 
                            const int canvasWidth_pixels,
                            const int canvasHeight_pixels)
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

   return selectionDelta_pixels <= m_barSelectThresh_pixels;
}

void plotBar::moveBar(const QPointF& pos)
{
   double newBarPos = m_barAxis == E_X_AXIS ? pos.x() : pos.y();
   setBarPoints(newBarPos, m_zoomDim);
   m_curve->setSamples(m_xPoints, m_yPoints, 2);
   m_parentPlot->replot();
}

void plotBar::updateZoom(const maxMinXY& zoomDim)
{
   m_zoomDim = zoomDim;
}

void plotBar::setBarPoints(double newBarPos, const maxMinXY& zoomDim)
{
   if(m_barAxis == E_X_AXIS)
   {
      if(newBarPos < zoomDim.minX)
         newBarPos = zoomDim.minX;
      else if(newBarPos > zoomDim.maxX)
         newBarPos = zoomDim.maxX;
      m_xPoints[0] = newBarPos;
      m_xPoints[1] = newBarPos;
      m_yPoints[0] = zoomDim.minY;
      m_yPoints[1] = zoomDim.maxY;
      
      m_curBarPos = newBarPos;
   }
   else
   {
      if(newBarPos < zoomDim.minY)
         newBarPos = zoomDim.minY;
      else if(newBarPos > zoomDim.maxY)
         newBarPos = zoomDim.maxY;
      m_yPoints[0] = newBarPos;
      m_yPoints[1] = newBarPos;;
      m_xPoints[0] = zoomDim.minX;
      m_xPoints[1] = zoomDim.maxX;
      
      m_curBarPos = newBarPos;
   }
}


   
