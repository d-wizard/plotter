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
#ifndef Cursor_h
#define Cursor_h

#include <math.h>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include "CurveData.h"
#include "PlotHelperTypes.h"

class Cursor
{
public:
    Cursor(QwtPlot* plot, QwtSymbol::Style style):
        m_xPoint(0),
        m_yPoint(0),
        m_parentCurve(NULL),
        m_pointIndex(0),
        isAttached(false),
        outOfRange(false),
        m_plot(plot),
        m_curve(NULL),
        m_style(style)
    {
        m_curve = new QwtPlotCurve("");
    }
    ~Cursor()
    {
        if(m_curve != NULL)
        {
            delete m_curve;
            m_curve = NULL;
        }
    }

    void setCurve(CurveData* curve)
    {
        m_parentCurve = curve;
        if(isAttached && m_pointIndex >= m_parentCurve->getNumPoints())
        {
            outOfRange = true;
            hideCursor();
            isAttached = true;
        }
    }

    CurveData* getCurve()
    {
        return m_parentCurve;
    }

    void showCursor(QPointF pos, maxMinXY maxMin, double displayRatio)
    {
        double xPos = pos.x();
        double yPos = pos.y();

        const double* xPoints = m_parentCurve->getXPoints();
        const double* yPoints = m_parentCurve->getYPoints();

        // The data will not be displayed on the plot 1:1, need to adjust
        // delta calculation to make the x and y delta ratio 1:1
        double width = (maxMin.maxX - maxMin.minX);
        double height = (maxMin.maxY - maxMin.minY) * displayRatio;
        double inverseWidth = 1.0/width;
        double inverseHeight = 1.0/height;

        // Initialize for 2D plot
        int minPointIndex = 0;

        if(m_parentCurve->getPlotDim() == E_PLOT_DIM_1D)
        {
            // Since 1D plot, assume the X position the user selected is close to the curve,
            // convert input position to actual X position.
            tLinear inXtoPlotX = m_parentCurve->getLinearXAxisCorrection();
            minPointIndex = (int)( ((pos.x() * inXtoPlotX.m) + inXtoPlotX.b) + 0.5);
        }

        double xDelta = fabs(xPoints[minPointIndex] - xPos)*inverseWidth;
        double yDelta = fabs(yPoints[minPointIndex] - yPos)*inverseHeight;
        double minDist = sqrt((xDelta*xDelta) + (yDelta*yDelta));

        // Initialize for 2D plot
        int startIndex = 1;
        int endIndex = m_parentCurve->getNumPoints();

        if(m_parentCurve->getPlotDim() == E_PLOT_DIM_1D)
        {
            int roundDownMinDist = (int)(minDist * width) + 1;
            startIndex = minPointIndex - roundDownMinDist;
            endIndex = minPointIndex + roundDownMinDist;
            if(startIndex < 0)
            {
                startIndex = 0;
            }
            if(endIndex > (int)m_parentCurve->getNumPoints())
            {
                endIndex = (int)m_parentCurve->getNumPoints();
            }
        }

        for(int i = startIndex; i < endIndex; ++i)
        {
            xDelta = fabs(xPoints[i] - xPos)*inverseWidth;
            yDelta = fabs(yPoints[i] - yPos)*inverseHeight;
            // Don't do sqrt if its already known that it won't be smaller than curPointDist
            if(xDelta < minDist && yDelta < minDist)
            {
                double curPointDist = sqrt((xDelta*xDelta) + (yDelta*yDelta));
                if(curPointDist < minDist)
                {
                    minDist = curPointDist;
                    minPointIndex = i;
                }
            }
        }

        m_pointIndex = minPointIndex;


        showCursor();
    }

    void showCursor()
    {
        hideCursor();
        if(m_pointIndex < m_parentCurve->getNumPoints())
        {
            m_xPoint = m_parentCurve->getXPoints()[m_pointIndex];
            m_yPoint = m_parentCurve->getYPoints()[m_pointIndex];

            // QwtPlotCurve wants to be called with new and deletes the symbol on its own.
            m_curve->setSymbol( new QwtSymbol( m_style,
                                               QBrush(m_parentCurve->getColor()),
                                               QPen( Qt::black, 1),
                                               QSize(9, 9) ) );

            m_curve->setSamples( &m_xPoint, &m_yPoint, 1);

            outOfRange = false;

            m_curve->attach(m_plot);
        }
        else
        {
            outOfRange = true;
        }
        isAttached = true;
    }

    void hideCursor()
    {
        if(isAttached)
        {
            m_curve->detach();
            isAttached = false;
        }
    }

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

