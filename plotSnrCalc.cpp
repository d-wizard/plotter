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
#include "plotSnrCalc.h"



plotSnrCalc::plotSnrCalc(QwtPlot* parentPlot):
   m_parentPlot(parentPlot),
   m_isVisable(false),
   m_activeBarIndex(-1)
{
   unsigned int allBarsIndex = 0;
   for(size_t i = 0; i < ARRAY_SIZE(m_noiseBars); ++i)
   {
      m_noiseBars[i] = new plotBar(parentPlot, E_X_AXIS, Qt::white, 3, Qt::DashLine, QwtPlotCurve::Lines);
      m_allBars[allBarsIndex++] = m_noiseBars[i];
   }
   for(size_t i = 0; i < ARRAY_SIZE(m_signalBars); ++i)
   {
      m_signalBars[i] = new plotBar(parentPlot, E_X_AXIS, Qt::green, 3, Qt::DashLine, QwtPlotCurve::Lines);
      m_allBars[allBarsIndex++] = m_signalBars[i];
   }

}

plotSnrCalc::~plotSnrCalc()
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      delete m_allBars[i];
   }
}


void plotSnrCalc::show(const maxMinXY& zoomDim)
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      m_allBars[i]->show(zoomDim);
   }
   m_isVisable = true;
}

void plotSnrCalc::hide()
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      m_allBars[i]->hide();
   }
   m_isVisable = false;
}

bool plotSnrCalc::isVisable()
{
   return m_isVisable;
}

void plotSnrCalc::updateZoom(const maxMinXY& zoomDim)
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      m_allBars[i]->updateZoom(zoomDim, true);
   }
   m_parentPlot->replot();
}


bool plotSnrCalc::isSelectionCloseToBar( const QPointF& pos,
                                         const maxMinXY& zoomDim,
                                         const int canvasWidth_pixels,
                                         const int canvasHeight_pixels)
{
   bool barFound = false;
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      barFound = m_allBars[i]->isSelectionCloseToBar(pos, zoomDim, canvasWidth_pixels, canvasHeight_pixels);
      if(barFound)
      {
         m_activeBarIndex = i;
         break;
      }
   }
   return barFound;
}

void plotSnrCalc::moveBar(const QPointF& pos)
{
   if(m_activeBarIndex >= 0 && m_activeBarIndex < (signed)ARRAY_SIZE(m_allBars))
   {
      m_allBars[m_activeBarIndex]->moveBar(pos);
   }
}

