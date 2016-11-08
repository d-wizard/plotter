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
#ifndef PLOTSNRCALC_H
#define PLOTSNRCALC_H

#include "plotBar.h"


class plotSnrCalc
{
public:

   plotSnrCalc(QwtPlot* parentPlot);
   ~plotSnrCalc();


   void show(const maxMinXY& zoomDim);
   void hide();
   bool isVisable();

   void updateZoom(const maxMinXY& zoomDim);

   bool isSelectionCloseToBar( const QPointF& pos,
                               const maxMinXY& zoomDim,
                               const int canvasWidth_pixels,
                               const int canvasHeight_pixels);

   void moveBar(const QPointF& pos);

private:
   // Eliminate default, copy, assign
   plotSnrCalc();
   plotSnrCalc(plotSnrCalc const&);
   void operator=(plotSnrCalc const&);


   bool m_isVisable;

   plotBar* m_noiseBars[2];
   plotBar* m_signalBars[2];

   plotBar* m_allBars[4];

   int m_activeBarIndex;
};





#endif // PLOTSNRCALC_H
