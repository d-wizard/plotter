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
#include <QVector>
#include "PlotHelperTypes.h"
#include <qwt_plot.h>

#include <math.h>

#define ZOOM_OUT_PAD (0.02)
#define ZOOM_SCROLL_RESOLUTION (10000)
#define ZOOM_SCROLL_CHANGE_SMALL (50)
#define ZOOM_SCROLL_CHANGE_BIG (250)
#define ZOOM_IN_PERCENT (0.9)
#define ZOOM_OUT_PERCENT (1.1)

class PlotZoom
{
public:
   PlotZoom(QwtPlot* qwtPlot, QScrollBar* vertScroll, QScrollBar* horzScroll):
       m_qwtPlot(qwtPlot),
       m_vertScroll(vertScroll),
       m_horzScroll(horzScroll),
       m_plotWidth(0),
       m_plotHeight(0),
       m_zoomWidth(0),
       m_zoomHeight(0),
       m_xAxisM(0),
       m_xAxisB(0),
       m_yAxisM(0),
       m_yAxisB(0),
       m_scrollBarResXAxis(ZOOM_SCROLL_RESOLUTION),
       m_scrollBarResYAxis(ZOOM_SCROLL_RESOLUTION),
       m_curXScrollPos(-1),
       m_curYScrollPos(-1),
       m_zoomDimIndex(0)
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
        double width = plotDimensions.maxX - plotDimensions.minX;
        double height = plotDimensions.maxY - plotDimensions.minY;

        m_plotDimensions.maxX = plotDimensions.maxX + (width * ZOOM_OUT_PAD);
        m_plotDimensions.minX = plotDimensions.minX - (width * ZOOM_OUT_PAD);
        m_plotDimensions.maxY = plotDimensions.maxY + (height * ZOOM_OUT_PAD);
        m_plotDimensions.minY = plotDimensions.minY - (height * ZOOM_OUT_PAD);

        m_plotWidth = m_plotDimensions.maxX - m_plotDimensions.minX;
        m_plotHeight = m_plotDimensions.maxY - m_plotDimensions.minY;

