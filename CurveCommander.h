/* Copyright 2013 - 2017, 2019 Dan Williams. All Rights Reserved.
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
#include "ipBlocker.h"

// Debug Defines
//#define CHILD_MSG_GROUPING_DEBUG

#ifdef CHILD_MSG_GROUPING_DEBUG
#include "logToFile.h"
#endif

typedef QMap<QString, CurveData*> tCurveDataInfo;
typedef struct{MainWindow* plotGui; tCurveDataInfo curves;}tPlotGuiCurveInfo;
typedef QMap<QString, tPlotGuiCurveInfo> tCurveCommanderInfo;

typedef std::list<PlotMsgIdType> tParentMsgIdGroup;
typedef struct{PlotMsgIdType parentMsgID; UnpackPlotMsg* childMsg;}tChildAndParentID;


class plotGuiMain;
class curveProperties;
class createPlotFromData;
class ChildCurve;

class CurveCommander : public QWidget
{
    Q_OBJECT
public:
    CurveCommander(plotGuiMain* parent);
    ~CurveCommander();

    void curveUpdated(UnpackPlotMsg* plotMsg, CurveData* curveData, bool plotDataWasChanged);
    void curveUpdated(QString plotName, QString curveName, CurveData* curveData, unsigned int sampleStartIndex, unsigned int numPoints, PlotMsgIdType parentMsgId = PLOT_MSG_ID_TYPE_NO_PARENT_MSG);
    void plotMsgGroupRemovedWithoutBeingProcessed(plotMsgGroup* plotMsgGroup);
    void curvePropertyChanged();
    void plotRemoved(QString plotName);

    bool validPlot(QString plotName);
    bool validCurve(QString plotName, QString curveName);
    CurveData* getCurveData(QString plotName, QString curveName);
    MainWindow* getMainPlot(QString plotName);

    tCurveCommanderInfo& getCurveCommanderInfo();

    void readPlotMsg(UnpackMultiPlotMsg* plotMsg);

    void create1dCurve(QString plotName, QString curveName, ePlotType plotType, dubVect& yPoints, tCurveMathProperties* mathProps = NULL);
    void create2dCurve(QString plotName, QString curveName, dubVect& xPoints, dubVect& yPoints, tCurveMathProperties* mathProps = NULL);
    void update1dChildCurve(QString plotName, QString curveName, ePlotType plotType, unsigned int sampleStartIndex, dubVect& yPoints, PlotMsgIdType parentMsgId);
    void update2dChildCurve(QString plotName, QString curveName, unsigned int sampleStartIndex, dubVect& xPoints, dubVect& yPoints, PlotMsgIdType parentMsgId);
    void destroyAllPlots();

    void plotWindowClose(QString plotName){emit plotWindowCloseSignal(plotName);}
    void curvePropertiesGuiClose(){emit curvePropertiesGuiCloseSignal();}
    void createPlotFromDataGuiClose(){emit createPlotFromDataGuiCloseSignal();}

    void showHidePlotGui(QString plotName);

    void createChildCurve(QString plotName, QString curveName, ePlotType plotType, tParentCurveInfo yAxis); // 1D
    void createChildCurve(QString plotName, QString curveName, ePlotType plotType, tParentCurveInfo xAxis, tParentCurveInfo yAxis); // 2D

    void showCurvePropertiesGui(QString plotName = "", QString curveName = "");
    void showCreatePlotFromDataGui(QString plotName, const char* dataToPlot);

    void storePlotMsg(const char *msgPtr, unsigned int msgSize, QString& plotName, QString& curveName);
    void getStoredPlotMsgs(QVector<tStoredMsg> &storedMsgs);

    void restorePlotMsg(tStoredMsg msgToRestore, tPlotCurveName plotCurveName);

    void removeCurve(const QString& plotName, const QString& curveName);

    QVector<tPlotCurveAxis> getCurveParents(const QString& plotName, const QString& curveName);

    void unlinkChildFromParents(const QString& plotName, const QString& curveName);

    // This function should be called after the child curve has been created in MainWindow.
    void doFinalChildCurveInit(const QString& plotName, const QString& curveName);

    ipBlocker* getIpBlocker(){return &m_ipBlocker;}
private:
    CurveCommander();

    void createPlot(QString plotName);
    void notifyChildCurvesOfParentChange(QString plotName, QString curveName, unsigned int sampleStartIndex, unsigned int numPoints, PlotMsgIdType parentMsgId);
    void removeOrphanedChildCurves();

    std::list<tParentMsgIdGroup>::iterator childPlots_getParentMsgIdGroupIter(PlotMsgIdType parentMsgID);
    bool childPlots_haveAllMsgsBeenProcessed(PlotMsgIdType parentMsgID);
    void childPlots_eraseParentIdsFromProcessedList(std::list<tParentMsgIdGroup>::iterator& parentMsgIdGroup);
    void childPlots_createParentMsgIdGroup(plotMsgGroup* group);
    void childPlots_createParentMsgIdGroup(UnpackMultiPlotMsg* plotMsg);
    void childPlots_addParentMsgIdToProcessedList(PlotMsgIdType parentID);
    void childPlots_addMsgGroupToParentMsgIdProcessedList(plotMsgGroup* plotMsgGroup);
    void childPlots_addChildUpdateToList(tChildAndParentID childAndParentID);
    void childPlots_removeDuplicateChildData(UnpackMultiPlotMsg* multiPlotMsg);
    void childPlots_plot(PlotMsgIdType parentID);

    void configForSpectrumAnalyzerView(QString plotName);

#ifdef CHILD_MSG_GROUPING_DEBUG
    void childPlots_debugPrint();
#endif


    tCurveCommanderInfo m_allCurves;
    plotGuiMain* m_plotGuiMain;

    curveProperties* m_curvePropGui;

    createPlotFromData* m_createPlotFromDataGui;

    std::list<ChildCurve*> m_childCurves;

    std::list<tStoredMsg> m_storedMsgs;
    QMutex m_storedMsgsMutex;

    QMutex m_childPlots_mutex;
    std::list<tParentMsgIdGroup> m_parentMsgIdGroups;
    tParentMsgIdGroup m_processedParentMsgIDs;
    std::list<tChildAndParentID> m_queuedChildCurveMsgs;

    ipBlocker m_ipBlocker;

public slots:
    void plotWindowCloseSlot(QString plotName);
    void curvePropertiesGuiCloseSlot();
    void createPlotFromDataGuiCloseSlot();

signals:
    void plotWindowCloseSignal(QString plotName);
    void curvePropertiesGuiCloseSignal();
    void createPlotFromDataGuiCloseSignal();
};


#endif

