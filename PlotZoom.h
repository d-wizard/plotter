/* Copyright 2013 - 2017 Dan Williams. All Rights Reserved.
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
#include <qwt_scale_engine.h>
#include <limits>
#include <math.h>

#define ZOOM_OUT_PAD (0.02)
#define ZOOM_SCROLL_RESOLUTION (10000)
#define ZOOM_SCROLL_CHANGE_TINY (0.001)
#define ZOOM_SCROLL_CHANGE_SMALL (0.05)
#define ZOOM_SCROLL_CHANGE_BIG (1.0)
#define ZOOM_IN_PERCENT (0.9)
#define ZOOM_OUT_PERCENT (1.1)

class MainWindow;

class PlotZoom
{
public:
   PlotZoom(MainWindow* mainWindow, QwtPlot* qwtPlot, QScrollBar* vertScroll, QScrollBar* horzScroll);

   void SetPlotDimensions(maxMinXY plotDimensions, bool changeCausedByUserGuiInput);

   void SetZoom(maxMinXY zoomDimensions);

   void ResetPlotAxisScale();

   void VertSliderMoved();
   void HorzSliderMoved();

   void ModSliderPos(eAxis axis, double changeWindowPercent);

   void BoundZoom(maxMinXY& zoom);
   void BoundZoomXaxis(maxMinXY& zoom);
   void BoundScroll(QScrollBar* scroll, int& newPos);

   void Zoom(double zoomFactor);


   // relativeMousePos is the % (0 to 1) of where the mouse is when zoom is requested
   // Keep the relative point point at the same poisition in the canvas as before the zoom.
   void Zoom(double zoomFactor, QPointF relativeMousePos);
   void Zoom(double zoomFactor, QPointF relativeMousePos, bool holdYAxis);

   void ResetZoom();

   void changeZoomFromSavedZooms(int changeZoomDelta);

   maxMinXY getCurZoom();
   maxMinXY getCurPlotDim();

   bool m_holdZoom;
   bool m_maxHoldZoom;

   bool m_plotIs1D; // This will affect how we deal with Max Hold.

private:
   MainWindow* m_mainWindow;

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

   void SetZoom(maxMinXY zoomDimensions, bool changeCausedByUserGuiInput, bool saveZoom);

   void UpdateScrollBars();

   bool areTheyClose(double val1, double val2);

   PlotZoom();
};


#endif


