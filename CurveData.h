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

class CurveData
{
public:
    CurveData(const std::string& curveName, const dubVect& newYPoints):
        yPoints(newYPoints),
        plotType(E_PLOT_TYPE_1D),
        curve(new QwtPlotCurve(curveName.c_str())),
        pointLabel(NULL),
        curveAction(NULL),
        mapper(NULL),
        displayed(false),
        title(curveName){}
    CurveData(const std::string& curveName, const dubVect& newXPoints, const dubVect& newYPoints):
        xPoints(newXPoints),
        yPoints(newYPoints),
        plotType(E_PLOT_TYPE_2D),
        curve(new QwtPlotCurve(curveName.c_str())),
        pointLabel(NULL),
        curveAction(NULL),
        mapper(NULL),
        displayed(false),
        title(curveName){}
    ~CurveData()
    {
        if(curve != NULL)
        {
            //delete curve;
        }
        //delete pointLabel;
    }

    QwtPlotCurve* curve;
    dubVect xPoints;
    dubVect yPoints;
    maxMinXY maxMin;
    ePlotType plotType;

    std::string title;

    QColor color;

    QLabel* pointLabel;

    QAction* curveAction;
    QSignalMapper* mapper;

    bool displayed;

    QwtPlotCurve* getCurve(){return curve;}
private:
    CurveData();
};

#endif
