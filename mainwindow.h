/* Copyright 2013 - 2025 Dan Williams. All Rights Reserved.
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <list>
#include <math.h>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_scale_widget.h>

#include <QLabel>
#include <QSignalMapper>
#include <QList>
#include <QMenu>
#include <QAction>
#include <QString>
#include <QCursor>
#include <QSharedPointer>
#include <QMutex>
#include <QTimer>
#include <QSemaphore>

#include <queue>

#include "PlotHelperTypes.h"
#include "TCPMsgReader.h"
#include "PlotZoom.h"
#include "Cursor.h"
#include "CurveData.h"
#include "plotSnrCalc.h"
#include "zoomlimitsdialog.h"

#define PLOT_CANVAS_OFFSET (6)

#define MIN_DISPLAY_PRECISION (4)

class CurveCommander;
class plotGuiMain;


typedef enum
{
    E_CURSOR,
    E_DELTA_CURSOR,
    E_ZOOM
}eSelectMode;


typedef struct
{
    QAction* action;
    QSignalMapper* mapper;
}tMenuActionMapper;

class tQMenuActionMapper
{
public:
   tQMenuActionMapper(QString actionStr, QObject* _this):
      m_action(actionStr, _this),
      m_mapper(_this){}
   tQMenuActionMapper(QIcon actionIcon, QString actionStr, QObject* _this):
      m_action(actionIcon, actionStr, _this),
      m_mapper(_this){}
    QAction m_action;
    QSignalMapper m_mapper;
private:
   tQMenuActionMapper();
};


class curveStyleMenuActionMapper
{
public:
   curveStyleMenuActionMapper(QwtPlotCurve::CurveStyle style, QString title, QObject* _this):
      m_qmam(title, _this),
      m_style(style){}
   curveStyleMenuActionMapper(QIcon actionIcon, QwtPlotCurve::CurveStyle style, QString title, QObject* _this):
      m_qmam(actionIcon, title, _this),
      m_style(style){}
   tQMenuActionMapper m_qmam;
   QwtPlotCurve::CurveStyle m_style;
private:
   curveStyleMenuActionMapper();
};

class curveStyleMenu
{
public:
   curveStyleMenu(QString menuStr, QObject* _this):
      m_menu(menuStr)
   {
      m_actionMapper.push_back(new curveStyleMenuActionMapper(QwtPlotCurve::Lines, "Lines", _this));
      m_actionMapper.push_back(new curveStyleMenuActionMapper(QwtPlotCurve::Dots, "Dots", _this));
      m_actionMapper.push_back(new curveStyleMenuActionMapper(QwtPlotCurve::Sticks, "Sticks", _this));
      m_actionMapper.push_back(new curveStyleMenuActionMapper(QwtPlotCurve::Steps, "Steps", _this));
   }
   ~curveStyleMenu()
   {
      for(int i = 0; i < m_actionMapper.size(); ++i)
      {
         delete m_actionMapper[i];
      }
   }

   QMenu m_menu;
   QVector<curveStyleMenuActionMapper*> m_actionMapper;
private:
   curveStyleMenu();
};


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(CurveCommander* curveCmdr, plotGuiMain* plotGui, QString plotName, bool startInScrollMode = false, QWidget *parent = 0);
    ~MainWindow();

    void readPlotMsg(plotMsgGroup* plotMsg);

    void setCurveSampleRate(QString curveName, double sampleRate, bool userSpecified);

    void setCurveProperties(QString curveName, eAxis axis, double sampleRate, tMathOpList& mathOps);
    void setCurveProperties_allCurves(double sampleRate, tMathOpList& mathOps, bool overwrite, bool replaceFromTop, int numOpsToReplace);
    void setCurveProperties_allAxes(QString curveName, double sampleRate, tMathOpList& mathOps, bool overwrite, bool replaceFromTop, int numOpsToReplace);
    void setSampleRate_allCurves(double sampleRate);

    void setCurveVisibleHidden(QString curveName, bool visible, bool hidden);

    // Return current zoom dimemsions if pointer is value, otherwise return zero values
    maxMinXY getZoomDimensions(){ maxMinXY zeroRetVal = {0,0,0,0,true,true}; return m_plotZoom ? m_plotZoom->getCurZoom() : zeroRetVal; }

    int getNumCurves();
    int getCurveIndex(const QString& curveTitle);
    void setCurveIndex(const QString& curveTitle, int newIndex);

    void removeCurve(const QString& curveName);

    unsigned int findNextUnusedColorIndex();

    void setDisplayIoMapIp(std::stringstream &iostr);
    void clearDisplayIoMapIp(std::stringstream& iostr);
    bool setDisplayIoMapipXAxis(std::stringstream& iostr, CurveData *curve);
    void setDisplayIoMapipYAxis(std::stringstream& iostr);

    void toggleCurveVisability(const QString& curveName);

    void setLegendState(bool showLegend);

    void plotZoomDimChanged(const tMaxMinXY& plotDimensions, const tMaxMinXY& zoomDimensions, bool xAxisZoomChanged, bool changeCausedByUserGuiInput);

    QString getPlotName(){return m_plotName;}

    void setScrollMode(bool newState, int size = 0);
    void externalZoomReset();
    void setSnrBarMode(bool newState);

    double getFftMeasurement(eFftSigNoiseMeasurements type);
    double getCurveStat(QString& curveName, eCurveStats type);
    bool areFftMeasurementsVisible();

    void saveCurveAppearance(QString curveName, CurveAppearance& appearance);

    bool closeSubWindows(); // Returns true if sub-windows were closed.

    bool m_spectrumAnalyzerViewSet;
private:
    // Define indexes for the array of delta cursor labels.
    enum
    {
       DELTA_LABEL_ANCHORED,
       DELTA_LABEL_SEP1,
       DELTA_LABEL_CURRENT,
       DELTA_LABEL_SEP2,
       DELTA_LABEL_DELTA,
       DELTA_LABEL_NUM_LABELS
    }eDeltaLabelsTypes;

    Ui::MainWindow *ui;

    QString m_plotName;

    CurveCommander* m_curveCommander;
    plotGuiMain* m_plotGuiMain;


    QwtPlot* m_qwtPlot;
    QList<CurveData*> m_qwtCurves;
    QMutex m_qwtCurvesMutex;
    Cursor* m_qwtSelectedSample;
    Cursor* m_qwtSelectedSampleDelta;
    QwtPlotPicker* m_qwtMainPicker;
    QwtPlotPicker* m_qwtDragZoomModePicker;
    QwtPlotPicker* m_qwtDragZoomModePicker2;
    QwtPlotGrid* m_qwtGrid;

    std::queue<plotMsgGroup*> m_plotMsgQueue;
    QMutex m_plotMsgQueueMutex;

    eSelectMode m_selectMode;

    int m_selectedCurveIndex;

    bool m_userHasSpecifiedZoomType;
    PlotZoom* m_plotZoom;
    maxMinXY m_maxMin;

    QIcon m_checkedIcon;
    QCursor* m_zoomCursor;

    bool m_normalizeCurves_xAxis;
    bool m_normalizeCurves_yAxis;

    bool m_legendDisplayed;
    bool m_calcSnrDisplayed;
    bool m_specAnFuncDisplayed;

    int m_canvasWidth_pixels;
    int m_canvasHeight_pixels;
    double m_canvasXOverYRatio;

    bool m_allowNewCurves;

    bool m_scrollMode;

    bool m_needToUpdateGuiOnNextPlotUpdate;

    bool m_cursorCanSelectAnyCurve;

    QList<tMenuActionMapper> m_selectedCursorActions;

    QAction m_zoomAction;
    QAction m_cursorAction;
    QAction m_deltaCursorAction;
    QAction m_autoZoomAction;
    QAction m_holdZoomAction;
    QAction m_maxHoldZoomAction;
    QAction m_setZoomLimitsAction;
    QAction m_clearZoomLimitsAction;
    QAction m_scrollModeAction;
    QAction m_scrollModeChangePlotSizeAction;
    QAction m_clearAllSamplesAction;
    QAction m_resetZoomAction;
    QAction m_normalizeNoneAction;
    QAction m_normalizeYOnlyAction;
    QAction m_normalizeXOnlyAction;
    QAction m_normalizeBothAction;
    QAction m_toggleLegendAction;
    QAction m_toggleSnrCalcAction;
    QAction m_toggleSpecAnAction;
    QAction m_toggleCursorCanSelectAnyCurveAction;
    QMenu m_rightClickMenu;
    QMenu m_normalizeMenu;
    QMenu m_zoomSettingsMenu;
    QMenu m_selectedCurvesMenu;
    QMenu m_visibleCurvesMenu;
    QAction m_visCurveAllAction;
    QAction m_visCurveNoneAction;
    QAction m_visCurveInvertAction;
    QAction m_visCurveSelectedAction;
    QMenu m_stylesCurvesMenu;

    tQMenuActionMapper m_2dPointFirstAction;
    tQMenuActionMapper m_2dPointLastAction;
    tQMenuActionMapper m_2dPointCopyToClipboardAction;
    QMenu m_2dPointMenu;

    QAction m_enableDisablePlotUpdate;

    QAction m_showSetSampleRateDialogAction;
    QAction m_curveProperties;

    QwtPlotCurve::CurveStyle m_defaultCurveStyle;
    QList< QSharedPointer< curveStyleMenu> > m_curveLineMenu;

    eDisplayPointType m_displayType;
    int m_displayPrecision;
    eDisplayPointHexDec m_displayDecHexX;
    eDisplayPointHexDec m_displayDecHexY;
    QMenu m_displayPointsMenu;
    int m_displayPointsMenu_selectedCurveIndex;
    tQMenuActionMapper m_displayPointsAutoAction;
    tQMenuActionMapper m_displayPointsFixedAction;
    tQMenuActionMapper m_displayPointsScientificAction;

    tQMenuActionMapper m_displayPointsPrecisionAutoAction;
    tQMenuActionMapper m_displayPointsPrecisionUpAction;
    tQMenuActionMapper m_displayPointsPrecisionDownAction;
    tQMenuActionMapper m_displayPointsPrecisionUpBigAction;
    tQMenuActionMapper m_displayPointsPrecisionDownBigAction;
    tQMenuActionMapper m_displayPointsClearCurveSamples;
    tQMenuActionMapper m_displayPointsCopyToClipboard;

    QMenu m_displayPointsHexMenu;
    tQMenuActionMapper m_displayPointsHexOffX;
    tQMenuActionMapper m_displayPointsHexUnsignedX;
    tQMenuActionMapper m_displayPointsHexSignedX;
    tQMenuActionMapper m_displayPointsHexOffY;
    tQMenuActionMapper m_displayPointsHexUnsignedY;
    tQMenuActionMapper m_displayPointsHexSignedY;

    QTimer m_activityIndicator_timer;
    bool m_activityIndicator_plotIsActive;
    bool m_activityIndicator_indicatorState;
    unsigned int m_activityIndicator_inactiveCount;
    QPalette m_activityIndicator_onEnabledPallet;
    QPalette m_activityIndicator_onDisabledPallet;
    QPalette m_activityIndicator_offPallet;
    QMutex m_activityIndicator_mutex;
    void resetActivityIndicator();

    plotSnrCalc* m_snrCalcBars;
    QPointF m_snrBarStartPoint;

    bool m_dragZoomModeActive;
    QMutex m_dragZoomModeMutex;
    QPointF m_dragZoomMode_startPoint;
    QPointF m_dragZoomMode_newPoint;

    bool m_moveCalcSnrBarActive;
    bool m_showCalcSnrBarCursor;

    bool m_debouncePointSelected;

    QLabel* m_deltaLabels[DELTA_LABEL_NUM_LABELS];

    // Zoom Limit Stuff
    zoomLimitsDialog* m_zoomLimitDialog = NULL;
    ZoomLimits m_zoomLimits;
    QMutex m_zoomLimitMutex;


    void resetPlot();

    void createUpdateCurve(UnpackPlotMsg* unpackPlotMsg);

    void initCursorIndex(int curveIndex);
    void handleCurveDataChange(int curveIndex, bool onlyPlotSizeChanged = false);
    void updatePlotWithNewCurveData(bool onlyCurveDataChanged);

    maxMinXY calcMaxMin(bool limitedZoom = false, eAxis limitedAxis = E_X_AXIS, double startValue = 0, double stopValue = 0);
    void SetZoomPlotDimensions(maxMinXY plotDimensions, bool changeCausedByUserGuiInput);

    void autoZoom();
    void holdZoom();
    void maxHoldZoom();

    void initDeltaLabels();
    void clearPointLabels();
    QPalette labelColorToPalette(QColor color);
    void displayLabelAddNum(std::stringstream& lblText, double number, eAxis axis, bool forceHex = false);
    void displayPointLabels_getLabelText(std::stringstream& lblText, unsigned curveIndex, unsigned curvePointIndex);
    void displayPointLabels_getToolTipText(std::stringstream& ss, double number, const std::string& title, const std::string& oneOverStr);
    void displayPointLabels_getToolTipText(std::stringstream& ss, const QString& curveName, double xNumber, double yNumber, bool isDelta);
    void displayPointLabels_clean();
    void displayPointLabels_update();
    void displayDeltaLabel_getLabelText(QString& anchored, QString& current, QString& delta,
         QString& ttAnchored, QString& ttCurrent, QString& ttDelta); // Tooltips.
    void displayDeltaLabel_clean();
    void displayDeltaLabel_update();
    void display2dPointDeltaLabel(bool valid, bool isDelta);
    void connectPointLabelToRightClickMenu(QLabel* label);
    void updatePointDisplay(bool onlyCurveDataChanged = false);
    void setDisplayRightClickIcons();

    void updateCursors();

    void modifySelectedCursor(int modDelta);
    void modifyCursorPos(int modDelta);

    maxMinXY getPlotDimWithNormalization();

    void setSelectedCurveIndex(int index);

    void replotMainPlot(bool changeCausedByUserGuiInput = true, bool cursorChanged = false);

    // Key Press Functions
    bool keyPressModifyZoom(int key);
    bool keyPressModifyCursor(int key);


    bool eventFilter(QObject *obj, QEvent *event);
    void closeEvent(QCloseEvent* event);
    void resizeEvent(QResizeEvent* event);


    void setCurveStyleMenu();
    void setCurveStyleMenuIcons();

    bool setCurveProperties_modify(CurveData *curveData, eAxis axis, tMathOpList& mathOps, bool replaceFromTop, int numOpsToReplace);
    void setCurveProperties_fromList(QList<CurveData*> curves, double sampleRate, tMathOpList& mathOps, bool overwrite, bool replaceFromTop, int numOpsToReplace);

    void updateAllCurveGuiPoints();
    void updateAllCurveGuiPointsReplot();

    void restorePersistentPlotParams();

    void scrollModeSetPlotSize(int newPlotSize);
    void clearCurveSamples(bool askUserViaMsgBox, bool allCurves, int singleCurveIndex = -1);
    void setDisplayedSamples(bool askUserViaMsgBox, double val); // Sets all the samples in the current zoom to a specific value.

    bool areAllCurves1D();

    void setNew2dPlotStyle(QString curveName);

    void setCurveStyleForCurve(int curveIndex, QwtPlotCurve::CurveStyle curveStyle); // curveIndex of -1 will set all curves.

    void normalizeCurves(bool xAxis, bool yAxis);

    void setCursor();

    int findIndexWithClosestPoint(const QPointF &pos, unsigned int &selectedCurvePointIndex);

    bool isSelectionCloseToBar(const QPointF& pos);

    int getCurveIndex(CurveData* ptr);

    void specAn_setTraceType(fftSpecAnFunc::eFftSpecAnTraceType newTraceType);
    void setSpecAnGuiVisible(bool visible);

    void silentSavePlotToFile();

    void fillWithSavedAppearance(QString& curveName, CurveAppearance& appearance);

    void setVisibleCurveAction(int actionType);

    void openSavePlotDialog(bool limitToZoom);

   // Functions for sorting curves / manipulating curve colors.
   void sortCurvesByName(bool reverse);

private slots:
    void pointSelected(const QPointF &pos);
    void rectSelected(const QRectF &pos);
    void pickerMoved_calcSnr(const QPointF &pos);

    void pointSelected_dragZoomMode(const QPointF &pos);
    void rectSelected_dragZoomMode(const QRectF &pos);
    void pickerMoved_dragZoomMode(const QPointF &pos);

    // Menu Commands
    void toggleLegend();
    void calcSnrToggle();
    void specAnFuncToggle();
    void cursorMode();
    void deltaCursorMode();
    void autoZoom_guiSlot();
    void holdZoom_guiSlot();
    void maxHoldZoom_guiSlot();
    void setZoomLimits_guiSlot();
    void clearZoomLimits_guiSlot();
    void scrollModeToggle();
    void scrollModeChangePlotSize();
    void clearAllSamplesSlot();
    void zoomMode();
    void resetZoom();
    void normalizeCurvesNone();
    void normalizeCurvesYOnly();
    void normalizeCurvesXOnly();
    void normalizeCurvesBoth();
    void toggleCursorCanSelectAnyCurveAction();

    void visibleCursorMenuSelect(int index);
    void selectedCursorMenuSelect(int index);

    void changeCurveStyle(int inVal);

    void showSetPlotSampleRateDialog();

    void showCurveProperties();
    void togglePlotUpdateAbility();

    void on_verticalScrollBar_sliderMoved(int position);
    void on_horizontalScrollBar_sliderMoved(int position);

    void on_verticalScrollBar_actionTriggered(int action);
    void on_horizontalScrollBar_actionTriggered(int action);


    void onApplicationFocusChanged(QWidget* old, QWidget* now);
    void ShowRightClickForPlot(const QPoint& pos);
    void ShowRightClickForDisplayPoints(const QPoint& pos);
    void ShowRightClickFor2dPointLabel(const QPoint& pos);

    void displayPointsChangeType(int type);
    void displayPointsChangePrecision(int precision);
    void displayPointsChangeDecHexX(int type);
    void displayPointsChangeDecHexY(int type);
    void displayPointsClearCurveSamples(int dummy);
    void displayPointsCopyToClipboard(int dummy);

    void set2dPointIndex(int index);
    void twoDPointsCopyToClipboard(int dummy);

    void setVisibleCurveActionAll();
    void setVisibleCurveActionNone();
    void setVisibleCurveActionInvert();
    void setVisibleCurveActionSelected();

    void updateCurveOrder();

    void activityIndicatorTimerSlot();

    // Spectrum Analyzer Like GUI Element functions.
    void on_radClearWrite_clicked();
    void on_radMaxHold_clicked();
    void on_radAverage_clicked();
    void on_spnSpecAnAvgAmount_valueChanged(int arg1);
    void on_cmdPeakSearch_clicked();
    void on_cmdSpecAnResetZoom_clicked();

public slots:
    void updateCursorMenus();
    void readPlotMsgSlot();
    void dragZoomMode_moveSlot();
signals:
    void updateCursorMenusSignal();
    void readPlotMsgSignal();
    void dragZoomMode_moveSignal();
    void externalResetZoomSignal();

};

#endif // MAINWINDOW_H