        if(m_zoomWidth == 0)
        {
            m_zoomWidth = m_plotWidth;
            m_zoomDimensions.minX = m_plotDimensions.minX;
            m_zoomDimensions.maxX = m_plotDimensions.maxX;
        }
        if(m_zoomHeight == 0)
        {
            m_zoomHeight = m_plotHeight;
            m_zoomDimensions.minY = m_plotDimensions.minY;
            m_zoomDimensions.maxY = m_plotDimensions.maxY;
        }
   }

   void SetZoom(maxMinXY zoomDimensions)
   {
       SetZoom(zoomDimensions, true);
   }

   void VertSliderMoved()
   {
       if(m_curYScrollPos != m_vertScroll->sliderPosition())
       {
           m_curYScrollPos = m_vertScroll->sliderPosition();
           m_zoomDimensions.minY = (((double)m_curYScrollPos) - m_yAxisB) / m_yAxisM;
           m_zoomDimensions.maxY = m_zoomDimensions.minY + m_zoomHeight;

           BoundZoom(m_zoomDimensions);

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

           BoundZoom(m_zoomDimensions);

           m_qwtPlot->setAxisScale(QwtPlot::xBottom, m_zoomDimensions.minX, m_zoomDimensions.maxX);
           m_qwtPlot->replot();
       }
   }

    void ModSliderPos(QScrollBar* scroll, int posMod)
    {
        if((scroll == m_vertScroll || scroll == m_horzScroll) && posMod != 0)
        {
            int newPos = scroll->sliderPosition() + posMod;

            BoundScroll(scroll, newPos);

            if(newPos != scroll->sliderPosition())
            {
                scroll->setSliderPosition(newPos);
                if(scroll == m_vertScroll)
                {
                    VertSliderMoved();
                }
                else if(scroll == m_horzScroll)
                {
                    HorzSliderMoved();
                }
            }
        }
    }

    void BoundZoom(maxMinXY& zoom)
    {
        if(zoom.minX < m_plotDimensions.minX)
        {
            zoom.minX = m_plotDimensions.minX;
        }
        if(zoom.maxX > m_plotDimensions.maxX)
        {
            zoom.maxX = m_plotDimensions.maxX;
        }
        if(zoom.minY < m_plotDimensions.minY)
        {
            zoom.minY = m_plotDimensions.minY;
        }
        if(zoom.maxY > m_plotDimensions.maxY)
        {
            zoom.maxY = m_plotDimensions.maxY;
        }
    }

    void BoundScroll(QScrollBar* scroll, int& newPos)
    {
        if(newPos < scroll->minimum())
        {
            newPos = scroll->minimum();
        }
        else if(newPos > scroll->maximum())
        {
            newPos = scroll->maximum();
        }
    }

    void Zoom(double zoomFactor)
    {
        // Keep current zoom center point the center
        Zoom(zoomFactor, QPointF(0.5,0.5));
    }


    // relativeMousePos is the % (0 to 1) of where the mouse is when zoom is requested
    // Keep the relative point point at the same poisition in the canvas as before the zoom.
    void Zoom(double zoomFactor, QPointF relativeMousePos)
    {
        double newXPoint = m_zoomWidth * relativeMousePos.x() + m_zoomDimensions.minX;
        double newYPoint = m_zoomHeight * relativeMousePos.y() + m_zoomDimensions.minY;

        double newWidth = m_zoomWidth * zoomFactor;
        double newHeight = m_zoomHeight * zoomFactor;

        double leftOfPoint = newWidth * relativeMousePos.x();
        double rightOfPoint = newWidth - leftOfPoint;
        double downOfPoint = newHeight * relativeMousePos.y();
        double upOfPoint = newHeight - downOfPoint;

        maxMinXY zoom = m_zoomDimensions;
        zoom.minX = newXPoint - leftOfPoint;
        zoom.minY = newYPoint - downOfPoint;
        zoom.maxX = newXPoint + rightOfPoint;
        zoom.maxY = newYPoint + upOfPoint;

        // If out of bounds, set within bounds and try to keep the same width/height
        if(zoom.minX < m_plotDimensions.minX)
        {
            double diff = m_plotDimensions.minX - zoom.minX;
            zoom.minX = m_plotDimensions.minX;
            zoom.maxX += diff;
            if(zoom.maxX > m_plotDimensions.maxX)
            {
                zoom.maxX = m_plotDimensions.maxX;
            }
        }
        if(zoom.maxX > m_plotDimensions.maxX)
        {
            double diff = zoom.maxX - m_plotDimensions.maxX;
            zoom.maxX = m_plotDimensions.maxX;
            zoom.minX -= diff;
            if(zoom.minX < m_plotDimensions.minX)
            {
                zoom.minX = m_plotDimensions.minX;
            }
        }
        if(zoom.minY < m_plotDimensions.minY)
        {
            double diff = m_plotDimensions.minY - zoom.minY;
            zoom.minY = m_plotDimensions.minY;
            zoom.maxY += diff;
            if(zoom.maxY > m_plotDimensions.maxY)
            {
                zoom.maxY = m_plotDimensions.maxY;
            }
        }
        if(zoom.maxY > m_plotDimensions.maxY)
        {
            double diff = zoom.maxY - m_plotDimensions.maxY;
            zoom.maxY = m_plotDimensions.maxY;
            zoom.minY -= diff;
            if(zoom.minY < m_plotDimensions.minY)
            {
                zoom.minY = m_plotDimensions.minY;
            }
        }

        SetZoom(zoom, false);
    }

    void ResetZoom()
    {
        SetZoom(m_plotDimensions, false);
    }

    void changeZoomFromSavedZooms(int changeZoomDelta)
    {
        bool resetZoom = true;

        if(m_zoomDimSave.size() > 0)
        {
            int newZoomIndex = m_zoomDimIndex + changeZoomDelta;
            resetZoom = false;
            if(newZoomIndex < 0)
            {
                newZoomIndex = 0;
                resetZoom = true;
            }
            else if( newZoomIndex >= m_zoomDimSave.size())
            {
                newZoomIndex = m_zoomDimSave.size() - 1;
            }

            if(resetZoom == false && (unsigned int)newZoomIndex != m_zoomDimIndex)
            {
                m_zoomDimIndex = newZoomIndex;
                SetZoom(m_zoomDimSave[m_zoomDimIndex], false);
            }
        }

        if(resetZoom)
        {
            ResetZoom();
        }
    }

    maxMinXY getCurZoom()
    {
        return m_zoomDimensions;
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

   QVector<maxMinXY> m_zoomDimSave;
   unsigned int m_zoomDimIndex;

   void SetZoom(maxMinXY zoomDimensions, bool saveZoom)
   {
       // Nothing in the save zoom vector, initialize it with current zoom value.
       if(saveZoom == true && m_zoomDimSave.size() == 0)
       {
           m_zoomDimSave.resize(1);
           m_zoomDimIndex = 0;
           m_zoomDimSave[m_zoomDimIndex] = m_plotDimensions;//m_zoomDimensions;

       }

       BoundZoom(zoomDimensions);
       double zoomWidth = zoomDimensions.maxX - zoomDimensions.minX;
       double zoomHeight = zoomDimensions.maxY - zoomDimensions.minY;

       if(zoomWidth > 0.0 && zoomHeight > 0.0)
       {
           m_zoomDimensions = zoomDimensions;
           m_zoomWidth = zoomWidth;
           m_zoomHeight = zoomHeight;

           m_qwtPlot->setAxisScale(QwtPlot::yLeft, m_zoomDimensions.minY, m_zoomDimensions.maxY);
           m_qwtPlot->setAxisScale(QwtPlot::xBottom, m_zoomDimensions.minX, m_zoomDimensions.maxX);

           m_xAxisM = (double)(m_scrollBarResXAxis-1)/ (m_plotWidth - m_zoomWidth);
           m_xAxisB = (-m_xAxisM) * m_plotDimensions.minX;
           m_yAxisM = (double)(1-m_scrollBarResYAxis)/ (m_plotHeight - m_zoomHeight);
           m_yAxisB = (-m_yAxisM) * (m_plotDimensions.maxY - m_zoomHeight);

           m_curXScrollPos = (int)((m_zoomDimensions.minX * m_xAxisM) + m_xAxisB);
           m_curYScrollPos = (int)((m_zoomDimensions.minY * m_yAxisM) + m_yAxisB);


           if( m_zoomDimensions.minX == m_plotDimensions.minX &&
               m_zoomDimensions.maxX == m_plotDimensions.maxX )
           {
               m_horzScroll->setRange(0, 0);
               m_curXScrollPos = -1;
           }
           else
           {
               m_horzScroll->setRange(0, m_scrollBarResXAxis-1);
               BoundScroll(m_horzScroll, m_curXScrollPos);
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
               BoundScroll(m_vertScroll, m_curYScrollPos);
               m_vertScroll->setSliderPosition(m_curYScrollPos);
           }

           // Remove any next zooms and set the current zoom.
           if(saveZoom == true)
           {
               ++m_zoomDimIndex;
               m_zoomDimSave.resize(m_zoomDimIndex+1);
               m_zoomDimSave[m_zoomDimIndex] = m_zoomDimensions;
           }

           m_qwtPlot->replot();
       }

   }

   PlotZoom();
};


#endif


