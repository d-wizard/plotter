/* Copyright 2013 Dan Williams. All Rights Reserved.
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
    CurveData(const QString& curveName, const dubVect& newYPoints, const QColor& curveColor);
    CurveData(const QString& curveName, const dubVect& newXPoints, const dubVect& newYPoints, const QColor& curveColor);
    ~CurveData();

    ePlotType plotType;
    QwtPlotCurve* curve;
    unsigned int numPoints;

    QColor color;

    QLabel* pointLabel;

    QAction* curveAction;
    QSignalMapper* mapper;

    bool displayed;

    QwtPlotCurve* getCurve();
    const double* getXPoints();
    const double* getYPoints();

    void fill1DxPoints();

    void findMaxMin();

    void initCurve();

    void setNormalizeFactor(maxMinXY desiredScale);

    void resetNormalizeFactor();

    void setCurveSamples();

    maxMinXY GetMaxMinXYOfCurve();

    maxMinXY GetMaxMinXYOfData();

    QString GetCurveTitle();
    
    void SetNewCurveSamples(dubVect& newYPoints);
    void SetNewCurveSamples(dubVect& newXPoints, dubVect& newYPoints);

    tLinearXYAxis getNormFactor();

    void getXPoints(dubVect& ioXPoints);
    void getYPoints(dubVect& ioYPoints);


private:
    CurveData();
    void init();

    dubVect xPoints;
    dubVect yPoints;
    maxMinXY maxMin;

    // Normalize parameters
    tLinearXYAxis normFactor;

    bool xNormalized;
    bool yNormalized;

};

#endif
