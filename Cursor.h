/* Copyright 2013 - 2014, 2017 - 2019 Dan Williams. All Rights Reserved.
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
#ifndef Cursor_h
#define Cursor_h

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include "CurveData.h"
#include "PlotHelperTypes.h"

class Cursor
{
public:
   Cursor();
   Cursor(QwtPlot* plot, QwtSymbol::Style style);
   ~Cursor();

   void setCurve(CurveData* curve);

   CurveData* getCurve();

   double showCursor(QPointF pos, maxMinXY maxMin, double displayRatio);

   void showCursor(bool accountForNormalization = false);

   void hideCursor();

   double m_xPoint;
   double m_yPoint;

   CurveData* m_parentCurve;
   unsigned int m_pointIndex;

   bool isAttached;
   bool outOfRange;

private:
   QwtPlot* m_plot;

   QwtPlotCurve* m_curve;

   QwtSymbol::Style m_style;

};

#endif

