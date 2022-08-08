/* Copyright 2013 - 2019, 2022 Dan Williams. All Rights Reserved.
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
#include "PlotZoom.h"
#include "mainwindow.h"

PlotZoom::PlotZoom(MainWindow* mainWindow, QwtPlot* qwtPlot, QScrollBar* vertScroll, QScrollBar* horzScroll):
   m_holdZoom(false),
   m_maxHoldZoom(false),
   m_plotIs1D(false),
   m_mainWindow(mainWindow),
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
   m_vertScroll->setVisible(false);
   m_horzScroll->setVisible(false);
}

void PlotZoom::SetPlotDimensions(maxMinXY plotDimensions, bool changeCausedByUserGuiInput)
{
   // Make sure min and max values are not the same (i.e. do not set the
   // plot width/height to 0).
   if(plotDimensions.maxX == plotDimensions.minX)
   {
      plotDimensions.minX -= 1.0;
      plotDimensions.maxX += 1.0;
   }
   if(plotDimensions.maxY == plotDimensions.minY)
   {
      plotDimensions.minY -= 1.0;
      plotDimensions.maxY += 1.0;
   }

   maxMinXY oldPlotDim = m_plotDimensions;
   double width = plotDimensions.maxX - plotDimensions.minX;
   double height = plotDimensions.maxY - plotDimensions.minY;

   m_plotDimensions.maxX = plotDimensions.maxX + (width * ZOOM_OUT_PAD);
   m_plotDimensions.minX = plotDimensions.minX - (width * ZOOM_OUT_PAD);
   m_plotDimensions.maxY = plotDimensions.maxY + (height * ZOOM_OUT_PAD);
   m_plotDimensions.minY = plotDimensions.minY - (height * ZOOM_OUT_PAD);

   m_plotWidth = m_plotDimensions.maxX - m_plotDimensions.minX;
   m_plotHeight = m_plotDimensions.maxY - m_plotDimensions.minY;

   // Check for initial case where m_zoomWidth and m_zoomHeight are still
   // 0 from the construtor.
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

   if(m_maxHoldZoom)
   {
      bool zoomChanged = false;
      maxMinXY modZoomDim = m_plotDimensions;

      if(modZoomDim.maxX > m_zoomDimensions.maxX)
      {
         zoomChanged = true;
      }
      else
      {
         modZoomDim.maxX = m_zoomDimensions.maxX;
      }
      if(modZoomDim.minX < m_zoomDimensions.minX)
      {
         zoomChanged = true;
      }
      else
      {
         modZoomDim.minX = m_zoomDimensions.minX;
      }
      if(modZoomDim.maxY > m_zoomDimensions.maxY)
      {
         zoomChanged = true;
      }
      else
      {
         modZoomDim.maxY = m_zoomDimensions.maxY;
      }
      if(modZoomDim.minY < m_zoomDimensions.minY)
      {
         zoomChanged = true;
      }
      else
      {
         modZoomDim.minY = m_zoomDimensions.minY;
      }

      if(!zoomChanged && m_plotIs1D)
      {
         // If zoomChanged is false, then modZoomDim has been set to m_zoomDimensions.
         // If the plot that we are dealing with only contains 1D curves, we should still update
         // the zoom when the X Axis zoom dimensions are larger than the plot dimensions.
         // The actual bounding of the zoom dimensions will be done in the SetZoom function.
         if(modZoomDim.minX < m_plotDimensions.minX || modZoomDim.maxX > m_plotDimensions.maxX)
         {
            zoomChanged = true;
         }
      }

      if(zoomChanged)
      {
         SetZoom(modZoomDim, changeCausedByUserGuiInput, false);
      }
   }
   else if(m_zoomDimensions == oldPlotDim) // This checks if we are zoomed in.
   {
      // We are not zoomed in, set zoom to new plot dimensions
      SetZoom(m_plotDimensions, changeCausedByUserGuiInput, false);
   }
   else
   {
      // We are zoomed in, check if old zoom is within new plot dimensions
      bool minXValid = false;
      bool maxXValid = false;
      bool minYValid = false;
      bool maxYValid = false;

      if(m_zoomDimensions.minX >= m_plotDimensions.minX && m_zoomDimensions.minX <= m_plotDimensions.maxX)
      {
         minXValid = true;
      }
      if(m_zoomDimensions.maxX >= m_plotDimensions.minX && m_zoomDimensions.maxX <= m_plotDimensions.maxX)
      {
         maxXValid = true;
      }
      if(m_zoomDimensions.minY >= m_plotDimensions.minY && m_zoomDimensions.minY <= m_plotDimensions.maxY)
      {
         minYValid = true;
      }
      if(m_zoomDimensions.maxY >= m_plotDimensions.minY && m_zoomDimensions.maxY <= m_plotDimensions.maxY)
      {
         maxYValid = true;
      }

      maxMinXY modZoomDim = m_zoomDimensions;

      if(minXValid == false && maxXValid == false)
      {
         // Entire axis zoom out of range, reset to current plot dimensions
         modZoomDim.maxX = m_plotDimensions.maxX;
         modZoomDim.minX = m_plotDimensions.minX;
      }
      else if(minXValid == false)
      {
         // Min out of range, max in range, set min to plot dimension min
         modZoomDim.minX = m_plotDimensions.minX;
      }
      else if(maxXValid == false)
      {
         // Max out of range, min in range, set max to plot dimension max
         modZoomDim.maxX = m_plotDimensions.maxX;
      }

      if(minYValid == false && maxYValid == false)
      {
         // Entire axis zoom out of range, reset to current plot dimensions
         modZoomDim.maxY = m_plotDimensions.maxY;
         modZoomDim.minY = m_plotDimensions.minY;
      }
      else if(minYValid == false)
      {
         // Min out of range, max in range, set min to plot dimension min
         modZoomDim.minY = m_plotDimensions.minY;
      }
      else if(maxYValid == false)
      {
         // Max out of range, min in range, set max to plot dimension max
         modZoomDim.maxY = m_plotDimensions.maxY;
      }

      // Reset the zoom so it will be bound by new dimensions
      SetZoom(modZoomDim, changeCausedByUserGuiInput, false);
   }
}

void PlotZoom::SetZoom(maxMinXY zoomDimensions)
{
   SetZoom(zoomDimensions, true, true);
}

void PlotZoom::ResetPlotAxisScale()
{
  QwtScaleDiv div;
  QwtLinearScaleEngine lineSE;

  div = lineSE.divideScale(m_zoomDimensions.minY, m_zoomDimensions.maxY, 12,4);
  m_qwtPlot->setAxisScale(QwtPlot::yLeft, m_zoomDimensions.minY, m_zoomDimensions.maxY);
  m_qwtPlot->setAxisScaleDiv(QwtPlot::yLeft, div);

  div = lineSE.divideScale(m_zoomDimensions.minX, m_zoomDimensions.maxX, 12,4);
  m_qwtPlot->setAxisScale(QwtPlot::xBottom, m_zoomDimensions.minX, m_zoomDimensions.maxX);
  m_qwtPlot->setAxisScaleDiv(QwtPlot::xBottom, div);
}

void PlotZoom::VertSliderMoved()
{
   if(m_curYScrollPos != m_vertScroll->sliderPosition())
   {
       m_curYScrollPos = m_vertScroll->sliderPosition();
       m_zoomDimensions.minY = (((double)m_curYScrollPos) - m_yAxisB) / m_yAxisM;
       m_zoomDimensions.maxY = m_zoomDimensions.minY + m_zoomHeight;

       BoundZoom(m_zoomDimensions);

       ResetPlotAxisScale();
       m_qwtPlot->replot();
   }
}
void PlotZoom::HorzSliderMoved()
{
   if(m_curXScrollPos != m_horzScroll->sliderPosition())
   {
       m_curXScrollPos = m_horzScroll->sliderPosition();
       m_zoomDimensions.minX = (((double)m_curXScrollPos) - m_xAxisB) / m_xAxisM;
       m_zoomDimensions.maxX = m_zoomDimensions.minX + m_zoomWidth;

       BoundZoom(m_zoomDimensions);

       ResetPlotAxisScale();
       m_qwtPlot->replot();
   }
}

void PlotZoom::ModSliderPos(eAxis axis, double changeWindowPercent)
{
    if(axis == E_X_AXIS)
    {
        maxMinXY zoomDim = m_zoomDimensions;
        double width = zoomDim.maxX - zoomDim.minX;
        double changeAmount = width * changeWindowPercent;

        zoomDim.maxX += changeAmount;
        zoomDim.minX += changeAmount;

        // If start or end are out of range, set to max/min, but keep zoom width the same.
        if(zoomDim.maxX > m_plotDimensions.maxX)
        {
            zoomDim.minX = m_plotDimensions.maxX - width;
            zoomDim.maxX = m_plotDimensions.maxX;
        }
        else if(zoomDim.minX < m_plotDimensions.minX)
        {
            zoomDim.maxX = m_plotDimensions.minX + width;
            zoomDim.minX = m_plotDimensions.minX;
        }

        SetZoom(zoomDim, true, false);
    }
    else
    {
        maxMinXY zoomDim = m_zoomDimensions;
        double height = zoomDim.maxY - zoomDim.minY;
        double changeAmount = height * changeWindowPercent;

        zoomDim.maxY += changeAmount;
        zoomDim.minY += changeAmount;

        // If start or end are out of range, set to max/min, but keep zoom height the same.
        if(zoomDim.maxY > m_plotDimensions.maxY)
        {
            zoomDim.minY = m_plotDimensions.maxY - height;
            zoomDim.maxY = m_plotDimensions.maxY;
        }
        else if(zoomDim.minY < m_plotDimensions.minY)
        {
            zoomDim.maxY = m_plotDimensions.minY + height;
            zoomDim.minY = m_plotDimensions.minY;
        }

        SetZoom(zoomDim, true, false);
    }

}

void PlotZoom::BoundZoom(maxMinXY& zoom)
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

void PlotZoom::BoundZoomXaxis(maxMinXY& zoom)
{
   if(zoom.minX < m_plotDimensions.minX)
   {
       zoom.minX = m_plotDimensions.minX;
   }
   if(zoom.maxX > m_plotDimensions.maxX)
   {
       zoom.maxX = m_plotDimensions.maxX;
   }
}

void PlotZoom::BoundScroll(QScrollBar* scroll, int& newPos)
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

void PlotZoom::Zoom(double zoomFactor)
{
    // Keep current zoom center point the center
    Zoom(zoomFactor, QPointF(0.5,0.5));
}

void PlotZoom::Zoom(double zoomFactor, QPointF relativeMousePos)
{
   Zoom(zoomFactor, relativeMousePos, false);
}

// relativeMousePos is the % (0 to 1) of where the mouse is when zoom is requested
// Keep the relative point point at the same poisition in the canvas as before the zoom.
void PlotZoom::Zoom(double zoomFactor, QPointF relativeMousePos, bool holdYAxis)
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
    if(holdYAxis == false)
    {
        zoom.minY = newYPoint - downOfPoint;
        zoom.maxY = newYPoint + upOfPoint;
    }
    zoom.minX = newXPoint - leftOfPoint;
    zoom.maxX = newXPoint + rightOfPoint;

    if(m_holdZoom == false)
    {
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
    }

    SetZoom(zoom, true, false);
}

void PlotZoom::ResetZoom()
{
    SetZoom(m_plotDimensions, true, false);
}

void PlotZoom::changeZoomFromSavedZooms(int changeZoomDelta)
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
            SetZoom(m_zoomDimSave[m_zoomDimIndex], true, false);
        }
    }

    if(resetZoom)
    {
        ResetZoom();
    }
}

maxMinXY PlotZoom::getCurZoom()
{
    return m_zoomDimensions;
}

maxMinXY PlotZoom::getCurPlotDim()
{
   return m_plotDimensions;
}

void PlotZoom::SetZoom(maxMinXY zoomDimensions, bool changeCausedByUserGuiInput, bool saveZoom)
{
   maxMinXY origZoomDim = m_zoomDimensions;

   // Nothing in the save zoom vector, initialize it with the plot dimensions.
   if(saveZoom == true && m_zoomDimSave.size() == 0)
   {
      m_zoomDimSave.resize(1);
      m_zoomDimIndex = 0;
      m_zoomDimSave[m_zoomDimIndex] = m_plotDimensions;
   }
   
   if(changeCausedByUserGuiInput == false && m_holdZoom == true)
   {
      zoomDimensions = m_zoomDimensions;
   }

   if(m_maxHoldZoom == true && changeCausedByUserGuiInput == true)
   {
      // Max hold should only keep the zoom held at max for changes caused by new data.
      // If a change is made by the User via the GUI then hold the zoom where the user set it to.
      m_holdZoom = true;
   }

   if(m_maxHoldZoom == false && m_holdZoom == false)
   {
      BoundZoom(zoomDimensions);
   }
   else if(m_plotIs1D == true && m_holdZoom == false)
   {
      BoundZoomXaxis(zoomDimensions); // For 1D plots do not apply max hold to the X axis.
   }

   double zoomWidth = zoomDimensions.maxX - zoomDimensions.minX;
   double zoomHeight = zoomDimensions.maxY - zoomDimensions.minY;

   if(zoomWidth > 0.0 && zoomHeight > 0.0)
   {
      m_zoomDimensions = zoomDimensions;
      m_zoomWidth = zoomWidth;
      m_zoomHeight = zoomHeight;

      ResetPlotAxisScale();

      UpdateScrollBars(changeCausedByUserGuiInput);

      // Remove any saved next zooms and store the current zoom and the new zoom.
      if(saveZoom == true)
      {
         // Make sure the current zoom is saved to the front of the Saved Zoom vector.
         // This way we can guarantee that the user can easily get back to this zoom.
         if(m_zoomDimSave[m_zoomDimIndex] != origZoomDim)
         {
            ++m_zoomDimIndex;
            m_zoomDimSave.resize(m_zoomDimIndex+1);
            m_zoomDimSave[m_zoomDimIndex] = origZoomDim;
         }

         // Add the new zoom to the front of the vector.
         ++m_zoomDimIndex;
         m_zoomDimSave.resize(m_zoomDimIndex+1);
         m_zoomDimSave[m_zoomDimIndex] = m_zoomDimensions;
      }

      // Call function in parent... The parent might have some stuff to do before replot is called.
      bool xAxisZoomChanged = m_zoomDimensions.minX != origZoomDim.minX || m_zoomDimensions.maxX != origZoomDim.maxX;
      m_mainWindow->plotZoomDimChanged(m_plotDimensions, m_zoomDimensions, xAxisZoomChanged, changeCausedByUserGuiInput);

      if(changeCausedByUserGuiInput)
      {
         m_qwtPlot->replot();
      }
   }
}

// Move the zoom window, keeping the zoom width and height the same.
void PlotZoom::moveZoom(double deltaX, double deltaY, bool changeCausedByUserGuiInput)
{
   maxMinXY newZoom = m_zoomDimensions;
   newZoom.maxX += deltaX;
   newZoom.minX += deltaX;
   newZoom.maxY += deltaY;
   newZoom.minY += deltaY;

   if(m_maxHoldZoom == false) // In Max Hold Zoom we can have zoom dimensions that are larger than the plot dimensions.
   {
      // Make sure the zoom window doesn't move outside of the plot dimensions.
      if(newZoom.maxX > m_plotDimensions.maxX)
      {
         double delta = newZoom.maxX - m_plotDimensions.maxX;
         newZoom.maxX -= delta;
         newZoom.minX -= delta;
      }
      if(newZoom.minX < m_plotDimensions.minX)
      {
         double delta = m_plotDimensions.minX - newZoom.minX;
         newZoom.maxX += delta;
         newZoom.minX += delta;
      }
      if(newZoom.maxY > m_plotDimensions.maxY)
      {
         double delta = newZoom.maxY - m_plotDimensions.maxY;
         newZoom.maxY -= delta;
         newZoom.minY -= delta;
      }
      if(newZoom.minY < m_plotDimensions.minY)
      {
         double delta = m_plotDimensions.minY - newZoom.minY;
         newZoom.maxY += delta;
         newZoom.minY += delta;
      }
   }

   if(newZoom != m_zoomDimensions)
   {
      SetZoom(newZoom, changeCausedByUserGuiInput, false);
   }
}

void PlotZoom::UpdateScrollBars(bool changeCausedByUserGuiInput)
{
   // Update Scroll Bar variables.
   m_xAxisM = (double)(m_scrollBarResXAxis-1)/ (m_plotWidth - m_zoomWidth);
   m_xAxisB = (-m_xAxisM) * m_plotDimensions.minX;
   m_yAxisM = (double)(1-m_scrollBarResYAxis)/ (m_plotHeight - m_zoomHeight);
   m_yAxisB = (-m_yAxisM) * (m_plotDimensions.maxY - m_zoomHeight);

   m_curXScrollPos = (int)((m_zoomDimensions.minX * m_xAxisM) + m_xAxisB);
   m_curYScrollPos = (int)((m_zoomDimensions.minY * m_yAxisM) + m_yAxisB);


   // Normalize to -1 to 1 before checking if they are close (calculate m & b from the plot dimensions)
   double mX = 2.0 / (m_plotDimensions.maxX -  m_plotDimensions.minX);
   double mY = 2.0 / (m_plotDimensions.maxY -  m_plotDimensions.minY);
   double bX = 1.0 - mX * m_plotDimensions.maxX;
   double bY = 1.0 - mY * m_plotDimensions.maxY;


   // Check if zoom and plot are the same, take into account rounding error by comparing normalized versions of zoom and plot.
   bool areTheyClose_minX = areTheyClose(m_zoomDimensions.minX*mX+bX, -1);
   bool areTheyClose_maxX = areTheyClose(m_zoomDimensions.maxX*mX+bX,  1);
   bool areTheyClose_minY = areTheyClose(m_zoomDimensions.minY*mY+bY, -1);
   bool areTheyClose_maxY = areTheyClose(m_zoomDimensions.maxY*mY+bY,  1);

   bool setXScrollBars = false;
   bool setYScrollBars = false;

   if( areTheyClose_minX && areTheyClose_maxX )
   {
      // Zoom and plot are the same, no need for scroll bar.
      setXScrollBars = false;
   }
   else if(m_maxHoldZoom == false)
   {
      // Max hold is off, show scroll bar.
      setXScrollBars = true;
   }
   else if( (areTheyClose_minX || m_zoomDimensions.minX < m_plotDimensions.minX) &&
            (areTheyClose_maxX || m_zoomDimensions.maxX > m_plotDimensions.maxX))
   {
      // Max hold is on and plot is within zoom, no need for scroll bar.
      setXScrollBars = false;
   }
   else
   {
      // Max hold is on but zoom is narrower than plot dimensions, show scroll bar.
      setXScrollBars = true;
   }

   if( areTheyClose_minY && areTheyClose_maxY )
   {
      // Zoom and plot are the same, no need for scroll bar.
      setYScrollBars = false;
   }
   else if(m_maxHoldZoom == false)
   {
      // Max hold is off, show scroll bar.
      setYScrollBars = true;
   }
   else if( (areTheyClose_minY || m_zoomDimensions.minY < m_plotDimensions.minY) &&
            (areTheyClose_maxY || m_zoomDimensions.maxY > m_plotDimensions.maxY))
   {
      // Max hold is on and plot is within zoom, no need for scroll bar.
      setYScrollBars = false;
   }
   else
   {
      // Max hold is on but zoom is narrower than plot dimensions, show scroll bar.
      setYScrollBars = true;
   }

   // Make sure not to flicker the scroll bar on and off when the plot data is changing.
   // So only turn it off if a change was made by user GUI input.
   if(setXScrollBars == false && (changeCausedByUserGuiInput || !m_horzScroll->isVisible()))
   {
      m_horzScroll->setRange(0, 0);
      m_horzScroll->setVisible(false);
      m_curXScrollPos = -1;
   }
   else
   {
      m_horzScroll->setRange(0, m_scrollBarResXAxis-1);
      m_horzScroll->setVisible(true);
      BoundScroll(m_horzScroll, m_curXScrollPos);
      m_horzScroll->setSliderPosition(m_curXScrollPos);
   }

   // Make sure not to flicker the scroll bar on and off when the plot data is changing.
   // So only turn it off if a change was made by user GUI input.
   if(setYScrollBars == false && (changeCausedByUserGuiInput || !m_vertScroll->isVisible()))
   {
      m_vertScroll->setRange(0, 0);
      m_vertScroll->setVisible(false);
      m_curYScrollPos = -1;
   }
   else
   {
      m_vertScroll->setRange(0, m_scrollBarResYAxis-1);
      m_vertScroll->setVisible(true);
      BoundScroll(m_vertScroll, m_curYScrollPos);
      m_vertScroll->setSliderPosition(m_curYScrollPos);
   }

}

bool PlotZoom::areTheyClose(double val1, double val2)
{
   bool close = false;
   double ratio = 0.0;
   if(val1 == val2)
   {
      close = true;
   }
   else if(val1 != 0.0)
   {
      ratio = val2/val1;
   }
   else
   {
      ratio = val1/val2;
   }

   // If the values are close, the ratio should be very near +1.0
   if(close == false && fabs(1.0 - ratio) < 0.00001)
   {
      close = true;
   }

   return close;
}
