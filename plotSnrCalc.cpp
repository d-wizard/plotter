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



plotSnrCalc::plotSnrCalc(QwtPlot* parentPlot, QLabel* snrLabel):
   m_parentPlot(parentPlot),
   m_snrLabel(snrLabel),
   m_parentCurve(NULL),
   m_isVisable(false),
   m_activeBarIndex(-1)
{
   m_snrLabel->setVisible(m_isVisable);

   unsigned int allBarsIndex = 0;
   for(size_t i = 0; i < ARRAY_SIZE(m_noiseBars); ++i)
   {
      QString barName = "Noise Bar " + QString::number(i + 1);
      m_noiseBars[i] = new plotBar(parentPlot, barName, E_X_AXIS, Qt::white, 3, Qt::DashLine, QwtPlotCurve::Lines);
      m_allBars[allBarsIndex++] = m_noiseBars[i];
   }
   for(size_t i = 0; i < ARRAY_SIZE(m_signalBars); ++i)
   {
      QString barName = "Signal Bar " + QString::number(i + 1);
      QColor lightGreen = QColor(128,255,128);
      m_signalBars[i] = new plotBar(parentPlot, barName, E_X_AXIS, lightGreen, 3, Qt::DashLine, QwtPlotCurve::Lines);
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
   calcSnr();
   m_snrLabel->setVisible(m_isVisable);
}

void plotSnrCalc::hide()
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      m_allBars[i]->hide();
   }
   m_isVisable = false;
   m_snrLabel->setVisible(m_isVisable);
}

bool plotSnrCalc::isVisable()
{
   return m_isVisable;
}

void plotSnrCalc::updateZoom(const maxMinXY& zoomDim, bool skipReplot)
{
   for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
   {
      m_allBars[i]->updateZoom(zoomDim, true);
   }
   if(skipReplot == false)
   {
      m_parentPlot->replot();
   }
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
   calcSnr();
}


void plotSnrCalc::moveToFront(bool skipReplot)
{
   if(m_isVisable)
   {
      for(size_t i = 0; i < ARRAY_SIZE(m_allBars); ++i)
      {
         m_allBars[i]->moveToFront();
      }
      if(skipReplot == false)
      {
         m_parentPlot->replot();
      }
   }
}

void plotSnrCalc::setCurve(CurveData* curve)
{
   if(curve != m_parentCurve)
   {
      m_parentCurve = curve;
      calcSnr();
   }
}

void plotSnrCalc::calcSnr()
{
   if(m_isVisable && m_parentCurve != NULL)
   {
      unsigned int numPoints = m_parentCurve->getNumPoints();
      ePlotType plotType = m_parentCurve->getPlotType();
      const double* xPoints = m_parentCurve->getXPoints();
      const double* yPoints = m_parentCurve->getYPoints();
      QColor color = m_parentCurve->getColor();

      // Noise
      calcPower(
         m_noiseBars[0]->getBarPos(),
         m_noiseBars[1]->getBarPos(),
         &m_noiseWidth,
         &m_noisePower,
         numPoints,
         plotType,
         xPoints,
         yPoints);

      // Signal
      calcPower(
         m_signalBars[0]->getBarPos(),
         m_signalBars[1]->getBarPos(),
         &m_signalWidth,
         &m_signalPower,
         numPoints,
         plotType,
         xPoints,
         yPoints);


      double snr = m_signalPower - m_noisePower;


      QPalette palette = m_snrLabel->palette();
      palette.setColor( QPalette::WindowText, color);
      palette.setColor( QPalette::Text, color);
      m_snrLabel->setPalette(palette);
      m_snrLabel->setText(QString::number(snr) + " dB");

   }
}

void plotSnrCalc::calcPower(
   double start,
   double stop,
   double* width,
   double* power,
   unsigned int numPoints,
   ePlotType plotType,
   const double* xPoints,
   const double* yPoints )
{
   if(start > stop)
   {
      double swap = start;
      start = stop;
      stop = swap;
   }

   double minX = stop;
   double maxX = start;

   double powerSumLinear = 0;

   for(unsigned int i = 0; i <= numPoints; ++i)
   {
      if(xPoints[i] >= start && xPoints[i] <= stop)
      {
         if(plotType == E_PLOT_TYPE_DB_POWER_FFT_REAL || plotType == E_PLOT_TYPE_DB_POWER_FFT_COMPLEX)
         {
            powerSumLinear += pow(10.0, yPoints[i] / 10.0);
         }
         else
         {
            powerSumLinear += yPoints[i];
         }

         if(xPoints[i] < minX)
            minX = xPoints[i];
         if(xPoints[i] > maxX)
            maxX = xPoints[i];

      }
   }

   *width = maxX - minX;
   *power = 10.0 * log10(powerSumLinear);

}
