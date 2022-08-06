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
   typedef enum
   {
      E_ZOOM_LIMIT__NONE,
      E_ZOOM_LIMIT__ABSOLUTE,
      E_ZOOM_LIMIT__FROM_MIN,
      E_ZOOM_LIMIT__FROM_MAX,
      E_ZOOM_LIMIT__INVALID
   }eZoomLimitType;

   typedef struct
   {
      eZoomLimitType limitType = E_ZOOM_LIMIT__NONE;
      double absMin = 0.0;
      double absMax = 0.0;
      double fromMinMaxVal = 0.0;
   }tZoomLimitInfo;

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

   void moveZoom(double deltaX, double deltaY, bool changeCausedByUserGuiInput);

   void SetPlotLimit(eAxis axis, double limitValue); // limitValue of 0 means no limit, positive means limit from max, negative means limit from min
   void ResetPlotLimits();

   tZoomLimitInfo GetPlotLimits(eAxis axis);
   void SetPlotLimits(eAxis axis, tZoomLimitInfo& limits);

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

   // Plot Dimension Limits
   tZoomLimitInfo m_widthLimit;
   tZoomLimitInfo m_heightLimit;

   double m_xWidthLimit = 0.0; // 0 means no limit, positive means limit from max, negative means limit from min
   double m_yHeightLimit = 0.0; // 0 means no limit, positive means limit from max, negative means limit from min

   void SetZoom(maxMinXY zoomDimensions, bool changeCausedByUserGuiInput, bool saveZoom);

   void UpdateScrollBars(bool changeCausedByUserGuiInput);

   bool areTheyClose(double val1, double val2);

   void ApplyLimits(maxMinXY &plotDimensions);

   PlotZoom();
};


#endif


