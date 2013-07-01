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

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "PlotHelperTypes.h"

class CurveData
{
public:
    CurveData(const std::string& curveName, const dubVect& newYPoints, const QColor& curveColor):
        yPoints(newYPoints),
        numPoints(0),
        plotType(E_PLOT_TYPE_1D),
        curve(new QwtPlotCurve(curveName.c_str())),
        pointLabel(NULL),
        curveAction(NULL),
        mapper(NULL),
        displayed(false),
        title(curveName),
        color(curveColor)
        {
            numPoints = yPoints.size();
            fill1DxPoints();
            findMaxMin();
            initCurve();
        }
    CurveData(const std::string& curveName, const dubVect& newXPoints, const dubVect& newYPoints, const QColor& curveColor):
        xPoints(newXPoints),
        yPoints(newYPoints),
        numPoints(0),
        plotType(E_PLOT_TYPE_2D),
        curve(new QwtPlotCurve(curveName.c_str())),
        pointLabel(NULL),
        curveAction(NULL),
        mapper(NULL),
        displayed(false),
        title(curveName),
        color(curveColor)
        {
            if(xPoints.size() > yPoints.size())
            {
                xPoints.resize(yPoints.size());
            }
            else if(xPoints.size() < yPoints.size())
            {
                yPoints.resize(xPoints.size());
            }

            numPoints = yPoints.size();
            findMaxMin();
            initCurve();
        }
    ~CurveData()
    {
        if(curve != NULL)
        {
            //delete curve;
        }
        //delete pointLabel;
    }

    QwtPlotCurve* curve;
    maxMinXY maxMin;
    ePlotType plotType;
    unsigned int numPoints;
    std::string title;

    QColor color;

    QLabel* pointLabel;

    QAction* curveAction;
    QSignalMapper* mapper;

    bool displayed;

    QwtPlotCurve* getCurve(){return curve;}
    const double* getXPoints(){return &xPoints[0];}
    const double* getYPoints(){return &yPoints[0];}

    void fill1DxPoints()
    {
        xPoints.resize(yPoints.size());
        for(int i = 0; i < xPoints.size(); ++i)
        {
            xPoints[i] = (double)i;
        }
    }

    void findMaxMin()
    {
        int vectSize = yPoints.size();
        if(plotType == E_PLOT_TYPE_1D)
        {
            maxMin.minX = 0;
            maxMin.maxX = vectSize-1;
            maxMin.minY = yPoints[0];
            maxMin.maxY = yPoints[0];

            for(int i = 1; i < vectSize; ++i)
            {
                if(maxMin.minY > yPoints[i])
                {
                    maxMin.minY = yPoints[i];
                }
                if(maxMin.maxY < yPoints[i])
                {
                    maxMin.maxY = yPoints[i];
                }
            }
        }
        else if(plotType == E_PLOT_TYPE_2D)
        {
            maxMin.minX = xPoints[0];
            maxMin.maxX = xPoints[0];
            maxMin.minY = yPoints[0];
            maxMin.maxY = yPoints[0];

            for(int i = 1; i < vectSize; ++i)
            {
                if(maxMin.minX > xPoints[i])
                {
                    maxMin.minX = xPoints[i];
                }
                if(maxMin.maxX < xPoints[i])
                {
                    maxMin.maxX = xPoints[i];
                }

                if(maxMin.minY > yPoints[i])
                {
                    maxMin.minY = yPoints[i];
                }
                if(maxMin.maxY < yPoints[i])
                {
                    maxMin.maxY = yPoints[i];
                }
            }
        }
    }

    void initCurve()
    {
        curve->setPen(color);
        curve->setSamples( &xPoints[0],
                           &yPoints[0],
                           yPoints.size());
    }


private:
    CurveData();

    dubVect xPoints;
    dubVect yPoints;
};

#endif
