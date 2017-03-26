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
#ifndef CurveData_h
#define CurveData_h

#include <list>

#include <QLabel>
#include <QSignalMapper>
#include <QAction>
#include <QColor>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "PlotHelperTypes.h"
#include "PackUnpackPlotMsg.h"

#include "smartMaxMin.h"

class CurveAppearance
{
public:
   CurveAppearance(QColor initColor, QwtPlotCurve::CurveStyle initStyle):
      color(initColor),
      style(initStyle){}
   QColor color;
   QwtPlotCurve::CurveStyle style;
};

class CurveData
{
public:
   CurveData( QwtPlot* parentPlot,
              const CurveAppearance& curveAppearance,
              const UnpackPlotMsg* data);

   ~CurveData();

   const double* getXPoints();
   const double* getYPoints();
   void getXPoints(dubVect& ioXPoints);
   void getYPoints(dubVect& ioYPoints);
   void getXPoints(dubVect& ioXPoints, int startIndex, int stopIndex);
   void getYPoints(dubVect& ioYPoints, int startIndex, int stopIndex);

   const double* getOrigXPoints(){return &xOrigPoints[0];}
   const double* getOrigYPoints(){return &yOrigPoints[0];}
   void getOrigXPoints(dubVect& ioXPoints){ioXPoints = xOrigPoints;}
   void getOrigYPoints(dubVect& ioYPoints){ioYPoints = yOrigPoints;}

   QColor getColor();
   QwtPlotCurve::CurveStyle getStyle(){return appearance.style;}
   ePlotDim getPlotDim();
   ePlotType getPlotType(){ return plotType;}
   unsigned int getNumPoints();
   void setNumPoints(unsigned int newNumPointsSize, bool scrollMode);
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
   void setCurveDataGuiPoints();

   void ResetCurveSamples(const UnpackPlotMsg* data);
   void UpdateCurveSamples(const UnpackPlotMsg* data, bool scrollMode);

   bool setSampleRate(double inSampleRate, bool userSpecified = true);
   double getSampleRate(){return sampleRate;}

   bool isXNormalized(){return xNormalized;}
   bool isYNormalized(){return yNormalized;}

   bool setMathOps(tMathOpList& mathOpsIn, eAxis axis);
   tMathOpList getMathOps(eAxis axis);

   tLinear getLinearXAxisCorrection(){return linearXAxisCorrection;}

   bool getHidden(){return hidden;}
   bool setHidden(bool isHidden);

   void setCurveAppearance(CurveAppearance curveAppearance);

   tPlotterIpAddr getLastMsgIpAddr(){return lastMsgIpAddr;}
   ePlotDataTypes getLastMsgDataType(eAxis axis){return axis == E_X_AXIS ? lastMsgXAxisType : lastMsgYAxisType;}

   QLabel* pointLabel;
   QAction* curveAction;
   QSignalMapper* mapper;

private:
   CurveData();
   void init();
   void fill1DxPoints();
   void findMaxMin();
   void initCurve();

   void findRealMaxMin(const dubVect& inPoints, double& max, double& min);

   void performMathOnPoints();
   void performMathOnPoints(unsigned int sampleStartIndex, unsigned int numSamples);
   void doMathOnCurve(dubVect& data, tMathOpList& mathOp, unsigned int sampleStartIndex, unsigned int numSamples);
   unsigned int removeInvalidPoints();

   void UpdateCurveSamples(const dubVect& newYPoints, unsigned int sampleStartIndex, bool scrollMode);
   void UpdateCurveSamples(const dubVect& newXPoints, const dubVect& newYPoints, unsigned int sampleStartIndex, bool scrollMode);

   void storeLastMsgStats(const UnpackPlotMsg* data);

   maxMinXY getMinMaxInRange(const dubVect& in, unsigned int start, unsigned int len);

   QwtPlot* m_parentPlot;
   dubVect xOrigPoints;
   dubVect yOrigPoints;
   smartMaxMin smartMaxMinXPoints;
   smartMaxMin smartMaxMinYPoints;
   maxMinXY maxMin_beforeScale;
   maxMinXY maxMin_1dXPoints;
   maxMinXY maxMin_finalSamples;
   ePlotDim plotDim;
   ePlotType plotType;
   CurveAppearance appearance;
   QwtPlotCurve* curve;
   unsigned int numPoints;
   bool displayed; // Indicates whether the user wants the curve to be displayed on the plot at the present time.

   // Indicates whether the curve is avaiable to be displayed.
   // If true, it is probably some other curve's parent.
   // If a plot has no non-hidden curves, its GUI window will not be displayed.
   bool hidden;

   // Normalize parameters
   tLinearXYAxis normFactor;

   bool xNormalized;
   bool yNormalized;

   // Math Manipulations.
   dubVect xPoints;
   dubVect yPoints;
   dubVect normX;
   dubVect normY;

   double sampleRate;
   double samplePeriod;
   bool sampleRateIsUserSpecified;

   tLinear linearXAxisCorrection;

   tMathOpList mathOpsXAxis;
   tMathOpList mathOpsYAxis;

   // Stats about the last message.
   tPlotterIpAddr lastMsgIpAddr;
   ePlotDataTypes lastMsgXAxisType;
   ePlotDataTypes lastMsgYAxisType;


};

#endif
