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
#include <QMutex>
#include <list>
#include <QSharedPointer>
#include "CurveData.h"
#include "mainwindow.h"

typedef QMap<QString, CurveData*> tCurveDataInfo;
typedef struct{MainWindow* plotGui; tCurveDataInfo curves;}tPlotGuiCurveInfo;
typedef QMap<QString, tPlotGuiCurveInfo> tCurveCommanderInfo;

class plotGuiMain;
class curveProperties;
class ChildCurve;

class CurveCommander : public QWidget
{
    Q_OBJECT
public:
    CurveCommander(plotGuiMain* parent);
    ~CurveCommander();

    void curveUpdated(QString plotName, QString curveName, CurveData* curveData, unsigned int sampleStartIndex, unsigned int numPoints);
    void curvePropertyChanged();
    void plotRemoved(QString plotName);

    bool validPlot(QString plotName);
    bool validCurve(QString plotName, QString curveName);
    CurveData* getCurveData(QString plotName, QString curveName);
    MainWindow* getMainPlot(QString plotName);

    tCurveCommanderInfo& getCurveCommanderInfo();

    void create1dCurve(QString plotName, QString curveName, ePlotType plotType, dubVect& yPoints);
    void create2dCurve(QString plotName, QString curveName, dubVect& xPoints, dubVect& yPoints);
    void update1dCurve(QString plotName, QString curveName, ePlotType plotType, unsigned int sampleStartIndex, dubVect& yPoints);
    void update2dCurve(QString plotName, QString curveName, unsigned int sampleStartIndex, dubVect& xPoints, dubVect& yPoints);
    void destroyAllPlots();

    void plotWindowClose(QString plotName){emit plotWindowCloseSignal(plotName);}
    void curvePropertiesGuiClose(){emit curvePropertiesGuiCloseSignal();}

    void showHidePlotGui(QString plotName);

    void createChildCurve(QString plotName, QString curveName, ePlotType plotType, tParentCurveInfo yAxis); // 1D
    void createChildCurve(QString plotName, QString curveName, ePlotType plotType, tParentCurveInfo xAxis, tParentCurveInfo yAxis); // 2D

    void showCurvePropertiesGui(QString plotName = "", QString curveName = "");

    void storePlotMsg(const char *msgPtr, unsigned int msgSize, QString& plotName, QString& curveName);
    void getStoredPlotMsgs(QVector<tStoredMsg> &storedMsgs);

    void restorePlotMsg(tStoredMsg msgToRestore, tPlotCurveName plotCurveName);

    void removeCurve(const QString& plotName, const QString& curveName);

    QVector<tPlotCurveAxis> getCurveParents(const QString& plotName, const QString& curveName);

    void unlinkChildFromParents(const QString& plotName, const QString& curveName);

private:
    CurveCommander();

    void createPlot(QString plotName);
    void notifyChildCurvesOfParentChange(QString plotName, QString curveName, unsigned int sampleStartIndex, unsigned int numPoints);
    void removeOrphanedChildCurves();



    tCurveCommanderInfo m_allCurves;
    plotGuiMain* m_plotGuiMain;

    curveProperties* m_curvePropGui;

    std::list<ChildCurve*> m_childCurves;

    std::list<tStoredMsg> m_storedMsgs;
    QMutex m_storedMsgsMutex;

public slots:
    void plotWindowCloseSlot(QString plotName);
    void curvePropertiesGuiCloseSlot();

signals:
    void plotWindowCloseSignal(QString plotName);
    void curvePropertiesGuiCloseSignal();
};


#endif

