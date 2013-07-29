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
#ifndef CurveCommander_h
#define CurveCommander_h

#include <QMap>
#include <QString>
#include <QVector>
#include <CurveData.h>

typedef QMap<QString, CurveData*> tCurveDataInfo;
typedef QMap<QString, tCurveDataInfo> tCurveCommanderInfo;

class CurveCommander
{
public:
    CurveCommander();
    ~CurveCommander();

    void curveUpdated(QString plotName, QString curveName, CurveData* curveData);
    void plotRemoved(QString plotName);

    bool validPlot(QString plotName);
    bool validCurve(QString plotName, QString curveName);
    CurveData* getCurveData(QString plotName, QString curveName);

    tCurveCommanderInfo& getCurveCommanderInfo();


private:
    tCurveCommanderInfo m_allCurves;
};


#endif

