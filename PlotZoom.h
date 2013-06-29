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
#ifndef PlotZoom_h
#define PlotZoom_h

#include <QScrollBar>
#include "PlotHelperTypes.h"
#include <qwt_plot.h>

class PlotZoom
{
public:
   PlotZoom(QwtPlot* qwtPlot, QScrollBar* vertScroll, QScrollBar* horzScroll):
       m_qwtPlot(qwtPlot),
       m_vertScroll(vertScroll),
       m_horzScroll(horzScroll),
       m_plotWidth(0),
       m_zoomWidth(0),
       m_plotHeight(0),
       m_zoomHeight(0),
       m_xAxisM(0),
       m_xAxisB(0),
       m_yAxisM(0),
       m_yAxisB(0),
       m_scrollBarResXAxis(1000),
       m_scrollBarResYAxis(1000),
       m_curXScrollPos(-1),
       m_curYScrollPos(-1)
   {
       m_plotDimensions.minX = 0;
       m_plotDimensions.minY = 0;
       m_plotDimensions.maxX = 0;
       m_plotDimensions.maxY = 0;

       m_zoomDimensions.minX = 0;
       m_zoomDimensions.minY = 0;
       m_zoomDimensions.maxX = 0;
       m_zoomDimensions.maxY = 0;

       m_vertScroll->setRange(0,0);
       m_horzScroll->setRange(0,0);
   }

   void SetPlotDimensions(maxMinXY plotDimensions)
   {
        m_plotDimensions = plotDimensions;
        m_plotWidth = m_plotDimensions.maxX - m_plotDimensions.minX;
        m_plotHeight = m_plotDimensions.maxY - m_plotDimensions.minY;
   }
   
   void SetZoom(maxMinXY zoomDimensions)
   {
        m_zoomDimensions = zoomDimensions;
        m_zoomWidth = m_zoomDimensions.maxX - m_zoomDimensions.minX;
        m_zoomHeight = m_zoomDimensions.maxY - m_zoomDimensions.minY;

        m_qwtPlot->setAxisScale(QwtPlot::yLeft, m_zoomDimensions.minY, m_zoomDimensions.maxY);
        m_qwtPlot->setAxisScale(QwtPlot::xBottom, m_zoomDimensions.minX, m_zoomDimensions.maxX);

        m_xAxisM = (double)(m_scrollBarResXAxis-1)/ (m_plotWidth - m_zoomWidth);
        m_xAxisB = (-m_xAxisM) * m_plotDimensions.minX;
        m_yAxisM = (double)(1-m_scrollBarResYAxis)/ (m_plotHeight - m_zoomHeight);
        m_yAxisB = (-m_yAxisM) * (m_plotDimensions.maxY - m_zoomHeight);

        m_curXScrollPos = (int)((m_zoomDimensions.minX * m_xAxisM) + m_xAxisB);
        m_curYScrollPos = (int)((m_zoomDimensions.minY * m_yAxisM) + m_yAxisB);

        if(m_curXScrollPos < 0)
        {
            m_curXScrollPos = 0;
        }
        else if(m_curXScrollPos >= m_scrollBarResXAxis)
        {
            m_curXScrollPos = m_scrollBarResXAxis - 1;
        }
        if(m_curYScrollPos < 0)
        {
            m_curYScrollPos = 0;
        }
        else if(m_curYScrollPos >= m_scrollBarResYAxis)
        {
            m_curYScrollPos = m_scrollBarResYAxis - 1;
        }

        if(m_zoomDimensions.minX < m_plotDimensions.minX)
        {
            m_zoomDimensions.minX = m_plotDimensions.minX;
        }
        if(m_zoomDimensions.maxX > m_plotDimensions.maxX)
        {
            m_zoomDimensions.maxX = m_plotDimensions.maxX;
        }
        if(m_zoomDimensions.minY < m_plotDimensions.minY)
        {
            m_zoomDimensions.minY = m_plotDimensions.minY;
        }
        if(m_zoomDimensions.maxY > m_plotDimensions.maxY)
        {
            m_zoomDimensions.maxY = m_plotDimensions.maxY;
        }

        if( m_zoomDimensions.minX == m_plotDimensions.minX &&
            m_zoomDimensions.maxX == m_plotDimensions.maxX )
        {
            m_horzScroll->setRange(0, 0);
            m_curXScrollPos = -1;
        }
        else
        {
            m_horzScroll->setRange(0, m_scrollBarResXAxis-1);
            m_horzScroll->setSliderPosition(m_curXScrollPos);
        }

        if( m_zoomDimensions.minY == m_plotDimensions.minY &&
            m_zoomDimensions.maxY == m_plotDimensions.maxY )
        {
            m_vertScroll->setRange(0, 0);
            m_curYScrollPos = -1;
        }
        else
        {
            m_vertScroll->setRange(0, m_scrollBarResYAxis-1);
            m_vertScroll->setSliderPosition(m_curYScrollPos);
        }

        m_qwtPlot->replot();

   }

   void VertSliderMoved()
   {
       if(m_curYScrollPos != m_vertScroll->sliderPosition())
       {
           m_curYScrollPos = m_vertScroll->sliderPosition();
           m_zoomDimensions.minY = (((double)m_curYScrollPos) - m_yAxisB) / m_yAxisM;
           m_zoomDimensions.maxY = m_zoomDimensions.minY + m_zoomHeight;

           m_qwtPlot->setAxisScale(QwtPlot::yLeft, m_zoomDimensions.minY, m_zoomDimensions.maxY);
           m_qwtPlot->replot();
       }
   }
   void HorzSliderMoved()
   {
       if(m_curXScrollPos != m_horzScroll->sliderPosition())
       {
           m_curXScrollPos = m_horzScroll->sliderPosition();
           m_zoomDimensions.minX = (((double)m_curXScrollPos) - m_xAxisB) / m_xAxisM;
           m_zoomDimensions.maxX = m_zoomDimensions.minX + m_zoomWidth;

           m_qwtPlot->setAxisScale(QwtPlot::xBottom, m_zoomDimensions.minX, m_zoomDimensions.maxX);
           m_qwtPlot->replot();
       }
   }

private:
   QwtPlot* m_qwtPlot;
   
   QScrollBar* m_vertScroll;
   QScrollBar* m_horzScroll;
   
   maxMinXY m_plotDimensions;
   maxMinXY m_zoomDimensions;

   double m_plotWidth;
   double m_plotHeight;
   double m_zoomWidth;
   double m_zoomHeight;
   
   double m_xAxisM;
   double m_xAxisB;
   double m_yAxisM;
   double m_yAxisB;

   int m_scrollBarResXAxis;
   int m_scrollBarResYAxis;

   int m_curXScrollPos;
   int m_curYScrollPos;
   
   PlotZoom();
};


#endif


