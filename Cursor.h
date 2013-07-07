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
        m_plot(plot),
        m_parentCurve(NULL),
        m_style(style),
        isAttached(false),
        outOfRange(false),
        m_curve(NULL),
        m_xPoint(0),
        m_yPoint(0),
        m_pointIndex(0)
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
        if(isAttached && m_pointIndex >= m_parentCurve->numPoints)
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

    void showCursor(QPointF pos)
    {
        if(m_parentCurve->plotType == E_PLOT_TYPE_1D)
        {
            // 1d assumed
            m_pointIndex = (int)(pos.x() + 0.5);

            if(m_pointIndex < 0)
            {
                m_pointIndex = 0;
            }
            else if(m_pointIndex >= m_parentCurve->numPoints)
            {
                m_pointIndex = m_parentCurve->numPoints-1;
            }
        }
        else
        {
            double xPos = pos.x();
            double yPos = pos.y();

            const double* xPoints = m_parentCurve->getXPoints();
            const double* yPoints = m_parentCurve->getYPoints();

            double xDelta = fabs(xPoints[0] - xPos);
            double yDelta = fabs(yPoints[0] - yPos);
            double minDist = sqrt((xDelta*xDelta) + (yDelta*yDelta));
            int minPointIndex = 0;

            double curPointDist = 0.0;
            int numPoints = m_parentCurve->numPoints;

            for(int i = 1; i < numPoints; ++i)
            {
                xDelta = fabs(xPoints[i] - xPos);
                yDelta = fabs(yPoints[i] - yPos);
                if(xDelta < minDist && yDelta < minDist)
                {
                    curPointDist = sqrt((xDelta*xDelta) + (yDelta*yDelta));
                    if(curPointDist < minDist)
                    {
                        minDist = curPointDist;
                        minPointIndex = i;
                    }
                }
            }

            m_pointIndex = minPointIndex;

        }

        showCursor();
    }

    void showCursor()
    {
        hideCursor();
        if(m_pointIndex < m_parentCurve->numPoints)
        {
            m_xPoint = m_parentCurve->getXPoints()[m_pointIndex];
            m_yPoint = m_parentCurve->getYPoints()[m_pointIndex];

            // QwtPlotCurve wants to be called with new and deletes the symbol on its own.
            m_curve->setSymbol( new QwtSymbol( m_style,
                                               QBrush(m_parentCurve->color),
                                               QPen(m_parentCurve->color, 2),
                                               QSize(8, 8) ) );

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
    int m_pointIndex;

    bool isAttached;
    bool outOfRange;

private:
    QwtPlotCurve* m_curve;

    QwtPlot* m_plot;

    QwtSymbol::Style m_style;

};

#endif

