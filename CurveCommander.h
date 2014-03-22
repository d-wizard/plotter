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
#ifndef CurveCommander_h
#define CurveCommander_h

#include <QMap>
#include <QString>
#include <QVector>
#include <QList>
#include <QSharedPointer>
#include "CurveData.h"
#include "mainwindow.h"

typedef QMap<QString, CurveData*> tCurveDataInfo;
typedef struct{MainWindow* plotGui; tCurveDataInfo curves;}tPlotGuiCurveInfo;
typedef QMap<QString, tPlotGuiCurveInfo> tCurveCommanderInfo;

class plotGuiMain;
class ChildCurve;

class CurveCommander : public QWidget
{
    Q_OBJECT
public:
    CurveCommander(plotGuiMain* parent);
    ~CurveCommander();

    void curveUpdated(QString plotName, QString curveName, CurveData* curveData);
    void plotRemoved(QString plotName);

    bool validPlot(QString plotName);
    bool validCurve(QString plotName, QString curveName);
    CurveData* getCurveData(QString plotName, QString curveName);

    tCurveCommanderInfo& getCurveCommanderInfo();

    void create1dCurve(QString plotName, QString curveName, dubVect yPoints);
    void create2dCurve(QString plotName, QString curveName, dubVect xPoints, dubVect yPoints);
    void update1dCurve(QString plotName, QString curveName, unsigned int sampleStartIndex, dubVect yPoints);
    void update2dCurve(QString plotName, QString curveName, unsigned int sampleStartIndex, dubVect xPoints, dubVect yPoints);
    void destroyAllPlots();

    void plotWindowClose(QString plotName){emit plotWindowCloseSignal(plotName);}

    void showHidePlotGui(QString plotName);

    void createChildCurve(QString plotName, QString curveName, tParentCurveAxis yAxis); // 1D
    void createChildCurve(QString plotName, QString curveName, tParentCurveAxis xAxis, tParentCurveAxis yAxis); // 2D

private:
    CurveCommander();

    void createPlot(QString plotName);
    void notifyChildCurvesOfParentChange(QString plotName, QString curveName);
    void removeOrphanedChildCurves();

    tCurveCommanderInfo m_allCurves;
    plotGuiMain* m_plotGuiMain;

    QList<QSharedPointer<ChildCurve> > m_childCurves;
public slots:
    void plotWindowCloseSlot(QString plotName);

signals:
    void plotWindowCloseSignal(QString plotName);
};


#endif

