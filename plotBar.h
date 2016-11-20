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
#ifndef PLOTBAR_H
#define PLOTBAR_H

#include <QColor>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "PlotHelperTypes.h"

class plotBar
{
public:
   plotBar( QwtPlot* parentPlot,
            QString barName,
            eAxis barAxis,
            QColor color, 
            qreal width, 
            Qt::PenStyle penStyle,
            QwtPlotCurve::CurveStyle curveStyle);
   

   ~plotBar();
   
   
   void show(const maxMinXY& zoomDim);
   void hide();
   bool isVisable();
   void moveToFront();
   
   bool isSelectionCloseToBar( const QPointF& pos, 
                               const maxMinXY& zoomDim, 
                               const int canvasWidth_pixels,
                               const int canvasHeight_pixels,
                               double* delta);

   void moveBar(const QPointF& pos);
   
   void updateZoom(const maxMinXY& zoomDim, bool skipReplot = false);

   double getBarPos(){return m_curBarPos;}
   
private:
   // Eliminate default, copy, assign
   plotBar();
   plotBar(plotBar const&);
   void operator=(plotBar const&);


   void init( QwtPlot* parentPlot,
              QString barName,
              eAxis barAxis,
              QColor color,
              qreal width,
              Qt::PenStyle penStyle,
              QwtPlotCurve::CurveStyle curveStyle,
              double barSelectThresh_pixels);
   
   void setBarPoints(double newBarPos, const maxMinXY& zoomDim);
   
   QwtPlot* m_parentPlot;
   QwtPlotCurve* m_curve;
   
   eAxis m_barAxis;
   bool m_isVisable;
   
   double m_barSelectThresh_pixels;
   
   maxMinXY m_zoomDim;
   
   double m_curBarPos;
   double m_xPoints[2];
   double m_yPoints[2];
   
   
   
};

#endif // PLOTBAR_H
