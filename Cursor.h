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

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include "CurveData.h"

class Cursor
{
public:
    Cursor(QwtPlot* plot, QwtSymbol::Style style):
        m_plot(plot),
        m_parentCurve(NULL),
        m_style(style),
        isAttached(false),
        m_curve(NULL),
        m_symbol(NULL),
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
            //delete m_curve;
            m_curve = NULL;
        }
    }

    void setCurve(CurveData* curve)
    {
        m_parentCurve = curve;
    }

    CurveData* getCurve()
    {
        return m_parentCurve;
    }

    void showCursor(QPointF pos)
    {
        // 1d assumed
        m_pointIndex = (int)(pos.x() + 0.5);

        if(m_pointIndex < 0)
        {
            m_pointIndex = 0;
        }
        else if(m_pointIndex >= m_parentCurve->xPoints.size())
        {
            m_pointIndex = m_parentCurve->xPoints.size()-1;
        }

        showCursor();
    }

    void showCursor()
    {
        hideCursor();

        m_symbol = new QwtSymbol( m_style,
           QBrush( m_parentCurve->color ), QPen( m_parentCurve->color, 2 ), QSize( 8, 8 ) );

        m_xPoint = m_parentCurve->xPoints[m_pointIndex];
        m_yPoint = m_parentCurve->yPoints[m_pointIndex];
        m_curve->setSymbol( m_symbol );
        m_curve->setSamples( &m_xPoint, &m_yPoint, 1);

        m_curve->attach(m_plot);
        isAttached = true;
    }

    void hideCursor()
    {
        if(isAttached)
        {
            m_curve->detach();
            isAttached = false;
        }

        if(m_symbol != NULL)
        {
            //delete m_symbol;
            m_symbol = NULL;
        }
    }

    double m_xPoint;
    double m_yPoint;

    CurveData* m_parentCurve;
    int m_pointIndex;

    bool isAttached;

private:
    QwtPlotCurve* m_curve;
    QwtSymbol* m_symbol;

    QwtPlot* m_plot;

    QwtSymbol::Style m_style;

};

#endif

