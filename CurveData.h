/* Copyright 2013 - 2014 Dan Williams. All Rights Reserved.
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
#ifndef CurveData_h
#define CurveData_h

#include <QLabel>
#include <QSignalMapper>
#include <QAction>
#include <QColor>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "PlotHelperTypes.h"

class CurveData
{
public:
   CurveData( QwtPlot* parentPlot,
              const QString& curveName,
              const dubVect& newYPoints,
              const QColor& curveColor);

   CurveData( QwtPlot* parentPlot,
              const QString& curveName,
              const dubVect& newXPoints,
              const dubVect& newYPoints,
              const QColor& curveColor);

   ~CurveData();

   const double* getXPoints();
   const double* getYPoints();
   void getXPoints(dubVect& ioXPoints);
   void getYPoints(dubVect& ioYPoints);
   QColor getColor();
   ePlotType getPlotType();
   unsigned int getNumPoints();
   maxMinXY getMaxMinXYOfCurve();
   maxMinXY getMaxMinXYOfData();
   QString getCurveTitle();
   tLinearXYAxis getNormFactor();
   bool isDisplayed();

   void attach();
   void detach();

   void setNormalizeFactor(maxMinXY desiredScale);
   void resetNormalizeFactor();
   void setCurveSamples();

   void ResetCurveSamples(dubVect& newYPoints);
   void ResetCurveSamples(dubVect& newXPoints, dubVect& newYPoints);

   void UpdateCurveSamples(dubVect& newYPoints, unsigned int sampleStartIndex);
   void UpdateCurveSamples(dubVect& newXPoints, dubVect& newYPoints, unsigned int sampleStartIndex);

   void setMath(double sampleRate = 1.0);

   QLabel* pointLabel;
   QAction* curveAction;
   QSignalMapper* mapper;

   // Indicates whether the curve is avaiable to be displayed.
   // If true, it is probably some other curve's parent.
   // If a plot has no non-hidden curves, its GUI window will not be displayed.
   bool hidden;
private:
   CurveData();
   void init();
   void fill1DxPoints();
   void findMaxMin();
   void initCurve();

   void performMathOnPoints();

   QwtPlot* m_parentPlot;
   dubVect xOrigPoints;
   dubVect yOrigPoints;
   maxMinXY maxMin;
   ePlotType plotType;
   QColor color;
   QwtPlotCurve* curve;
   unsigned int numPoints;
   bool displayed; // Indicates whether the user wants the curve to be displayed on the plot at the present time.

   // Normalize parameters
   tLinearXYAxis normFactor;

   bool xNormalized;
   bool yNormalized;

   // Math Manipulations.
   dubVect xPoints;
   dubVect yPoints;

   double samplePeriod;
};

#endif
