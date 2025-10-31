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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <QSignalMapper>
#include <QKeyEvent>
#include <QClipboard>
#include <QInputDialog>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QtWidgets>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <QBitmap>
#include <qwt_plot_canvas.h>
#include "CurveCommander.h"
#include "plotguimain.h"
#include "dString.h"
#include "saveRestoreCurve.h"
#include "persistentPlotParameters.h"
#include "persistentParameters.h"
#include "FileSystemOperations.h"

// curveColors array is created from .h file, probably should be made into its own class at some point.
#include "curveColors.h"

#define DISPLAY_POINT_START "("
#define DISPLAY_POINT_MID   ","
#define DISPLAY_POINT_STOP  ")"
static const QString CAPITAL_DELTA = QChar(0x94, 0x03);

typedef enum
{
   E_VIS_MENU_ALL,
   E_VIS_MENU_NONE,
   E_VIS_MENU_INVERT,
   E_VIS_MENU_SELECTED,
}eVisMenuActions;

#define MAPPER_ACTION_TO_SLOT(menu, mapperAction, intVal, callback) \
menu.addAction(&mapperAction.m_action); \
mapperAction.m_mapper.setMapping(&mapperAction.m_action, (int)intVal); \
connect(&mapperAction.m_action,SIGNAL(triggered()), &mapperAction.m_mapper,SLOT(map())); \
connect(&mapperAction.m_mapper, SIGNAL(mapped(int)), SLOT(callback(int)) );

#define ACTIVITY_INDICATOR_ON_PERIOD_MS (750)
#define ACTIVITY_INDICATOR_PERIODS_TO_TIMEOUT (3)
QString activityIndicatorStr = QChar(0xCF, 0x25); // Black Circle Character.

// Create map for storing peristent plot parameters. This will be used to restore certain plot window
// parameters in the situation where a plot window is closed and then a new plot window with the same
// plot name is created.
static tPersistentPlotParamMap g_persistentPlotParams;

MainWindow::MainWindow(CurveCommander* curveCmdr, plotGuiMain* plotGui, QString plotName, bool startInScrollMode, QWidget *parent) :
   QMainWindow(parent),
   m_spectrumAnalyzerViewSet(false),
   ui(new Ui::MainWindow),
   m_plotName(plotName),
   m_curveCommander(curveCmdr),
   m_plotGuiMain(plotGui),
   m_qwtPlot(NULL),
   m_qwtCurvesMutex(QMutex::Recursive),
   m_qwtSelectedSample(NULL),
   m_qwtSelectedSampleDelta(NULL),
   m_qwtMainPicker(NULL),
   m_qwtDragZoomModePicker(NULL),
   m_qwtDragZoomModePicker2(NULL),
   m_qwtGrid(NULL),
   m_selectMode(E_CURSOR),
   m_selectedCurveIndex(0),
   m_userHasSpecifiedZoomType(false),
   m_plotZoom(NULL),
   m_checkedIcon(":/check.png"),
   m_zoomCursor(NULL),
   m_normalizeCurves_xAxis(false),
   m_normalizeCurves_yAxis(false),
   m_legendDisplayed(false),
   m_calcSnrDisplayed(false),
   m_specAnFuncDisplayed(false),
   m_canvasWidth_pixels(1),
   m_canvasHeight_pixels(1),
   m_canvasXOverYRatio(1.0),
   m_allowNewCurves(true),
   m_scrollMode(false),
   m_needToUpdateGuiOnNextPlotUpdate(false),
   m_cursorCanSelectAnyCurve(false),
   m_zoomAction("Zoom", this),
   m_cursorAction("Cursor", this),
   m_deltaCursorAction("Delta Cursor", this),
   m_autoZoomAction("Auto", this),
   m_holdZoomAction("Freeze", this),
   m_maxHoldZoomAction("Max Hold", this),
   m_setZoomLimitsAction("Set Zoom Limits", this),
   m_clearZoomLimitsAction("Clear Zoom Limits", this),
   m_scrollModeAction("Scroll Mode", this),
   m_scrollModeChangePlotSizeAction("Change Scroll Plot Size", this),
   m_clearAllSamplesAction("Clear All Samples", this),
   m_resetZoomAction("Reset Zoom", this),
   m_normalizeNoneAction("Disable", this),
   m_normalizeYOnlyAction("Y Axis Only", this),
   m_normalizeXOnlyAction("X Axis Only", this),
   m_normalizeBothAction("Both Axes", this),
   m_toggleLegendAction("Legend", this),
   m_toggleSnrCalcAction("Calculate SNR", this),
   m_toggleSpecAnAction("FFT Functions", this),
   m_toggleCursorCanSelectAnyCurveAction("Any Curve Cursor", this),
   m_normalizeMenu("Normalize Curves"),
   m_zoomSettingsMenu("Zoom Settings"),
   m_selectedCurvesMenu("Selected Curve"),
   m_visibleCurvesMenu("Visible Curves"),
   m_visCurveAllAction("Show All Curves", this),
   m_visCurveNoneAction("Hide All Curves", this),
   m_visCurveInvertAction("Toggle Visible Curves", this),
   m_visCurveSelectedAction("Only Show Selected Curve(s)", this),
   m_stylesCurvesMenu("Curve Style"),
   m_2dPointFirstAction("Select First Point", this),
   m_2dPointLastAction("Select Last Point", this),
   m_2dPointCopyToClipboardAction("Copy to Clipboard", this),
   m_2dPointMenu("2D Point Menu"),
   m_enableDisablePlotUpdate("Disable New Curves", this),
   m_curveProperties("Properties", this),
   m_defaultCurveStyle(QwtPlotCurve::Lines),
   m_displayType(E_DISPLAY_POINT_AUTO),
   m_displayPrecision(8),
   m_displayDecHexX(E_DISPLAY_POINT_DEC),
   m_displayDecHexY(E_DISPLAY_POINT_DEC),
   m_displayPointsAutoAction("Auto", this),
   m_displayPointsFixedAction("Decimal", this),
   m_displayPointsScientificAction("Scientific", this),
   m_displayPointsPrecisionAutoAction("Auto", this),
   m_displayPointsPrecisionUpAction("Precision +1", this),
   m_displayPointsPrecisionDownAction("Precision -1", this),
   m_displayPointsPrecisionUpBigAction("Precision +3", this),
   m_displayPointsPrecisionDownBigAction("Precision -3", this),
   m_displayPointsClearCurveSamples("Clear Curve Samples", this),
   m_displayPointsCopyToClipboard("Copy to Clipboard", this),
   m_displayPointsHexMenu("Hex/Dec"),
   m_displayPointsHexOffX("X Axis Dec", this),
   m_displayPointsHexUnsignedX("X Axis Unsigned Hex", this),
   m_displayPointsHexSignedX("X Axis Signed Hex", this),
   m_displayPointsHexOffY("Y Axis Dec", this),
   m_displayPointsHexUnsignedY("Y Axis Unsigned Hex", this),
   m_displayPointsHexSignedY("Y Axis Signed Hex", this),
   m_activityIndicator_plotIsActive(true),
   m_activityIndicator_indicatorState(true),
   m_activityIndicator_inactiveCount(0),
   m_snrCalcBars(NULL),
   m_snrBarStartPoint(NAN, NAN),
   m_dragZoomModeActive(false),
   m_moveCalcSnrBarActive(false),
   m_showCalcSnrBarCursor(false),
   m_debouncePointSelected(false)
{
    ui->setupUi(this);
    setWindowTitle(m_plotName);

    initDeltaLabels();

    srand((unsigned)time(0));

    QPixmap zoomCursor(":/zoomCursor.png");
    zoomCursor.setMask(zoomCursor.createMaskFromColor(QColor(0,255,0)));
    m_zoomCursor = new QCursor(zoomCursor, 5,5);

    QPalette palette = this->palette();
    palette.setColor( QPalette::WindowText, Qt::white);
    palette.setColor( QPalette::Text, Qt::white);
    this->setPalette(palette);

    // Set scroll bar colors, only seems to matter in Windows Classic display
    palette = this->palette();
    palette.setColor( QPalette::Shadow, QColor(0x505050));
    palette.setColor( QPalette::Button, QColor(0x707070));
    palette.setColor( QPalette::Light, QColor(0x505050));
    ui->verticalScrollBar->setPalette(palette);
    ui->horizontalScrollBar->setPalette(palette);

    connect(this, SIGNAL(readPlotMsgSignal()), this, SLOT(readPlotMsgSlot()), Qt::QueuedConnection);
    connect(this, SIGNAL(updateCursorMenusSignal()), this, SLOT(updateCursorMenus()), Qt::QueuedConnection);
    connect(this, SIGNAL(dragZoomMode_moveSignal()), this, SLOT(dragZoomMode_moveSlot()), Qt::QueuedConnection);
    connect(this, SIGNAL(externalResetZoomSignal()), this, SLOT(resetZoom()), Qt::QueuedConnection);

    // Connect menu commands
    connect(&m_zoomAction, SIGNAL(triggered(bool)), this, SLOT(zoomMode()));
    connect(&m_cursorAction, SIGNAL(triggered(bool)), this, SLOT(cursorMode()));
    connect(&m_resetZoomAction, SIGNAL(triggered(bool)), this, SLOT(resetZoom()));
    connect(&m_deltaCursorAction, SIGNAL(triggered(bool)), this, SLOT(deltaCursorMode()));
    connect(&m_autoZoomAction, SIGNAL(triggered(bool)), this, SLOT(autoZoom_guiSlot()));
    connect(&m_holdZoomAction, SIGNAL(triggered(bool)), this, SLOT(holdZoom_guiSlot()));
    connect(&m_maxHoldZoomAction, SIGNAL(triggered(bool)), this, SLOT(maxHoldZoom_guiSlot()));
    connect(&m_setZoomLimitsAction, SIGNAL(triggered(bool)), this, SLOT(setZoomLimits_guiSlot()));
    connect(&m_clearZoomLimitsAction, SIGNAL(triggered(bool)), this, SLOT(clearZoomLimits_guiSlot()));
    connect(&m_scrollModeAction, SIGNAL(triggered(bool)), this, SLOT(scrollModeToggle()));
    connect(&m_scrollModeChangePlotSizeAction, SIGNAL(triggered(bool)), this, SLOT(scrollModeChangePlotSize()));
    connect(&m_clearAllSamplesAction, SIGNAL(triggered(bool)), this, SLOT(clearAllSamplesSlot()));
    connect(&m_normalizeNoneAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurvesNone()));
    connect(&m_normalizeYOnlyAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurvesYOnly()));
    connect(&m_normalizeXOnlyAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurvesXOnly()));
    connect(&m_normalizeBothAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurvesBoth()));
    connect(&m_toggleLegendAction, SIGNAL(triggered(bool)), this, SLOT(toggleLegend()));
    connect(&m_toggleSnrCalcAction, SIGNAL(triggered(bool)), this, SLOT(calcSnrToggle()));
    connect(&m_toggleSpecAnAction, SIGNAL(triggered(bool)), this, SLOT(specAnFuncToggle()));
    connect(&m_toggleCursorCanSelectAnyCurveAction, SIGNAL(triggered(bool)), this, SLOT(toggleCursorCanSelectAnyCurveAction()));
    connect(&m_enableDisablePlotUpdate, SIGNAL(triggered(bool)), this, SLOT(togglePlotUpdateAbility()));
    connect(&m_curveProperties, SIGNAL(triggered(bool)), this, SLOT(showCurveProperties()));
    connect(&m_visCurveAllAction, SIGNAL(triggered(bool)), this, SLOT(setVisibleCurveActionAll()));
    connect(&m_visCurveNoneAction, SIGNAL(triggered(bool)), this, SLOT(setVisibleCurveActionNone()));
    connect(&m_visCurveInvertAction, SIGNAL(triggered(bool)), this, SLOT(setVisibleCurveActionInvert()));
    connect(&m_visCurveSelectedAction, SIGNAL(triggered(bool)), this, SLOT(setVisibleCurveActionSelected()));

    m_cursorAction.setIcon(m_checkedIcon);

    resetPlot();

    ui->verticalScrollBar->setRange(0,0);
    ui->horizontalScrollBar->setRange(0,0);
    ui->verticalScrollBar->setVisible(false);
    ui->horizontalScrollBar->setVisible(false);

    // This should only be visable when Scroll Mode is active. Since Scroll Mode
    // is defaulted to be not active, default this to be not visable.
    m_scrollModeChangePlotSizeAction.setVisible(false);

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
      this, SLOT(onApplicationFocusChanged(QWidget*,QWidget*)));

    // Initialize the right click menu.
    m_rightClickMenu.addAction(&m_zoomAction);
    m_rightClickMenu.addAction(&m_cursorAction);
    m_rightClickMenu.addAction(&m_deltaCursorAction);
    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_resetZoomAction);
    m_rightClickMenu.addMenu(&m_normalizeMenu);
    m_rightClickMenu.addAction(&m_toggleLegendAction);
    m_rightClickMenu.addAction(&m_toggleCursorCanSelectAnyCurveAction);
    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addMenu(&m_zoomSettingsMenu);
    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addMenu(&m_visibleCurvesMenu);
    m_rightClickMenu.addMenu(&m_selectedCurvesMenu);

    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_toggleSnrCalcAction);
    m_rightClickMenu.addAction(&m_toggleSpecAnAction);
    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addMenu(&m_stylesCurvesMenu);

    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_scrollModeAction);
    m_rightClickMenu.addAction(&m_scrollModeChangePlotSizeAction);
    m_rightClickMenu.addAction(&m_clearAllSamplesAction);

    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_enableDisablePlotUpdate);

    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_curveProperties);

    // Initialize the right click menu's normalize settings sub menu.
    m_normalizeMenu.addAction(&m_normalizeNoneAction);
    m_normalizeMenu.addAction(&m_normalizeYOnlyAction);
    m_normalizeMenu.addAction(&m_normalizeXOnlyAction);
    m_normalizeMenu.addAction(&m_normalizeBothAction);
    m_normalizeNoneAction.setIcon(m_checkedIcon);

    // Initialize the right click menu's zoom settings sub menu.
    m_zoomSettingsMenu.addAction(&m_autoZoomAction);
    m_zoomSettingsMenu.addAction(&m_holdZoomAction);
    m_zoomSettingsMenu.addAction(&m_maxHoldZoomAction);
    m_zoomSettingsMenu.addSeparator();
    m_zoomSettingsMenu.addAction(&m_setZoomLimitsAction);
    m_zoomSettingsMenu.addAction(&m_clearZoomLimitsAction);
    m_autoZoomAction.setIcon(m_checkedIcon);
    setDisplayRightClickIcons();

    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsAutoAction,       E_DISPLAY_POINT_AUTO,       displayPointsChangeType);
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsFixedAction,      E_DISPLAY_POINT_FIXED,      displayPointsChangeType);
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsScientificAction, E_DISPLAY_POINT_SCIENTIFIC, displayPointsChangeType);

    m_displayPointsMenu.addSeparator();

    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsPrecisionUpBigAction,   +3, displayPointsChangePrecision);
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsPrecisionUpAction,      +1, displayPointsChangePrecision);
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsPrecisionDownAction,    -1, displayPointsChangePrecision);
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsPrecisionDownBigAction, -3, displayPointsChangePrecision);
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsPrecisionAutoAction,     0, displayPointsChangePrecision);

    m_displayPointsMenu.addSeparator();
    m_displayPointsMenu.addMenu(&m_displayPointsHexMenu);
    MAPPER_ACTION_TO_SLOT(m_displayPointsHexMenu, m_displayPointsHexOffX,       E_DISPLAY_POINT_DEC,          displayPointsChangeDecHexX);
    MAPPER_ACTION_TO_SLOT(m_displayPointsHexMenu, m_displayPointsHexUnsignedX,  E_DISPLAY_POINT_UNSIGNED_HEX, displayPointsChangeDecHexX);
    MAPPER_ACTION_TO_SLOT(m_displayPointsHexMenu, m_displayPointsHexSignedX,    E_DISPLAY_POINT_SIGNED_HEX,   displayPointsChangeDecHexX);
    m_displayPointsHexMenu.addSeparator();
    MAPPER_ACTION_TO_SLOT(m_displayPointsHexMenu, m_displayPointsHexOffY,       E_DISPLAY_POINT_DEC,          displayPointsChangeDecHexY);
    MAPPER_ACTION_TO_SLOT(m_displayPointsHexMenu, m_displayPointsHexUnsignedY,  E_DISPLAY_POINT_UNSIGNED_HEX, displayPointsChangeDecHexY);
    MAPPER_ACTION_TO_SLOT(m_displayPointsHexMenu, m_displayPointsHexSignedY,    E_DISPLAY_POINT_SIGNED_HEX,   displayPointsChangeDecHexY);
    
    m_displayPointsMenu.addSeparator();
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsClearCurveSamples,   0, displayPointsClearCurveSamples);

    m_displayPointsMenu.addSeparator();
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsCopyToClipboard,     0, displayPointsCopyToClipboard);

    // 2D Point right click menu
    MAPPER_ACTION_TO_SLOT(m_2dPointMenu, m_2dPointFirstAction, 0, set2dPointIndex);
    MAPPER_ACTION_TO_SLOT(m_2dPointMenu, m_2dPointLastAction, -1, set2dPointIndex);
    m_2dPointMenu.addSeparator();
    MAPPER_ACTION_TO_SLOT(m_2dPointMenu, m_2dPointCopyToClipboardAction, 0, twoDPointsCopyToClipboard);
    ui->lbl2dPoint->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lbl2dPoint, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowRightClickFor2dPointLabel(const QPoint&)));

    // Initialize Activity Indicator.
    m_activityIndicator_onEnabledPallet = palette;
    m_activityIndicator_onEnabledPallet.setColor( QPalette::WindowText, Qt::green);

    m_activityIndicator_onDisabledPallet = palette;
    m_activityIndicator_onDisabledPallet.setColor( QPalette::WindowText, Qt::yellow);

    m_activityIndicator_offPallet = palette;
    m_activityIndicator_offPallet.setColor( QPalette::WindowText, Qt::black);

    ui->activityIndicator->setPalette(m_activityIndicator_onEnabledPallet);
    ui->activityIndicator->setText(activityIndicatorStr);

    connect(&m_activityIndicator_timer, SIGNAL(timeout()), this, SLOT(activityIndicatorTimerSlot()));

    m_activityIndicator_timer.start(ACTIVITY_INDICATOR_ON_PERIOD_MS);

    // Disable SNR Calc Action by default. It will be activated (i.e. made visable) if there is a curve
    // that is an FFT curve.
    m_toggleSnrCalcAction.setVisible(false);
    m_toggleSpecAnAction.setVisible(false);
    m_snrCalcBars = new plotSnrCalc(m_qwtPlot, ui->snrLabel);

    m_toggleCursorCanSelectAnyCurveAction.setVisible(false); // Set to Invisible until more than 1 curve is displayed

    restorePersistentPlotParams();

    setSpecAnGuiVisible(false);

    if(startInScrollMode)
    {
       scrollModeToggle(); // Toggle scroll mode to On.
    }
}

MainWindow::~MainWindow()
{
    delete m_snrCalcBars;
    m_snrCalcBars = NULL;

    for(int i = 0; i < m_selectedCursorActions.size(); ++i)
    {
        // No need to check against NULL. All values in the list
        // will be valid pointers.
        delete m_selectedCursorActions[i].action;
        delete m_selectedCursorActions[i].mapper;
    }

    m_qwtCurvesMutex.lock();
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i] != NULL)
        {
            delete m_qwtCurves[i];
            m_qwtCurves[i] = NULL;
        }
    }
    m_qwtCurvesMutex.unlock();

    if(m_qwtGrid != NULL)
    {
        delete m_qwtGrid;
        m_qwtGrid = NULL;
    }
    if(m_qwtMainPicker != NULL)
    {
        delete m_qwtMainPicker;
        m_qwtMainPicker = NULL;
    }
    if(m_qwtDragZoomModePicker != NULL)
    {
        delete m_qwtDragZoomModePicker;
        m_qwtDragZoomModePicker = NULL;
    }
    if(m_qwtDragZoomModePicker2 != NULL)
    {
        delete m_qwtDragZoomModePicker2;
        m_qwtDragZoomModePicker2 = NULL;
    }
    if(m_plotZoom != NULL)
    {
        delete m_plotZoom;
        m_plotZoom = NULL;
    }
    if(m_qwtSelectedSample != NULL)
    {
        delete m_qwtSelectedSample;
        m_qwtSelectedSample = NULL;
    }
    if(m_qwtSelectedSampleDelta != NULL)
    {
        delete m_qwtSelectedSampleDelta;
        m_qwtSelectedSampleDelta = NULL;
    }

    if(m_qwtPlot != NULL)
    {
        delete m_qwtPlot;
        m_qwtPlot = NULL;
    }

    delete m_zoomCursor;

    delete ui;

   // Make sure all plot messages that are still queued are deleted.
    m_plotMsgQueueMutex.lock();
    while(m_plotMsgQueue.size() > 0)
    {
        plotMsgGroup* plotMsg = m_plotMsgQueue.front();
        m_plotMsgQueue.pop();
        m_curveCommander->plotMsgGroupRemovedWithoutBeingProcessed(plotMsg);
        delete plotMsg;
    }
    m_plotMsgQueueMutex.unlock();

    // Delete Delta Labels
    for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
    {
       if(m_deltaLabels[i] != NULL)
       {
          delete m_deltaLabels[i];
          m_deltaLabels[i] = NULL;;
       }
    }


 }

void MainWindow::resetPlot()
{
    if(m_qwtPlot != NULL)
    {
        ui->GraphLayout->removeWidget(m_qwtPlot);
        delete m_qwtPlot;
    }
    m_qwtPlot = new QwtPlot(this);

    m_qwtPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    // TODO: Do I need to disconnect the action
    connect(m_qwtPlot, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(ShowRightClickForPlot(const QPoint&)));

    m_legendDisplayed = false;
    m_qwtPlot->insertLegend(NULL);

    if(m_plotZoom != NULL)
    {
        delete m_plotZoom;
        m_plotZoom = NULL;
    }
    m_plotZoom = new PlotZoom(this, m_qwtPlot, ui->verticalScrollBar, ui->horizontalScrollBar);

    if(m_qwtSelectedSample != NULL)
    {
        delete m_qwtSelectedSample;
        m_qwtSelectedSample = NULL;
    }
    m_qwtSelectedSample = new Cursor(m_qwtPlot, QwtSymbol::Ellipse);

    if(m_qwtSelectedSampleDelta != NULL)
    {
        delete m_qwtSelectedSampleDelta;
        m_qwtSelectedSampleDelta = NULL;
    }
    m_qwtSelectedSampleDelta = new Cursor(m_qwtPlot, QwtSymbol::Diamond);

    if(m_qwtGrid != NULL)
    {
        delete m_qwtGrid;
    }
    m_qwtGrid = new QwtPlotGrid();
    m_qwtGrid->attach( m_qwtPlot );
    m_qwtGrid->setPen(QColor(55,55,55));

    ui->GraphLayout->addWidget(m_qwtPlot);

    ///// Initialize the Main Picker. This is used for selecting points in Cursor Mode and zooming in Zoom Mode.
    if(m_qwtMainPicker != NULL)
    {
        delete m_qwtMainPicker;
    }

    m_qwtMainPicker = new QwtPlotPicker(m_qwtPlot->canvas());

    // setStateMachine wants to be called with new and handles delete.
    m_qwtMainPicker->setStateMachine(new QwtPickerDragRectMachine());//QwtPickerDragPointMachine());//

    m_qwtMainPicker->setRubberBandPen( QColor( Qt::green ) );
    m_qwtMainPicker->setTrackerPen( QColor( Qt::white ) );


    ///// Initialize the Drag Zoom Mode Pickers. There are 2 pickers that do the same thing. The first keys off
    ///// Ctrl + Mouse Left Click. The second keys Mouse Middle Click.
    if(m_qwtDragZoomModePicker != NULL)
    {
        delete m_qwtDragZoomModePicker;
    }
    m_qwtDragZoomModePicker = new QwtPlotPicker(m_qwtPlot->canvas());
    m_qwtDragZoomModePicker->setStateMachine(new QwtPickerDragRectMachine());
    m_qwtDragZoomModePicker->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::ControlModifier);
    if(m_qwtDragZoomModePicker2 != NULL)
    {
        delete m_qwtDragZoomModePicker2;
    }
    m_qwtDragZoomModePicker2 = new QwtPlotPicker(m_qwtPlot->canvas());
    m_qwtDragZoomModePicker2->setStateMachine(new QwtPickerDragRectMachine());
    m_qwtDragZoomModePicker2->setMousePattern(QwtEventPattern::MouseSelect1, Qt::MiddleButton);

    // Hacky way to let the user specify whether the default mode is zoom
    // or cursor. defaultCursorZoomModeIsZoom is defined in main.cpp
    extern bool defaultCursorZoomModeIsZoom;
    if(defaultCursorZoomModeIsZoom)
    {
       // Initialize in zoom mode
       zoomMode();
    }
    else
    {
       // Initialize in cursor mode
       cursorMode();
    }

    setCursor();

    m_qwtPlot->show();
    m_qwtGrid->show();

    // TODO: Do I need to disconnect the action
    connect(m_qwtMainPicker, SIGNAL(appended(QPointF)), this, SLOT(pointSelected(QPointF)));
    connect(m_qwtMainPicker, SIGNAL(selected(QRectF)),  this, SLOT(rectSelected(QRectF)));

    connect(m_qwtDragZoomModePicker,  SIGNAL(appended(QPointF)), this, SLOT(pointSelected_dragZoomMode(QPointF)));
    connect(m_qwtDragZoomModePicker,  SIGNAL(selected(QRectF)),  this, SLOT(rectSelected_dragZoomMode(QRectF)));
    connect(m_qwtDragZoomModePicker,  SIGNAL(moved(QPointF)),    this, SLOT(pickerMoved_dragZoomMode(QPointF)));
    connect(m_qwtDragZoomModePicker2, SIGNAL(appended(QPointF)), this, SLOT(pointSelected_dragZoomMode(QPointF)));
    connect(m_qwtDragZoomModePicker2, SIGNAL(selected(QRectF)),  this, SLOT(rectSelected_dragZoomMode(QRectF)));
    connect(m_qwtDragZoomModePicker2, SIGNAL(moved(QPointF)),    this, SLOT(pickerMoved_dragZoomMode(QPointF)));
}

void MainWindow::updateCursorMenus()
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    int numDisplayedCurves = 0;

    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        m_visibleCurvesMenu.removeAction(m_qwtCurves[i]->curveAction);

        // TODO: Do I need to disconnect the action / mapper?
        if(m_qwtCurves[i]->curveAction != NULL)
        {
            delete m_qwtCurves[i]->curveAction;
            m_qwtCurves[i]->curveAction = NULL;
        }
        if(m_qwtCurves[i]->mapper != NULL)
        {
            delete m_qwtCurves[i]->mapper;
            m_qwtCurves[i]->mapper = NULL;
        }

        if(m_qwtCurves[i]->isDisplayed())
        {
            ++numDisplayedCurves;
        }
    }
    m_visibleCurvesMenu.removeAction(&m_visCurveAllAction);
    m_visibleCurvesMenu.removeAction(&m_visCurveNoneAction);
    m_visibleCurvesMenu.removeAction(&m_visCurveInvertAction);
    m_visibleCurvesMenu.removeAction(&m_visCurveSelectedAction);
    m_visibleCurvesMenu.clear();

    for(int i = 0; i < m_selectedCursorActions.size(); ++i)
    {
        m_selectedCurvesMenu.removeAction(m_selectedCursorActions[i].action);
        // TODO: Do I need to disconnect the action / mapper?
        delete m_selectedCursorActions[i].action;
        delete m_selectedCursorActions[i].mapper;
    }
    m_selectedCursorActions.clear();


    // Handle situation where a cursor has been made invisible but it was the selected cursor
    if(m_qwtCurves[m_selectedCurveIndex]->isDisplayed() == false && numDisplayedCurves > 0)
    {
        int newSelectedCurveIndex = 0;
        for(int i = 1; i < m_qwtCurves.size(); ++i)
        {
            newSelectedCurveIndex = (m_selectedCurveIndex - i);
            if(newSelectedCurveIndex < 0)
            {
                newSelectedCurveIndex += m_qwtCurves.size();
            }
            if(m_qwtCurves[newSelectedCurveIndex]->isDisplayed())
            {

                setSelectedCurveIndex(newSelectedCurveIndex);
                updateCursors();
                break;
            }
        }
    }
    else if(numDisplayedCurves == 0)
    {
        m_qwtSelectedSample->hideCursor();
        m_qwtSelectedSampleDelta->hideCursor();
        clearPointLabels();
    }

    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i]->getHidden() == false)
        {
            if(m_qwtCurves[i]->isDisplayed())
            {
                m_qwtCurves[i]->curveAction = new QAction(m_checkedIcon, m_qwtCurves[i]->getCurveTitle(), this);
            }
            else
            {
                m_qwtCurves[i]->curveAction = new QAction(m_qwtCurves[i]->getCurveTitle(), this);
            }
            m_qwtCurves[i]->mapper = new QSignalMapper(this);

            m_qwtCurves[i]->mapper->setMapping(m_qwtCurves[i]->curveAction, i);
            connect(m_qwtCurves[i]->curveAction,SIGNAL(triggered()),
                             m_qwtCurves[i]->mapper,SLOT(map()));

            m_visibleCurvesMenu.addAction(m_qwtCurves[i]->curveAction);
            connect( m_qwtCurves[i]->mapper, SIGNAL(mapped(int)), SLOT(visibleCursorMenuSelect(int)) );

            if(m_qwtCurves[i]->isDisplayed())
            {
                tMenuActionMapper actionMapper;
                if(i == m_selectedCurveIndex)
                {
                    actionMapper.action = new QAction(m_checkedIcon, m_qwtCurves[i]->getCurveTitle(), this);
                }
                else
                {
                    actionMapper.action = new QAction(m_qwtCurves[i]->getCurveTitle(), this);
                }
                actionMapper.mapper = new QSignalMapper(this);

                actionMapper.mapper->setMapping(actionMapper.action, i);
                connect(actionMapper.action,SIGNAL(triggered()),
                                 actionMapper.mapper,SLOT(map()));

                m_selectedCurvesMenu.addAction(actionMapper.action);
                connect( actionMapper.mapper, SIGNAL(mapped(int)), SLOT(selectedCursorMenuSelect(int)) );

                m_selectedCursorActions.push_back(actionMapper);
            }

        } // if(m_qwtCurves[i]->hidden == false)

    }

    // Add the rest of the Visible Curve menu actions.
    m_visibleCurvesMenu.addSeparator();
    m_visibleCurvesMenu.addAction(&m_visCurveAllAction);
    m_visibleCurvesMenu.addAction(&m_visCurveNoneAction);
    m_visibleCurvesMenu.addAction(&m_visCurveInvertAction);
    m_visibleCurvesMenu.addAction(&m_visCurveSelectedAction);

    setCurveStyleMenu();
}


void MainWindow::readPlotMsg(plotMsgGroup* plotMsg)
{
   // If we are allowing new curve data, then push it onto the queue. Otherwise clear it out right now.
   if(m_allowNewCurves || plotMsg->m_changeCausedByUserGuiInput)
   {
      m_plotMsgQueueMutex.lock();
      m_plotMsgQueue.push(plotMsg);
      m_plotMsgQueueMutex.unlock();
      emit readPlotMsgSignal();
   }
   else
   {
      // Not allowing new curve data, clear it out now.
      // Inform parent that new curve data has been completely processed (in this case, no processing was needed).
      for(UnpackPlotMsgPtrList::iterator iter = plotMsg->m_plotMsgs.begin(); iter != plotMsg->m_plotMsgs.end(); ++iter)
      {
         UnpackPlotMsg* subPlotMsg = (*iter);
         QString curveName( subPlotMsg->m_curveName.c_str() );
         int curveIndex = getCurveIndex(curveName);
         CurveData* curveDataPtr = curveIndex >= 0 ? m_qwtCurves[curveIndex] : NULL;
         m_curveCommander->curveUpdated(plotMsg, subPlotMsg, curveDataPtr, false);
      }
      // Done with new plot messages.
      delete plotMsg;
   }

   resetActivityIndicator();
}

void MainWindow::readPlotMsgSlot()
{
   while(m_plotMsgQueue.size() > 0)
   {
      plotMsgGroup* multiPlotMsg = NULL;

      m_plotMsgQueueMutex.lock();
      if(m_plotMsgQueue.size() > 0)
      {
         multiPlotMsg = m_plotMsgQueue.front();
         m_plotMsgQueue.pop();
      }
      m_plotMsgQueueMutex.unlock();

      if(multiPlotMsg != NULL)
      {
         bool newCurveAdded = false;
         bool firstCurve = m_qwtCurves.size() == 0;
         for(UnpackPlotMsgPtrList::iterator iter = multiPlotMsg->m_plotMsgs.begin(); iter != multiPlotMsg->m_plotMsgs.end(); ++iter)
         {
            UnpackPlotMsg* plotMsg = (*iter);
            QString curveName( plotMsg->m_curveName.c_str() );

            if(getCurveIndex(curveName) < 0)
            {
               newCurveAdded = true;
            }

            createUpdateCurve(plotMsg);

            if(plotMsg->m_useCurveMathProps)
            {
               setCurveProperties(
                  curveName, E_X_AXIS, plotMsg->m_curveMathProps.sampleRate, plotMsg->m_curveMathProps.mathOpsXAxis);
               setCurveProperties(
                  curveName, E_Y_AXIS, plotMsg->m_curveMathProps.sampleRate, plotMsg->m_curveMathProps.mathOpsYAxis);
            }
         }
         updatePlotWithNewCurveData(!newCurveAdded);

         // Inform parent that a curve has been added / changed
         for(UnpackPlotMsgPtrList::iterator iter = multiPlotMsg->m_plotMsgs.begin(); iter != multiPlotMsg->m_plotMsgs.end(); ++iter)
         {
            UnpackPlotMsg* plotMsg = (*iter);
            QString curveName( plotMsg->m_curveName.c_str() );
            int curveIndex = getCurveIndex(curveName);
            m_curveCommander->curveUpdated(multiPlotMsg, plotMsg, m_qwtCurves[curveIndex], true);

            // Make sure the SNR Calc bars are updated. If a new curve is plotted that is a
            // valid SNR curve, make sure the SNR Calc action is made visable.
            bool validSnrCurve = m_snrCalcBars->curveUpdated(m_qwtCurves[curveIndex], m_qwtCurves);
            if(validSnrCurve && m_toggleSnrCalcAction.isVisible() == false)
            {
               m_toggleSnrCalcAction.setVisible(true);

               // Don't allow Specturm Analyzer Functions for Complex FFTs (they can have negative values, which don't make sense in that case)
               if(m_qwtCurves[curveIndex]->getPlotType() != E_PLOT_TYPE_COMPLEX_FFT)
               {
                  m_toggleSpecAnAction.setVisible(true);
               }
            }

            // If this is a new FFT plot / curve, initialize to Max Hold Zoom mode. This is because
            // the max / min of the Y axis can jitter around in FFT plots (especially the min of
            // the Y axis).
            if(validSnrCurve && newCurveAdded && firstCurve && !m_userHasSpecifiedZoomType && !m_plotZoom->m_maxHoldZoom)
            {
               maxHoldZoom();
            }
         }

         // Done with new plot messages.
         delete multiPlotMsg;

         if(newCurveAdded)
         {
            QMutexLocker lock(&m_qwtCurvesMutex);
            if(m_snrCalcBars->isVisable())
            {
               // If the SNR Calc Bars are visable, make sure they are displayed in front of any new curves.
               m_snrCalcBars->moveToFront();
            }

            // The GUI parameters (e.g. canvas size) do not get fully initialized right away when the curve is
            // first created. Update the points sent to the GUI to ensure the correct GUI parameters are taken
            // in to account.
            updateAllCurveGuiPointsReplot();
         }

      }
   }

}

void MainWindow::setCurveSampleRate(QString curveName, double sampleRate, bool userSpecified)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   int curveIndex = getCurveIndex(curveName);
   if(curveIndex >= 0)
   {
      if(m_qwtCurves[curveIndex]->setSampleRate(sampleRate, userSpecified))
      {
         handleCurveDataChange(curveIndex);
      }
   }
}


void MainWindow::setCurveProperties(QString curveName, eAxis axis, double sampleRate, tMathOpList& mathOps)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   int curveIndex = getCurveIndex(curveName);
   if(curveIndex >= 0)
   {
      bool curveChanged = false;
      curveChanged |= m_qwtCurves[curveIndex]->setSampleRate(sampleRate, true);
      curveChanged |= m_qwtCurves[curveIndex]->setMathOps(mathOps, axis);

      if(curveChanged)
      {
         handleCurveDataChange(curveIndex);
      }
   }
}

// Replaces some of the existing operations (either from the top or bottom of the existing list)
// with the math operations passed.
bool MainWindow::setCurveProperties_modify(CurveData* curveData, eAxis axis, tMathOpList &mathOps, bool replaceFromTop, int numOpsToReplace)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   tMathOpList origMathOps = curveData->getMathOps(axis);
   tMathOpList finalMathOps;

   if(numOpsToReplace > (int)origMathOps.size())
      numOpsToReplace = origMathOps.size();

   tMathOpList::iterator origBegin = origMathOps.begin();
   tMathOpList::iterator origEnd   = origMathOps.end();
   if(replaceFromTop)
   {
      std::advance(origBegin, numOpsToReplace);
      finalMathOps = mathOps;
      finalMathOps.insert(finalMathOps.end(), origBegin, origEnd);
   }
   else
   {
      std::advance(origEnd, -numOpsToReplace);
      finalMathOps.insert(finalMathOps.begin(), origBegin, origEnd);
      finalMathOps.insert(finalMathOps.end(), mathOps.begin(), mathOps.end());
   }

   return curveData->setMathOps(finalMathOps, axis);
}

// Either overwrite all math operations with the math operations passed in or
// replace some of the existing operations (either from the top or bottom of the existing list)
// with the math operations passed. Apply these changes to all the curves in the list.
void MainWindow::setCurveProperties_fromList(QList<CurveData*> curves, double sampleRate, tMathOpList& mathOps, bool overwrite, bool replaceFromTop, int numOpsToReplace)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   for(int curveIndex = 0; curveIndex < curves.size(); ++curveIndex)
   {
      bool curveChanged = false;

      curveChanged |= curves[curveIndex]->setSampleRate(sampleRate, true);

      if(m_qwtCurves[curveIndex]->getPlotDim() == E_PLOT_DIM_1D)
      {
         // 1D
         if(overwrite)
         {
            curveChanged |= curves[curveIndex]->setMathOps(mathOps, E_Y_AXIS);
         }
         else
         {
            curveChanged |= setCurveProperties_modify(curves[curveIndex], E_Y_AXIS, mathOps, replaceFromTop, numOpsToReplace);
         }
      }
      else
      {
         // 2D
         if(overwrite)
         {
            curveChanged |= curves[curveIndex]->setMathOps(mathOps, E_X_AXIS);
            curveChanged |= curves[curveIndex]->setMathOps(mathOps, E_Y_AXIS);
         }
         else
         {
            curveChanged |= setCurveProperties_modify(curves[curveIndex], E_X_AXIS, mathOps, replaceFromTop, numOpsToReplace);
            curveChanged |= setCurveProperties_modify(curves[curveIndex], E_Y_AXIS, mathOps, replaceFromTop, numOpsToReplace);
         }
      }

      if(curveChanged)
      {
         int modifiedCurveIndex = getCurveIndex(curves[curveIndex]->getCurveTitle());
         if(modifiedCurveIndex >= 0)
         {
            handleCurveDataChange(modifiedCurveIndex);
         }
      }
   }

}

void MainWindow::setCurveProperties_allCurves(double sampleRate, tMathOpList& mathOps, bool overwrite, bool replaceFromTop, int numOpsToReplace)
{
   // Applying to all curves, use the existing list of all the curves.
   setCurveProperties_fromList(m_qwtCurves, sampleRate, mathOps, overwrite, replaceFromTop, numOpsToReplace);
}

void MainWindow::setCurveProperties_allAxes(QString curveName, double sampleRate, tMathOpList& mathOps, bool overwrite, bool replaceFromTop, int numOpsToReplace)
{
   int index = getCurveIndex(curveName);
   if(index >= 0)
   {
      // Make a new list that contains just the one curve that we want to modify.
      QList<CurveData*> curveList;
      curveList.push_back(m_qwtCurves[index]);
      setCurveProperties_fromList(curveList, sampleRate, mathOps, overwrite, replaceFromTop, numOpsToReplace);
   }
}

void MainWindow::setSampleRate_allCurves(double sampleRate)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   for(int curveIndex = 0; curveIndex < m_qwtCurves.size(); ++curveIndex)
   {
      bool curveChanged = m_qwtCurves[curveIndex]->setSampleRate(sampleRate, true);
      if(curveChanged)
      {
         int modifiedCurveIndex = getCurveIndex(m_qwtCurves[curveIndex]->getCurveTitle());
         if(modifiedCurveIndex >= 0)
         {
            handleCurveDataChange(modifiedCurveIndex);
         }
      }
   }
}

void MainWindow::setCurveVisibleHidden(QString curveName, bool visible, bool hidden)
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    int curveIndex = getCurveIndex(curveName);
    if(curveIndex >= 0)
    {
       bool curveChanged = m_qwtCurves[curveIndex]->setVisibleHidden(visible, hidden);

       if(curveChanged)
       {
          handleCurveDataChange(curveIndex);
          updateCurveOrder();
       }
    }
}

void MainWindow::createUpdateCurve(UnpackPlotMsg* unpackPlotMsg)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   QString name = unpackPlotMsg->m_curveName.c_str();
   ePlotDim plotDim = plotActionToPlotDim(unpackPlotMsg->m_plotAction);

   bool resetCurve = false;
   switch(unpackPlotMsg->m_plotAction)
   {
      case E_CREATE_1D_PLOT:
      case E_CREATE_2D_PLOT:
         if(m_scrollMode == false) // Scroll mode overrides default Create Plot behavior.
            resetCurve = true; // Reset curves on "Create" plot messages.
      break;
      default:
         // Keep variables at their initialized values.
      break;
   }

   // Check for any reason to not allow the adding of the new curve.
   if( plotDim == E_PLOT_DIM_INVALID || unpackPlotMsg->m_yAxisValues.size() <= 0 ||
       (plotDim != E_PLOT_DIM_1D && unpackPlotMsg->m_xAxisValues.size() <= 0) )
   {
      return;
   }

   int curveIndex = getCurveIndex(name);
   if(curveIndex >= 0)
   {
      // Curve Exists.
      if(resetCurve == true)
      {
         m_qwtCurves[curveIndex]->ResetCurveSamples(unpackPlotMsg);
      }
      else
      {
         m_qwtCurves[curveIndex]->UpdateCurveSamples(unpackPlotMsg);
      }
   }
   else
   {
      // Curve Does Not Exist. Create the new curve.
      curveIndex = m_qwtCurves.size();
      int colorLookupIndex = findNextUnusedColorIndex();

      if(resetCurve == false && unpackPlotMsg->m_sampleStartIndex > 0)
      {
         // New curve, but starting in the middle.
         if(plotDim == E_PLOT_DIM_1D)
         {
            // For 1D, prepend vector with 'Fill in Point' values.
            unpackPlotMsg->m_yAxisValues.insert(unpackPlotMsg->m_yAxisValues.begin(), unpackPlotMsg->m_sampleStartIndex, FILL_IN_POINT_1D);
         }
         else
         {
            // For 2D, prepend vector with 'Fill in Point' values (i.e. values that won't show up on the plot).
            unpackPlotMsg->m_xAxisValues.insert(unpackPlotMsg->m_xAxisValues.begin(), unpackPlotMsg->m_sampleStartIndex, FILL_IN_POINT_2D);
            unpackPlotMsg->m_yAxisValues.insert(unpackPlotMsg->m_yAxisValues.begin(), unpackPlotMsg->m_sampleStartIndex, FILL_IN_POINT_2D);
         }
      }

      CurveAppearance newCurveAppearance(curveColors[colorLookupIndex], m_defaultCurveStyle);
      fillWithSavedAppearance(name, newCurveAppearance); // If the user changed the curve appearance from the defaults for this curve name, use the user values.

      // Create the new curve.
      CurveData* newCurve = new CurveData(m_qwtPlot, newCurveAppearance, unpackPlotMsg);
      m_qwtCurves.push_back(newCurve);
      if(m_scrollMode)
      {
         // Set new curve to scroll mode and make its size match the selected curve's size.
         newCurve->handleScrollModeTransitions(m_scrollMode);
         if(m_qwtCurves.size() > 1 && m_selectedCurveIndex >= 0)
         {
            newCurve->setNumPoints(m_qwtCurves[m_selectedCurveIndex]->getNumPoints());
         }
      }

      m_plotZoom->m_plotIs1D = areAllCurves1D(); // The Zoom class needs to know if there are non-1D plots for the Max Hold functionality.

      // This is a new curve. If this is a child curve, there may be some final initialization that still needs to be done.
      m_curveCommander->doFinalChildCurveInit(getPlotName(), name);

      // The curve style for 2D plots may be different from the default value (i.e. Lines).
      if(plotDim == E_PLOT_DIM_2D)
      {
         setNew2dPlotStyle(name);
      }
   }

   // If the FFT Functions GUI stuff is visible, update the FFT Average Count.
   if(ui->lblSpecAnAvgCntLabel->isVisible())
   {
      QString avgCountStr = "---"; // Default Value.

      // When in average mode, set the value to the number of FFTs that have been averaged.
      if(ui->radAverage->isChecked() && m_selectedCurveIndex >= 0 && m_selectedCurveIndex < m_qwtCurves.size())
      {
         avgCountStr = QString::number(m_qwtCurves[m_selectedCurveIndex]->specAn_getAvgCount());
      }

      ui->lblSpecAnAvgCntLabel->setText("Avg Count: " + avgCountStr);
   }

   initCursorIndex(curveIndex);
}


void MainWindow::setNew2dPlotStyle(QString curveName)
{
   // If the user overwrote the Curve Style in previous plot windows, then there is no need to force the curve style.
   // Just use the value from the previous plot window.
   bool styleInPersistentParam = false;
   if(persistentPlotParam_areThereAnyParams(g_persistentPlotParams, m_plotName))
   {
      styleInPersistentParam = persistentPlotParam_get(g_persistentPlotParams, m_plotName)->m_curveStyle.isValid();
   }

   if(styleInPersistentParam == false)
   {
      int curveIndex = getCurveIndex(curveName);
      if(curveIndex >= 0 && curveIndex < m_qwtCurves.size())
      {
         extern bool default2dPlotStyleIsLines;
         QwtPlotCurve::CurveStyle curveStyle = default2dPlotStyleIsLines ? QwtPlotCurve::Lines : QwtPlotCurve::Dots;

         m_qwtCurves[curveIndex]->setCurveAppearance( CurveAppearance(m_qwtCurves[curveIndex]->getColor(), curveStyle) );

         if(m_qwtCurves.size() == 1)
         {
            // This is the first plot, set the default for all subsequent plots to match this curve style
            m_defaultCurveStyle = curveStyle;
            setCurveStyleMenuIcons();
         }
      }
   }
}

void MainWindow::initCursorIndex(int curveIndex)
{
   if(m_qwtSelectedSample->getCurve() == NULL)
   {
      setSelectedCurveIndex(curveIndex);
   }
}

void MainWindow::handleCurveDataChange(int curveIndex, bool onlyPlotSizeChanged)
{
   initCursorIndex(curveIndex);

   updatePlotWithNewCurveData(false);

   // inform children that a curve has been added / changed
   m_curveCommander->curveUpdated( this->getPlotName(),
                                   m_qwtCurves[curveIndex]->getCurveTitle(),
                                   m_qwtCurves[curveIndex],
                                   0,
                                   onlyPlotSizeChanged ? 0 : m_qwtCurves[curveIndex]->getNumPoints(), // If only the plot size changed, don't inform the child plots of the change.
                                   PLOT_MSG_ID_TYPE_NO_PARENT_MSG,
                                   PLOT_MSG_ID_TYPE_NO_PARENT_MSG );
}

void MainWindow::updatePlotWithNewCurveData(bool onlyCurveDataChanged)
{
   if(onlyCurveDataChanged == false)
   {
      m_needToUpdateGuiOnNextPlotUpdate = true;
   }

   // Only update the GUI if no more Plot Messages are queued. If more Plot Messages are queued we
   // may as well wait until they are all processed. Basically, this avoids the processor hit that
   // is caused by updating the GUI when we know the plot is just going to change anyway.
   if(m_plotMsgQueue.size() == 0)
   {
      calcMaxMin(); // Make sure m_maxMin is updated.
      replotMainPlot(false);

      // Make sure the cursors matches the updated point.
      if(m_qwtSelectedSample->isAttached == true)
      {
         m_qwtSelectedSample->showCursor();
      }
      if(m_qwtSelectedSampleDelta->isAttached == true)
      {
         m_qwtSelectedSampleDelta->showCursor();
      }
      updatePointDisplay(!m_needToUpdateGuiOnNextPlotUpdate);

      if(m_needToUpdateGuiOnNextPlotUpdate == true)
      {
         m_needToUpdateGuiOnNextPlotUpdate = false;
         emit updateCursorMenusSignal();
      }
   }
}

void MainWindow::toggleLegend()
{
    m_legendDisplayed = !m_legendDisplayed;
    if(m_legendDisplayed)
    {
        m_toggleLegendAction.setIcon(m_checkedIcon);
        // The QWT plot handles delete of QwtLegend.
        m_qwtPlot->insertLegend(new QwtLegend());
        m_qwtPlot->updateLegend();
    }
    else
    {
        m_toggleLegendAction.setIcon(QIcon());
        m_qwtPlot->insertLegend(NULL);
        m_qwtPlot->updateLegend();
    }

    // If the plot window is closed, the next time a plot with the same name is created, it will initialize to use this value.
    persistentPlotParam_get(g_persistentPlotParams, m_plotName)->m_legend.set(m_legendDisplayed);
}


void MainWindow::calcSnrToggle()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   m_calcSnrDisplayed = !m_calcSnrDisplayed;
   if(m_calcSnrDisplayed)
   {
      m_toggleSnrCalcAction.setIcon(m_checkedIcon);
      m_snrCalcBars->show(m_plotZoom->getCurPlotDim(), m_plotZoom->getCurZoom());
   }
   else
   {
      m_toggleSnrCalcAction.setIcon(QIcon());
      m_snrCalcBars->hide();
   }

   m_curveCommander->curvePropertyChanged(); // Inform Curve Commander (useful for FFT Measurement Child Plots)

   m_qwtPlot->replot();
}

void MainWindow::specAnFuncToggle()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   m_specAnFuncDisplayed = !m_specAnFuncDisplayed;
   if(m_specAnFuncDisplayed)
   {
      m_toggleSpecAnAction.setIcon(m_checkedIcon);
   }
   else
   {
      m_toggleSpecAnAction.setIcon(QIcon());
   }
   setSpecAnGuiVisible(m_specAnFuncDisplayed);
}

void MainWindow::setLegendState(bool showLegend)
{
    if(m_legendDisplayed != showLegend)
    {
        toggleLegend();
    }
}

void MainWindow::cursorMode()
{
   eSelectMode origSelectMode = m_selectMode;

   // Config for Cursor Mode.
   m_selectMode = E_CURSOR;
   m_cursorAction.setIcon(m_checkedIcon);
   m_zoomAction.setIcon(QIcon());
   m_deltaCursorAction.setIcon(QIcon());
   m_qwtMainPicker->setStateMachine( new QwtPickerDragRectMachine() );
   m_qwtSelectedSampleDelta->hideCursor();
   setCursor();
   updatePointDisplay();
   replotMainPlot(true, true);

   // If the user selects Cursor Mode when we are already in cursor mode, hide the cursor and
   // clear out the point labels associated with the cursor. This behavior allows the user to
   // hide the cursor and point labels from the plot (i.e. the way the plot appears before
   // a cursor point is selected by the user).
   if(origSelectMode == E_CURSOR)
   {
      if(m_qwtSelectedSample != NULL)
         m_qwtSelectedSample->hideCursor();
      clearPointLabels();
   }
}


void MainWindow::deltaCursorMode()
{
    eSelectMode prevMode = m_selectMode;
    m_selectMode = E_DELTA_CURSOR;
    m_deltaCursorAction.setIcon(m_checkedIcon);
    m_zoomAction.setIcon(QIcon());
    m_cursorAction.setIcon(QIcon());

    // If we are exiting out of zoom mode we don't want to disrupt an existing
    // delta (i.e. when both delta samples have been selected).
    if(prevMode != E_ZOOM || m_qwtSelectedSampleDelta->isAttached == false)
    {
        // Move the main selected sample to the delta sample.
        m_qwtSelectedSampleDelta->setCurve(m_qwtSelectedSample->getCurve());
        m_qwtSelectedSampleDelta->determineClosestPointIndex(
            QPointF(m_qwtSelectedSample->m_xPoint,m_qwtSelectedSample->m_yPoint),
            m_maxMin,
            m_canvasXOverYRatio);
        m_qwtSelectedSampleDelta->showCursor();

        m_qwtSelectedSample->hideCursor();
    }
    setCursor();
    updatePointDisplay();
    replotMainPlot(true, true);
}


void MainWindow::zoomMode()
{
    m_selectMode = E_ZOOM;
    m_zoomAction.setIcon(m_checkedIcon);
    m_cursorAction.setIcon(QIcon());
    m_deltaCursorAction.setIcon(QIcon());
    setCursor();
}

void MainWindow::autoZoom_guiSlot()
{
   m_userHasSpecifiedZoomType = true;
   autoZoom();
}

void MainWindow::holdZoom_guiSlot()
{
   m_userHasSpecifiedZoomType = true;
   holdZoom();
}

void MainWindow::maxHoldZoom_guiSlot()
{
   m_userHasSpecifiedZoomType = true;
   maxHoldZoom();
}

void MainWindow::setZoomLimits_guiSlot()
{
   QMutexLocker lock(&m_zoomLimitMutex);
   m_zoomLimitDialog = new zoomLimitsDialog(this);

   lock.unlock();
   bool changeMade = m_zoomLimitDialog->getZoomLimits(&m_zoomLimits);
   lock.relock();

   if(changeMade)
   {
      maxMinXY maxMin = calcMaxMin();
      SetZoomPlotDimensions(maxMin, true);
   }
   delete m_zoomLimitDialog;
   m_zoomLimitDialog = NULL;
}

void MainWindow::clearZoomLimits_guiSlot()
{
   QMutexLocker lock(&m_zoomLimitMutex);

   bool changeMade = false;
   if(m_zoomLimits.GetPlotLimits(E_X_AXIS).limitType != ZoomLimits::E_ZOOM_LIMIT__NONE)
   {
      m_zoomLimits.SetPlotLimits(E_X_AXIS, ZoomLimits::tZoomLimitInfo()); // Set to default tZoomLimitInfo settings (i.e. E_ZOOM_LIMIT__NONE)
      changeMade = true;
   }
   if(m_zoomLimits.GetPlotLimits(E_Y_AXIS).limitType != ZoomLimits::E_ZOOM_LIMIT__NONE)
   {
      m_zoomLimits.SetPlotLimits(E_Y_AXIS, ZoomLimits::tZoomLimitInfo()); // Set to default tZoomLimitInfo settings (i.e. E_ZOOM_LIMIT__NONE)
      changeMade = true;
   }

   if(changeMade)
   {
      maxMinXY maxMin = calcMaxMin();
      SetZoomPlotDimensions(maxMin, true);
   }
}

void MainWindow::autoZoom()
{
   m_plotZoom->m_holdZoom = false;
   m_plotZoom->m_maxHoldZoom = false;
   m_autoZoomAction.setIcon(m_checkedIcon);
   m_holdZoomAction.setIcon(QIcon());
   m_maxHoldZoomAction.setIcon(QIcon());
   m_zoomAction.setEnabled(true);

   maxMinXY maxMin = calcMaxMin();
   SetZoomPlotDimensions(maxMin, true);
   m_plotZoom->ResetZoom();
}

void MainWindow::holdZoom()
{
   m_plotZoom->m_maxHoldZoom = false;
   m_plotZoom->m_holdZoom = true;
   m_autoZoomAction.setIcon(QIcon());
   m_holdZoomAction.setIcon(m_checkedIcon);
   m_maxHoldZoomAction.setIcon(QIcon());
}

void MainWindow::maxHoldZoom()
{
   m_autoZoomAction.setIcon(QIcon());
   m_holdZoomAction.setIcon(QIcon());
   m_maxHoldZoomAction.setIcon(m_checkedIcon);

   maxMinXY maxMin = calcMaxMin();
   SetZoomPlotDimensions(maxMin, true);
   m_plotZoom->ResetZoom();
   m_plotZoom->m_maxHoldZoom = true;
   m_plotZoom->m_holdZoom = false;
}

void MainWindow::scrollModeToggle()
{
   m_scrollMode = !m_scrollMode; // Toggle
   if(m_scrollMode)
   {
      m_scrollModeAction.setIcon(m_checkedIcon);
   }
   else
   {
      m_scrollModeAction.setIcon(QIcon());
   }
   m_scrollModeChangePlotSizeAction.setVisible(m_scrollMode);

   // Inform all the Child Curves of the new Scroll Mode state.
   QMutexLocker lock(&m_qwtCurvesMutex);
   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      m_qwtCurves[i]->handleScrollModeTransitions(m_scrollMode);
   }
   updatePlotWithNewCurveData(true);
   m_curveCommander->curvePropertyChanged(); // Inform Properies GUI that scroll mode has changed.
}

void MainWindow::scrollModeChangePlotSize()
{
   // Get current plot size (just get the plot size of the selected curve).
   int startPlotSize = 0;
   {
      QMutexLocker lock(&m_qwtCurvesMutex);
      startPlotSize = m_qwtCurves[m_selectedCurveIndex]->getNumPoints();
   }

   bool ok;
   QString dialogTitle = "Change Plot Size of " + getPlotName();
   int newPlotSize = QInputDialog::getInt(this, dialogTitle, tr("New Plot Size"), startPlotSize, 1, 0x7FFFFFFF, 1, &ok);

   if(ok && newPlotSize > 0)
   {
      scrollModeSetPlotSize(newPlotSize);
   }

}

void MainWindow::clearAllSamplesSlot()
{
   clearCurveSamples(true, true); // Clear all curve's samples, but ask the user for confirmation before doing so.
}

void MainWindow::clearCurveSamples(bool askUserViaMsgBox, bool allCurves, int singleCurveIndex)
{
   // Check if we are just clearing samples for a single curve.
   QString singleCurveTitle = "";
   if(!allCurves)
   {
      QMutexLocker lock(&m_qwtCurvesMutex);
      if(singleCurveIndex >= 0 && singleCurveIndex < m_qwtCurves.size())
         singleCurveTitle = m_qwtCurves[singleCurveIndex]->getCurveTitle();
      else
         return; // Return early on invalid singleCurveIndex (this should never happen).
   }

   bool doClearAll = true;
   if(askUserViaMsgBox)
   {
      QString title = allCurves ? "Clear ALL Curve Samples?" : "Clear Samples for Curve '" + singleCurveTitle + "' ?";
      QString text = allCurves ? "Are you sure you want to clear ALL the samples in ALL the curves?" : "Are you sure you want to clear ALL the samples in curve '" + singleCurveTitle + "' ?";

      QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, title, text, QMessageBox::Yes | QMessageBox::No);
      doClearAll = (reply == QMessageBox::Yes);
   }

   if(doClearAll)
   {
      QMutexLocker lock(&m_qwtCurvesMutex);
      size_t numCurves = m_qwtCurves.size();
      for(size_t curveIndex = 0; curveIndex < numCurves; ++curveIndex)
      {
         if(allCurves || m_qwtCurves[curveIndex]->getCurveTitle() == singleCurveTitle)
         {
            // Kinda hacky way to 'clear' samples. Set the number of samples to 1, set that
            // sample to not-a-number, then set the number of samples back to the orginal value (if in scroll mode).
            unsigned int origNumPoints = m_qwtCurves[curveIndex]->getNumPoints();
            m_qwtCurves[curveIndex]->setNumPoints(1);
            m_qwtCurves[curveIndex]->setPointValue(0, NAN);
            if(m_scrollMode)
               m_qwtCurves[curveIndex]->setNumPoints(origNumPoints);
            handleCurveDataChange(curveIndex, true);
         }
      }
   }
}

void MainWindow::setDisplayedSamples(bool askUserViaMsgBox, double val) // Sets all the samples in the current zoom to a specific value.
{
   bool doSetAll = true;
   if(askUserViaMsgBox)
   {
      QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "Set All Displayed Samples?", "Are you sure you want to set all the currently displayed samples to " + QString::number(val) + "?",
                                    QMessageBox::Yes | QMessageBox::No);
      doSetAll = (reply == QMessageBox::Yes);
   }

   if(doSetAll)
   {
      QMutexLocker lock(&m_qwtCurvesMutex);
      size_t numCurves = m_qwtCurves.size();
      for(size_t curveIndex = 0; curveIndex < numCurves; ++curveIndex)
      {
         m_qwtCurves[curveIndex]->setDisplayedPoints(val);
         handleCurveDataChange(curveIndex, true);
      }
   }
}

void MainWindow::scrollModeSetPlotSize(int newPlotSize)
{
   QMutexLocker lock(&m_qwtCurvesMutex);
   size_t numCurves = m_qwtCurves.size();
   for(size_t curveIndex = 0; curveIndex < numCurves; ++curveIndex)
   {
      m_qwtCurves[curveIndex]->setNumPoints(newPlotSize);
      handleCurveDataChange(curveIndex, true);
   }
}

bool MainWindow::areAllCurves1D()
{
   QMutexLocker lock(&m_qwtCurvesMutex);
   size_t numCurves = m_qwtCurves.size();
   for(size_t curveIndex = 0; curveIndex < numCurves; ++curveIndex)
   {
      if(m_qwtCurves[curveIndex]->getPlotDim() != E_PLOT_DIM_1D)
         return false;
   }
   return true;
}

void MainWindow::resetZoom()
{
   if(m_plotZoom->m_maxHoldZoom)
   {
      maxHoldZoom();
   }
   else
   {
      autoZoom();
   }
}

void MainWindow::normalizeCurves(bool xAxis, bool yAxis)
{
   if(xAxis != m_normalizeCurves_xAxis || yAxis != m_normalizeCurves_yAxis)
   {
      // Changed.
      m_normalizeCurves_xAxis = xAxis;
      m_normalizeCurves_yAxis = yAxis;

      replotMainPlot();
      m_plotZoom->ResetZoom();
   }

   // Make sure Icon is set correctly.
   m_normalizeNoneAction.setIcon(QIcon());
   m_normalizeYOnlyAction.setIcon(QIcon());
   m_normalizeXOnlyAction.setIcon(QIcon());
   m_normalizeBothAction.setIcon(QIcon());

   if(xAxis && yAxis)
      m_normalizeBothAction.setIcon(m_checkedIcon);
   else if(yAxis)
      m_normalizeYOnlyAction.setIcon(m_checkedIcon);
   else if(xAxis)
      m_normalizeXOnlyAction.setIcon(m_checkedIcon);
   else
      m_normalizeNoneAction.setIcon(m_checkedIcon);
}

void MainWindow::normalizeCurvesNone()
{
   normalizeCurves(false, false);
}
void MainWindow::normalizeCurvesYOnly()
{
   normalizeCurves(false, true);
}
void MainWindow::normalizeCurvesXOnly()
{
   normalizeCurves(true, false);
}
void MainWindow::normalizeCurvesBoth()
{
   normalizeCurves(true, true);
}

void MainWindow::toggleCursorCanSelectAnyCurveAction()
{
   m_cursorCanSelectAnyCurve = !m_cursorCanSelectAnyCurve;
   if(m_cursorCanSelectAnyCurve)
   {
      m_toggleCursorCanSelectAnyCurveAction.setIcon(m_checkedIcon);
   }
   else
   {
      m_toggleCursorCanSelectAnyCurveAction.setIcon(QIcon());
      setSelectedCurveIndex(m_selectedCurveIndex); // Make sure the delta point cursor moves back to the selected curve index.
      updateCursors(); // Make sure the cursors are displayed at the correct location.
   }

   // If the plot window is closed, the next time a plot with the same name is created, it will initialize to use this value.
   persistentPlotParam_get(g_persistentPlotParams, m_plotName)->m_cursorCanSelectAnyCurve.set(m_cursorCanSelectAnyCurve);
}

void MainWindow::visibleCursorMenuSelect(int index)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   // Toggle the curve that was clicked.
   m_qwtCurves[index]->setVisible(!m_qwtCurves[index]->isDisplayed());

   updateCurveOrder();
   m_curveCommander->curvePropertyChanged();
}

void MainWindow::selectedCursorMenuSelect(int index)
{
    if(m_selectedCurveIndex != index)
    {
        setSelectedCurveIndex(index);
        updateCursors();
        emit updateCursorMenusSignal();
    }
}

void MainWindow::showCurveProperties()
{
   QMutexLocker lock(&m_qwtCurvesMutex);
   m_curveCommander->showCurvePropertiesGui(getPlotName(), m_qwtCurves[m_selectedCurveIndex]->getCurveTitle());
}

void MainWindow::togglePlotUpdateAbility()
{
   m_allowNewCurves = !m_allowNewCurves;
   if(m_allowNewCurves == true)
   {
      m_enableDisablePlotUpdate.setText("Disable New Curves");
   }
   else
   {
      m_enableDisablePlotUpdate.setText("Enable New Curves");
   }
}

maxMinXY MainWindow::calcMaxMin(bool limitedZoom, eAxis limitedAxis, double startValue, double stopValue)
{
   QMutexLocker lock(&m_qwtCurvesMutex); // Make sure multiple threads can't modify m_maxMin at the same time.
   
   int i = 0;
   while(i < m_qwtCurves.size() && m_qwtCurves[i]->isDisplayed() == false)
   {
      ++i;
   }
   if(i < m_qwtCurves.size())
   {
      m_maxMin = limitedZoom ? m_qwtCurves[i]->getMaxMinXYOfLimitedCurve(limitedAxis, startValue, stopValue) : m_qwtCurves[i]->getMaxMinXYOfCurve();
      while(i < m_qwtCurves.size())
      {
         if(m_qwtCurves[i]->isDisplayed())
         {
            maxMinXY curveMaxMinXY = limitedZoom ? m_qwtCurves[i]->getMaxMinXYOfLimitedCurve(limitedAxis, startValue, stopValue) : m_qwtCurves[i]->getMaxMinXYOfCurve();

            if(curveMaxMinXY.realX)
            {
               if(m_maxMin.realX == false)
               {
                  m_maxMin.minX  = curveMaxMinXY.minX;
                  m_maxMin.maxX  = curveMaxMinXY.maxX;
               }
               else
               {
                  if(m_maxMin.minX > curveMaxMinXY.minX)
                  {
                     m_maxMin.minX = curveMaxMinXY.minX;
                  }
                  if(m_maxMin.maxX < curveMaxMinXY.maxX)
                  {
                     m_maxMin.maxX = curveMaxMinXY.maxX;
                  }
               }
               m_maxMin.realX = true;
            }

            if(curveMaxMinXY.realY)
            {
               if(m_maxMin.realY == false)
               {
                  m_maxMin.minY  = curveMaxMinXY.minY;
                  m_maxMin.maxY  = curveMaxMinXY.maxY;
               }
               else
               {
                  if(m_maxMin.maxY < curveMaxMinXY.maxY)
                  {
                     m_maxMin.maxY = curveMaxMinXY.maxY;
                  }
                  if(m_maxMin.minY > curveMaxMinXY.minY)
                  {
                     m_maxMin.minY = curveMaxMinXY.minY;
                  }
               }
               m_maxMin.realY = true;
            }
         }
         ++i;
      }
   }
   else
   {
      // TODO: Should do something when there are no curves displayed
   }
   return m_maxMin;
}

void MainWindow::SetZoomPlotDimensions(maxMinXY plotDimensions, bool changeCausedByUserGuiInput)
{
   ZoomLimits::tZoomLimitInfo xLimits = m_zoomLimits.GetPlotLimits(E_X_AXIS);
   ZoomLimits::tZoomLimitInfo yLimits = m_zoomLimits.GetPlotLimits(E_Y_AXIS);

   m_zoomLimits.ApplyLimits(plotDimensions);

   bool needToLimit = false;
   eAxis axisToLimit;
   double limitStart, limitStop;
   if(xLimits.limitType != ZoomLimits::E_ZOOM_LIMIT__NONE && yLimits.limitType == ZoomLimits::E_ZOOM_LIMIT__NONE)
   {
      // X Axis is limited, but Y Axis isn't. Reduce Y zoom to just the points that will show up.
      needToLimit = true;
      axisToLimit = E_X_AXIS;
      limitStart = plotDimensions.minX;
      limitStop  = plotDimensions.maxX;
   }
   else if(yLimits.limitType != ZoomLimits::E_ZOOM_LIMIT__NONE && xLimits.limitType == ZoomLimits::E_ZOOM_LIMIT__NONE)
   {
      // Y Axis is limited, but X Axis isn't. Reduce X zoom to just the points that will show up.
      needToLimit = true;
      axisToLimit = E_Y_AXIS;
      limitStart = plotDimensions.minY;
      limitStop  = plotDimensions.maxY;
   }
   if(needToLimit)
   {
      plotDimensions = calcMaxMin(true, axisToLimit, limitStart, limitStop);
   }

   // Actually Set the Plot Dimensions.
   m_plotZoom->SetPlotDimensions(plotDimensions, changeCausedByUserGuiInput);
}

int MainWindow::findIndexWithClosestPoint(const QPointF &pos, unsigned int& selectedCurvePointIndex)
{
   int selectCurveIndex = -1;
   QMutexLocker lock(&m_qwtCurvesMutex); // Make sure multiple threads can't modify the curves.

   if(m_qwtCurves.size() > 0)
   {
      Cursor tryCursor;
      maxMinXY curZoom = m_plotZoom->getCurZoom();
      bool firstClosestPointFound = false;

      double deltaToCurvePoint = 0.0;

      for(int i = 0; i < m_qwtCurves.size(); ++i)
      {
          if(m_qwtCurves[i]->isDisplayed())
          {
             // Determine how far we are from the closest point for this curve.
             tryCursor.setCurve(m_qwtCurves[i]);
             double tryDeltaToCurvePoint = tryCursor.determineClosestPointIndex(pos, curZoom, m_canvasXOverYRatio);

             // Check if this point should be the new closest point across all curves.
             if(firstClosestPointFound == false || tryDeltaToCurvePoint < deltaToCurvePoint)
             {
                deltaToCurvePoint = tryDeltaToCurvePoint;
                selectCurveIndex = i;
                firstClosestPointFound = true;
                selectedCurvePointIndex = tryCursor.m_pointIndex; // Return the index to the point on the closest curve.
             }
          }
      }
   }
   return selectCurveIndex;
}

bool MainWindow::isSelectionCloseToBar(const QPointF& pos)
{
   return m_snrCalcBars->isSelectionCloseToBar( pos,
                                                m_plotZoom->getCurZoom(),
                                                m_canvasWidth_pixels,
                                                m_canvasHeight_pixels );
}



void MainWindow::pointSelected(const QPointF &pos)
{
   // Check if we should switch to dragging SNR Calc Bar mode.
   if( m_showCalcSnrBarCursor || (m_selectMode != E_ZOOM && m_snrCalcBars->isVisable() && isSelectionCloseToBar(pos)) )
   {
      // The user clicked on a SNR Calc Bar. Activate the slot that tracks the cursor
      // movement while the left mouse button is held down.
      m_snrBarStartPoint = m_snrCalcBars->selectedBarPos();
      m_moveCalcSnrBarActive = true;
      connect(m_qwtMainPicker, SIGNAL(moved(QPointF)), this, SLOT(pickerMoved_calcSnr(QPointF)));
      setCursor(); // Set the cursor for moving a Calc SNR Bar.

      // We are not selecting a cursor point, instead we are dragging a SNR Calc Bar. Nothing more to do.
      return;
   }
   else
   {
      // We are not dragging an SNR Calc Bar. Make sure the slot for tracking that is disconnected.
      disconnect(m_qwtMainPicker, SIGNAL(moved(QPointF)), 0, 0);
   }

   // Update the cursor with the selected point.
   if(m_selectMode == E_CURSOR || m_selectMode == E_DELTA_CURSOR)
   {
      m_debouncePointSelected = !m_debouncePointSelected; // This function is called twice per user click. Only process on the first call.

      if(m_debouncePointSelected)
      {
         QMutexLocker lock(&m_qwtCurvesMutex); // Make sure multiple threads can't modify the curves.

         bool validPointSelected = true;

         bool anyCurve = m_cursorCanSelectAnyCurve && m_toggleCursorCanSelectAnyCurveAction.isVisible(); // If action is invisible, there is only 1 curve. So no need for Any Curve logic.
         if(anyCurve)
         {
            // Find the closest point amoung all the curves.
            unsigned int selectedCurvePointIndex = 0;
            int selectCurveIndex = findIndexWithClosestPoint(pos, selectedCurvePointIndex);

            validPointSelected = selectCurveIndex >= 0; // If curve index is negative, something when wrong.

            if(validPointSelected)
            {
               // Update the selected curve index.
               setSelectedCurveIndex(selectCurveIndex);
               updateCursorMenus(); // Make sure menus are updated with the new selected curve.
               m_qwtSelectedSample->m_pointIndex = selectedCurvePointIndex; // Set to the point index that was determined by findIndexWithClosestPoint
            }
         }
         else
         {
            // Normal select mode. Only select a point on the selected curve.
            m_qwtSelectedSample->determineClosestPointIndex(pos, m_plotZoom->getCurZoom(), m_canvasXOverYRatio);
         }

         if(validPointSelected)
         {
            m_qwtSelectedSample->showCursor(); // Update the Selected Sample cursor with the new point that was just selected by the user.
            updatePointDisplay();
            replotMainPlot(true, true);
         }
      }

   }
   else
   {
      m_debouncePointSelected = false;
   }

}


void MainWindow::rectSelected(const QRectF &pos)
{
   // This function is called when the user unclicks the left mouse button. Reset the state if a Calc SNR was being moved.
   if(m_moveCalcSnrBarActive)
   {
      m_moveCalcSnrBarActive = false;
      setCursor(); // Set the cursor back to normal.
   }
   else if(m_selectMode == E_ZOOM)
   {
      QRectF rectCopy = pos;
      maxMinXY zoomDim;

      rectCopy.getCoords(&zoomDim.minX, &zoomDim.minY, &zoomDim.maxX, &zoomDim.maxY);

      m_plotZoom->SetZoom(zoomDim);
   }
}

void MainWindow::pickerMoved_calcSnr(const QPointF &pos)
{
   m_snrCalcBars->moveBar(pos);
   m_qwtPlot->replot();
}


void MainWindow::pointSelected_dragZoomMode(const QPointF &pos)
{
   QMutexLocker lock(&m_dragZoomModeMutex);
   if(m_dragZoomModeActive == false)
   {
      m_dragZoomModeActive = true;
      setCursor(); // Set to the Drag Zoom Mode cursor.
      m_dragZoomMode_startPoint = pos;
   }
}

void MainWindow::rectSelected_dragZoomMode(const QRectF &pos)
{
   (void)pos; // Ignore unused warning.
   QMutexLocker lock(&m_dragZoomModeMutex);
   m_dragZoomModeActive = false;
   setCursor(); // Set the cursor back to its default value.
}

void MainWindow::pickerMoved_dragZoomMode(const QPointF &pos)
{
   QMutexLocker lock(&m_dragZoomModeMutex);

   if(m_dragZoomModeActive == true)
   {
      // Store off the new position of Drag Mode Zoom and signal the GUI to process the zoom change.
      m_dragZoomMode_newPoint = pos;
      emit dragZoomMode_moveSignal(); // This function may be called multiple times, but only the most recent position will be processed.
   }
}

void MainWindow::dragZoomMode_moveSlot()
{
   QMutexLocker lock(&m_dragZoomModeMutex);

   if(m_dragZoomModeActive == true)
   {
      // Change zoom such that the new point position shows the data point of the previous position.
      double xDelta  = m_dragZoomMode_startPoint.x() - m_dragZoomMode_newPoint.x();
      double yDelta = m_dragZoomMode_startPoint.y() - m_dragZoomMode_newPoint.y();
      m_plotZoom->moveZoom(xDelta, yDelta, true);
   }
}

void MainWindow::on_verticalScrollBar_sliderMoved(int /*position*/)
{
    m_plotZoom->VertSliderMoved();
}

void MainWindow::on_horizontalScrollBar_sliderMoved(int /*position*/)
{
    m_plotZoom->HorzSliderMoved();
}

void MainWindow::setDisplayIoMapIp(std::stringstream &iostr)
{
    iostr << std::setprecision(m_displayPrecision);
    switch(m_displayType)
    {
    case E_DISPLAY_POINT_FIXED:
        iostr << std::fixed;
    break;
    case E_DISPLAY_POINT_SCIENTIFIC:
        iostr << std::scientific;
    break;
    case E_DISPLAY_POINT_AUTO:
    default:
        iostr << std::resetiosflags(std::ios::floatfield);
    break;
    }

}

void MainWindow::clearDisplayIoMapIp(std::stringstream &iostr)
{
    iostr << std::setprecision(STRING_STREAM_CLEAR_PRECISION_VAL) << std::resetiosflags(std::ios::floatfield);
}

bool MainWindow::setDisplayIoMapipXAxis(std::stringstream& iostr, CurveData* curve)
{
    bool simpleXAxis = (curve->getPlotDim() == E_PLOT_DIM_1D) &&
                       ( (curve->getSampleRate() == 1.0) || (curve->getSampleRate() == 0.0) );

    if(simpleXAxis)
    {
        iostr << std::fixed << std::setprecision(0);
    }
    else
    {
        setDisplayIoMapIp(iostr);
    }

    return !simpleXAxis;
}

void MainWindow::setDisplayIoMapipYAxis(std::stringstream& iostr)
{
    setDisplayIoMapIp(iostr);
}

void MainWindow::clearPointLabels()
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    // Delete and remove labels associated with the curves.
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i]->pointLabel != NULL)
        {
            ui->InfoLayout->removeWidget(m_qwtCurves[i]->pointLabel);
            delete m_qwtCurves[i]->pointLabel;
            m_qwtCurves[i]->pointLabel = NULL;
        }
    }

    // Delete and remove Delta Labels.
    for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
    {
       if(m_deltaLabels[i] != NULL)
       {
          ui->InfoLayout->removeWidget(m_deltaLabels[i]);
          delete m_deltaLabels[i];
          m_deltaLabels[i] = NULL;;
       }
    }

    // 2D Label
    ui->lbl2dPoint->setText("");
}

QPalette MainWindow::labelColorToPalette(QColor color)
{
   QPalette palette = this->palette();
   palette.setColor(QPalette::WindowText, color);
   palette.setColor(QPalette::Text, color);
   return palette;
}

void MainWindow::displayLabelAddNum(std::stringstream& lblText, double number, eAxis axis, bool forceHex)
{
   eDisplayPointHexDec displayDecHex = (axis == E_X_AXIS) ? m_displayDecHexX : m_displayDecHexY;
   bool hex = (displayDecHex == E_DISPLAY_POINT_UNSIGNED_HEX || displayDecHex == E_DISPLAY_POINT_SIGNED_HEX);

   if(forceHex) // Allow this function to be reused to always interpret a number as hex.
   {
      displayDecHex = E_DISPLAY_POINT_UNSIGNED_HEX;
      hex = true;
   }

   if(hex)
   {
      int64_t intVal = static_cast<int64_t>(number);
      if( double(intVal) != number )
         hex = false;
   }

   if(hex)
   {
      std::stringstream ss;
      int64_t intVal = static_cast<int64_t>(number);
      if(displayDecHex == E_DISPLAY_POINT_UNSIGNED_HEX)
      {
         // Do 32 bit or 64 bit
         uint64_t uintVal = (uint64_t)intVal;
         if((uintVal >> 32) == 0xFFFFFFFF)
            uintVal &= 0xFFFFFFFF;
         ss << "0x" << std::hex << uintVal;
      }
      else if(number < 0.0)
      {
         ss << "-0x" << std::hex << -intVal;
      }
      else
      {
         ss << "0x" << std::hex << intVal;
      }
      lblText << ss.str();
   }
   else
   {
      lblText << number;
   }

}

void MainWindow::displayPointLabels_getLabelText(std::stringstream& lblText, unsigned curveIndex, unsigned curvePointIndex)
{
   CurveData* curve = m_qwtCurves[curveIndex];
   bool isSelectedCurve = ((int)curveIndex == m_selectedCurveIndex);
   bool displayFormatSet = setDisplayIoMapipXAxis(lblText, curve);

   if(isSelectedCurve)
      lblText << "<b>"; // Make Bold

   lblText << DISPLAY_POINT_START;
   displayLabelAddNum(lblText, curve->getXPoints()[curvePointIndex], E_X_AXIS);
   lblText << DISPLAY_POINT_MID;

   if(displayFormatSet == false)
   {
       setDisplayIoMapipYAxis(lblText);
       displayFormatSet = true;
   }

   displayLabelAddNum(lblText, curve->getYPoints()[curvePointIndex], E_Y_AXIS);
   lblText << DISPLAY_POINT_STOP;

   if(isSelectedCurve)
      lblText << "</b>"; // Make Not Bold Anymore
}

void MainWindow::displayPointLabels_getToolTipText(std::stringstream& ss, double number, const std::string& title, const std::string& oneOverStr)
{
   // Get the curve point value.
   bool isValidInteger = (double(static_cast<int64_t>(number)) == number);

   std::stringstream toolTip;
   toolTip << std::setprecision(m_displayPrecision); // Do use the specified precision.
   toolTip << "-------------------------------" << std::endl << title << std::endl;
   toolTip << "Decimal: " << std::fixed << number << std::endl;
   toolTip << "Scientific: " << std::scientific << number;
   if(isValidInteger)
   {
      toolTip << std::endl << "Hex: ";
      displayLabelAddNum(toolTip, number, E_X_AXIS, true); // Hard-code axis. The axis value isn't used when forceHex is set to true
   }
   if(std::isfinite(number))
      toolTip << std::endl << "Inverse (1/" << oneOverStr << "): " << std::resetiosflags(std::ios::floatfield) << double(1.0)/number;
   ss << toolTip.str();
}

void MainWindow::displayPointLabels_getToolTipText(std::stringstream& ss, const QString& curveName, double xNumber, double yNumber, bool isDelta)
{
   std::stringstream toolTip;
   QString xTitle = "X Axis";
   QString yTitle = "Y Axis";
   QString xOneOverStr = "X";
   QString yOneOverStr = "Y";
   if(isDelta)
   {
      xTitle = CAPITAL_DELTA + xTitle;
      yTitle = CAPITAL_DELTA + yTitle;
      xOneOverStr = CAPITAL_DELTA + xOneOverStr;
      yOneOverStr = CAPITAL_DELTA + yOneOverStr;
   }

   if(curveName != "")
      toolTip << "Curve: " << curveName.toStdString() << std::endl;
   displayPointLabels_getToolTipText(toolTip, xNumber, xTitle.toStdString(), xOneOverStr.toStdString());
   toolTip << std::endl;
   displayPointLabels_getToolTipText(toolTip, yNumber, yTitle.toStdString(), yOneOverStr.toStdString());
   ss << toolTip.str();
}

void MainWindow::displayPointLabels_clean()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   clearPointLabels();
   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      if(m_qwtCurves[i]->isDisplayed() && m_qwtSelectedSample->m_pointIndex < m_qwtCurves[i]->getNumPoints())
      {
         m_qwtCurves[i]->pointLabel = new QLabel("");

         std::stringstream lblText;
         displayPointLabels_getLabelText(lblText, i, m_qwtSelectedSample->m_pointIndex);

         m_qwtCurves[i]->pointLabel->setText(lblText.str().c_str());
         m_qwtCurves[i]->pointLabel->setPalette(labelColorToPalette(m_qwtCurves[i]->getColor()));

         // Set the tool tip text.
         std::stringstream toolTipText;
         displayPointLabels_getToolTipText(toolTipText, m_qwtCurves[i]->getCurveTitle(), m_qwtCurves[i]->getXPoints()[m_qwtSelectedSample->m_pointIndex], m_qwtCurves[i]->getYPoints()[m_qwtSelectedSample->m_pointIndex], false);
         m_qwtCurves[i]->pointLabel->setToolTip(toolTipText.str().c_str());

         ui->InfoLayout->addWidget(m_qwtCurves[i]->pointLabel);

         connectPointLabelToRightClickMenu(m_qwtCurves[i]->pointLabel);

      }
   }
   display2dPointDeltaLabel(true, false);
}

void MainWindow::displayPointLabels_update()
{
   bool successfulUpdate = true;
   QMutexLocker lock(&m_qwtCurvesMutex);

   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      if(m_qwtCurves[i]->isDisplayed() && m_qwtSelectedSample->m_pointIndex < m_qwtCurves[i]->getNumPoints())
      {
         if(m_qwtCurves[i]->pointLabel != NULL)
         {
            std::stringstream lblText;
            displayPointLabels_getLabelText(lblText, i, m_qwtSelectedSample->m_pointIndex);

            m_qwtCurves[i]->pointLabel->setText(lblText.str().c_str());

            // Set the tool tip text.
            std::stringstream toolTipText;
            displayPointLabels_getToolTipText(toolTipText, m_qwtCurves[i]->getCurveTitle(), m_qwtCurves[i]->getXPoints()[m_qwtSelectedSample->m_pointIndex], m_qwtCurves[i]->getYPoints()[m_qwtSelectedSample->m_pointIndex], false);
            m_qwtCurves[i]->pointLabel->setToolTip(toolTipText.str().c_str());
         }
         else
         {
            successfulUpdate = false;
            break;
         }
      }
   }
   if(successfulUpdate == false)
   {
      lock.unlock(); // Clean will want to lock the mutex.
      displayPointLabels_clean();
   }
   else
   {
      display2dPointDeltaLabel(true, false);
   }
}

void MainWindow::initDeltaLabels()
{
   for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
   {
      m_deltaLabels[i] = NULL;
   }
}

void MainWindow::displayDeltaLabel_getLabelText(QString& anchored, QString& current, QString& delta,
   QString &ttAnchored, QString &ttCurrent, QString &ttDelta) // Tooltips
{
   CurveData* curve = m_qwtSelectedSample->m_parentCurve;
   std::stringstream lblText;
   std::stringstream toolTip;

   // Get Anchored text.
   lblText.str("");
   toolTip.str("");
   lblText << DISPLAY_POINT_START;
   setDisplayIoMapipXAxis(lblText, curve);
   displayLabelAddNum(lblText, m_qwtSelectedSampleDelta->m_xPoint, E_X_AXIS);
   lblText << DISPLAY_POINT_MID;
   setDisplayIoMapipYAxis(lblText);
   displayLabelAddNum(lblText, m_qwtSelectedSampleDelta->m_yPoint, E_Y_AXIS);
   lblText << DISPLAY_POINT_STOP;
   anchored = QString(lblText.str().c_str());
   displayPointLabels_getToolTipText(toolTip, m_qwtSelectedSampleDelta->m_parentCurve->getCurveTitle(), m_qwtSelectedSampleDelta->m_xPoint, m_qwtSelectedSampleDelta->m_yPoint, false);
   ttAnchored = toolTip.str().c_str();

   // Get Current text.
   lblText.str("");
   toolTip.str("");
   lblText << DISPLAY_POINT_START;
   setDisplayIoMapipXAxis(lblText, curve);
   displayLabelAddNum(lblText, m_qwtSelectedSample->m_xPoint, E_X_AXIS);
   lblText << DISPLAY_POINT_MID;
   setDisplayIoMapipYAxis(lblText);
   displayLabelAddNum(lblText, m_qwtSelectedSample->m_yPoint, E_Y_AXIS);
   lblText << DISPLAY_POINT_STOP;
   current = QString(lblText.str().c_str());
   displayPointLabels_getToolTipText(toolTip, m_qwtSelectedSample->m_parentCurve->getCurveTitle(), m_qwtSelectedSample->m_xPoint, m_qwtSelectedSample->m_yPoint, false);
   ttCurrent = toolTip.str().c_str();

   // Get Delta text.
   lblText.str("");
   toolTip.str("");
   lblText << DISPLAY_POINT_START;
   setDisplayIoMapipXAxis(lblText, curve);
   displayLabelAddNum(lblText, (m_qwtSelectedSample->m_xPoint-m_qwtSelectedSampleDelta->m_xPoint), E_X_AXIS);
   lblText << DISPLAY_POINT_MID;
   setDisplayIoMapipYAxis(lblText);
   displayLabelAddNum(lblText, (m_qwtSelectedSample->m_yPoint-m_qwtSelectedSampleDelta->m_yPoint), E_Y_AXIS);
   lblText << DISPLAY_POINT_STOP;
   delta = QString(lblText.str().c_str());
   QString deltaCurveName = m_qwtSelectedSample->m_parentCurve == m_qwtSelectedSampleDelta->m_parentCurve ? m_qwtSelectedSample->m_parentCurve->getCurveTitle() : "";
   displayPointLabels_getToolTipText(toolTip, deltaCurveName, m_qwtSelectedSample->m_xPoint-m_qwtSelectedSampleDelta->m_xPoint, m_qwtSelectedSample->m_yPoint-m_qwtSelectedSampleDelta->m_yPoint, true);
   ttDelta = toolTip.str().c_str();
}

void MainWindow::displayDeltaLabel_clean()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   clearPointLabels(); // If previous Delta Labels exist, delte them.
   if(m_qwtCurves[m_selectedCurveIndex]->isDisplayed())
   {
      // Create the Delta Labels
      for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
      {
         m_deltaLabels[i] = new QLabel();
      }

      // Set the Delta Label text.
      displayDeltaLabel_update();
      m_deltaLabels[DELTA_LABEL_SEP1]->setText(":");
      m_deltaLabels[DELTA_LABEL_SEP2]->setText(CAPITAL_DELTA);

      QPalette anchoredPalette = labelColorToPalette(m_qwtSelectedSampleDelta->getCurve()->getColor());
      QPalette currentPalette  = labelColorToPalette(m_qwtSelectedSample->getCurve()->getColor());
      QPalette nullPalette     = labelColorToPalette(Qt::white);

      m_deltaLabels[DELTA_LABEL_ANCHORED]->setPalette(anchoredPalette);
      m_deltaLabels[DELTA_LABEL_CURRENT ]->setPalette(currentPalette);

      // If Delta and Current Selected Curves match, set the rest of the labels to match the same color.
      // Otherwise set them to white.
      bool sameCurve = m_qwtSelectedSampleDelta->getCurve() == m_qwtSelectedSample->getCurve();
      QPalette* otherDeltaLabelPalette = sameCurve ? &currentPalette : &nullPalette;
      m_deltaLabels[DELTA_LABEL_DELTA]->setPalette(*otherDeltaLabelPalette);
      m_deltaLabels[DELTA_LABEL_SEP1 ]->setPalette(*otherDeltaLabelPalette);
      m_deltaLabels[DELTA_LABEL_SEP2 ]->setPalette(*otherDeltaLabelPalette);

      // Attach the Delta Labels
      for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
      {
         ui->InfoLayout->addWidget(m_deltaLabels[i]);
         connectPointLabelToRightClickMenu(m_deltaLabels[i]);
      }
   }

}

void MainWindow::displayDeltaLabel_update()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   QString anchored, current, delta, ttAnchored, ttCurrent, ttDelta;
   displayDeltaLabel_getLabelText(anchored, current, delta, ttAnchored, ttCurrent, ttDelta);

   bool deltaLabelsAreValid = true;
   for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
   {
      deltaLabelsAreValid = deltaLabelsAreValid && (m_deltaLabels[i] != NULL);
   }

   if(deltaLabelsAreValid)
   {
      m_deltaLabels[DELTA_LABEL_ANCHORED]->setText(anchored);
      m_deltaLabels[DELTA_LABEL_ANCHORED]->setToolTip(ttAnchored);
      m_deltaLabels[DELTA_LABEL_CURRENT]->setText(current);
      m_deltaLabels[DELTA_LABEL_CURRENT]->setToolTip(ttCurrent);
      m_deltaLabels[DELTA_LABEL_DELTA]->setText(delta);
      m_deltaLabels[DELTA_LABEL_DELTA]->setToolTip(ttDelta);
   }
   display2dPointDeltaLabel(deltaLabelsAreValid, true);
}

void MainWindow::display2dPointDeltaLabel(bool valid, bool isDelta)
{
   bool twoDCurveIsDisplayed = false;
   bool fftCurveIsDisplayed = false;
   bool xAxisDoesntMatchPointIndex = false;
   QString deltaCurveTitle = (isDelta ? m_qwtSelectedSampleDelta->getCurve()->getCurveTitle() : "");

   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      bool validNonDeltaCurve = (m_qwtCurves[i]->isDisplayed() && m_qwtSelectedSample->m_pointIndex < m_qwtCurves[i]->getNumPoints());
      bool validDeltaCurve_select = (i == m_selectedCurveIndex);
      bool validDeltaCurve_delta  = (m_qwtCurves[i]->getCurveTitle() == deltaCurveTitle);

      if((validNonDeltaCurve && !isDelta) || ((validDeltaCurve_select || validDeltaCurve_delta) && isDelta))
      {
         auto sampRate = m_qwtCurves[i]->getSampleRate();
         auto plotDim =  m_qwtCurves[i]->getPlotDim();
         auto plotType = m_qwtCurves[i]->getPlotType();

         if(plotDim == E_PLOT_DIM_2D)
            twoDCurveIsDisplayed = true;
         else if(plotDim == E_PLOT_DIM_1D && sampRate != 1.0 && sampRate != 0.0) // Setting the sample rate can make the x axis not match the point index.
            xAxisDoesntMatchPointIndex = true;

         switch(plotType)
         {
            case E_PLOT_TYPE_REAL_FFT:
            case E_PLOT_TYPE_COMPLEX_FFT:
            case E_PLOT_TYPE_DB_POWER_FFT_REAL:
            case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
               fftCurveIsDisplayed = true;
            break;
            default:
               // Nothing to do.
            break;
         }
      }
   }
   valid = valid && (twoDCurveIsDisplayed || xAxisDoesntMatchPointIndex) && !fftCurveIsDisplayed;
   if(!valid)
   {
      ui->lbl2dPoint->setText("");
   }
   else if(isDelta)
   {
      ui->lbl2dPoint->setText("Point indexes " + 
         QString::number(m_qwtSelectedSampleDelta->m_pointIndex) + " : " +
         QString::number(m_qwtSelectedSample->m_pointIndex) + " " + CAPITAL_DELTA + " " +
         QString::number((int64_t)m_qwtSelectedSample->m_pointIndex - (int64_t)m_qwtSelectedSampleDelta->m_pointIndex));
   }
   else
   {
      ui->lbl2dPoint->setText("Point index " + QString::number(m_qwtSelectedSample->m_pointIndex));
   }
}

void MainWindow::connectPointLabelToRightClickMenu(QLabel* label)
{
   label->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(label, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowRightClickForDisplayPoints(const QPoint&)));
}

void MainWindow::updatePointDisplay(bool onlyCurveDataChanged)
{
   if(onlyCurveDataChanged)
   {
      if(m_qwtSelectedSampleDelta->isAttached && m_qwtSelectedSample->isAttached)
      {
         displayDeltaLabel_update();
      }
      else if(!m_qwtSelectedSampleDelta->isAttached && m_qwtSelectedSample->isAttached)
      {
         displayPointLabels_update();
      }
   }
   else
   {
      clearPointLabels();
      if(m_qwtSelectedSampleDelta->isAttached && m_qwtSelectedSample->isAttached)
      {
         displayDeltaLabel_clean();
      }
      else if(!m_qwtSelectedSampleDelta->isAttached && m_qwtSelectedSample->isAttached)
      {
         displayPointLabels_clean();
      }
   }
}

void MainWindow::setDisplayRightClickIcons()
{
    switch(m_displayType)
    {
    case E_DISPLAY_POINT_FIXED:
        m_displayPointsAutoAction.m_action.setIcon(QIcon());
        m_displayPointsFixedAction.m_action.setIcon(m_checkedIcon);
        m_displayPointsScientificAction.m_action.setIcon(QIcon());
    break;
    case E_DISPLAY_POINT_SCIENTIFIC:
        m_displayPointsAutoAction.m_action.setIcon(QIcon());
        m_displayPointsFixedAction.m_action.setIcon(QIcon());
        m_displayPointsScientificAction.m_action.setIcon(m_checkedIcon);
    break;
    case E_DISPLAY_POINT_AUTO:
    default:
        m_displayPointsAutoAction.m_action.setIcon(m_checkedIcon);
        m_displayPointsFixedAction.m_action.setIcon(QIcon());
        m_displayPointsScientificAction.m_action.setIcon(QIcon());
    break;
    }

    switch(m_displayDecHexX)
    {
    default:
    case E_DISPLAY_POINT_DEC:
       m_displayPointsHexOffX.m_action.setIcon(m_checkedIcon);
       m_displayPointsHexUnsignedX.m_action.setIcon(QIcon());
       m_displayPointsHexSignedX.m_action.setIcon(QIcon());
    break;
    case E_DISPLAY_POINT_UNSIGNED_HEX:
       m_displayPointsHexOffX.m_action.setIcon(QIcon());
       m_displayPointsHexUnsignedX.m_action.setIcon(m_checkedIcon);
       m_displayPointsHexSignedX.m_action.setIcon(QIcon());
    break;
    case E_DISPLAY_POINT_SIGNED_HEX:
       m_displayPointsHexOffX.m_action.setIcon(QIcon());
       m_displayPointsHexUnsignedX.m_action.setIcon(QIcon());
       m_displayPointsHexSignedX.m_action.setIcon(m_checkedIcon);
    break;
    }

    switch(m_displayDecHexY)
    {
    default:
    case E_DISPLAY_POINT_DEC:
       m_displayPointsHexOffY.m_action.setIcon(m_checkedIcon);
       m_displayPointsHexUnsignedY.m_action.setIcon(QIcon());
       m_displayPointsHexSignedY.m_action.setIcon(QIcon());
    break;
    case E_DISPLAY_POINT_UNSIGNED_HEX:
       m_displayPointsHexOffY.m_action.setIcon(QIcon());
       m_displayPointsHexUnsignedY.m_action.setIcon(m_checkedIcon);
       m_displayPointsHexSignedY.m_action.setIcon(QIcon());
    break;
    case E_DISPLAY_POINT_SIGNED_HEX:
       m_displayPointsHexOffY.m_action.setIcon(QIcon());
       m_displayPointsHexUnsignedY.m_action.setIcon(QIcon());
       m_displayPointsHexSignedY.m_action.setIcon(m_checkedIcon);
    break;
    }
}

void MainWindow::displayPointsChangeType(int type) // This is a SLOT
{
    m_displayType = (eDisplayPointType)type;
    setDisplayRightClickIcons();
    updatePointDisplay();

    // If the plot window is closed, the next time a plot with the same name is created, it will initialize to use this value.
    persistentPlotParam_get(g_persistentPlotParams, m_plotName)->m_displayType.set(m_displayType);
}

void MainWindow::displayPointsChangePrecision(int precision) // This is a SLOT
{
    if(precision == 0)
    {
        // Input of zero means let the num to string conversion run as auto.
        m_displayPrecision = STRING_STREAM_CLEAR_PRECISION_VAL;
    }
    else
    {
        m_displayPrecision += precision;
        if(m_displayPrecision < MIN_DISPLAY_PRECISION)
        {
            // Limit to MIN_DISPLAY_PRECISION
            m_displayPrecision = MIN_DISPLAY_PRECISION;
        }
    }

    setDisplayRightClickIcons();
    updatePointDisplay();

    // If the plot window is closed, the next time a plot with the same name is created, it will initialize to use this value.
    persistentPlotParam_get(g_persistentPlotParams, m_plotName)->m_displayPrecision.set(m_displayPrecision);
}

void MainWindow::displayPointsChangeDecHexX(int type)
{
   m_displayDecHexX = (eDisplayPointHexDec)type;
   setDisplayRightClickIcons();
   updatePointDisplay();
}

void MainWindow::displayPointsChangeDecHexY(int type)
{
   m_displayDecHexY = (eDisplayPointHexDec)type;
   setDisplayRightClickIcons();
   updatePointDisplay();
}

void MainWindow::displayPointsClearCurveSamples(int dummy)
{
   (void)dummy; // Tell the compiler to not warn about this dummy variable. The dummy variable is needed to use the MAPPER_ACTION_TO_SLOT macro.
   clearCurveSamples(true, false, m_displayPointsMenu_selectedCurveIndex); // Clear just the sample of the selected curve. Ask the user for confirmation before doing so.
}

void MainWindow::displayPointsCopyToClipboard(int dummy)
{
   (void)dummy; // Tell the compiler to not warn about this dummy variable. The dummy variable is needed to use the MAPPER_ACTION_TO_SLOT macro.

   std::string clipboardStr = "";
   std::string delim = "\t"; // Use tab as the delimiter to work best with Excel.

   { // Mutex lock scope.
      QMutexLocker lock(&m_qwtCurvesMutex);

      if(m_qwtSelectedSampleDelta->isAttached && m_qwtSelectedSample->isAttached)
      {
         // Delta
         QString anchored, current, delta, ttAnchored, ttCurrent, ttDelta;
         displayDeltaLabel_getLabelText(anchored, current, delta, ttAnchored, ttCurrent, ttDelta);

         std::string label = (anchored + current + delta).toStdString(); // Combine all 3 strings into 1 so the following while loop can get all the individal values.
         while(dString::InStr(label, DISPLAY_POINT_START) >= 0)
         {
            // Get text between "(" and ")"
            std::string clipboardTemp = dString::GetMiddle(&label, DISPLAY_POINT_START, DISPLAY_POINT_STOP);
            clipboardTemp = dString::Replace(clipboardTemp, DISPLAY_POINT_MID, delim);
            clipboardStr.append(clipboardTemp + delim);
         }

      }
      else if(!m_qwtSelectedSampleDelta->isAttached && m_qwtSelectedSample->isAttached)
      {
         // Non-Delta
         for(int i = 0; i < m_qwtCurves.size(); ++i)
         {
            if(m_qwtCurves[i]->pointLabel != NULL)
            {
               std::string label(m_qwtCurves[i]->pointLabel->text().toStdString());

               // Get text between "(" and ")"
               std::string clipboardTemp = dString::GetMiddle(&label, DISPLAY_POINT_START, DISPLAY_POINT_STOP);
               clipboardTemp = dString::Replace(clipboardTemp, DISPLAY_POINT_MID, delim);
               clipboardStr.append(clipboardTemp + delim);
            }
         }
      }
   } // End Mutex lock scope.

   clipboardStr = dString::DontEndWithThis(clipboardStr, delim);
   QClipboard* pClipboard = QApplication::clipboard();
   pClipboard->setText(clipboardStr.c_str());

}

void MainWindow::twoDPointsCopyToClipboard(int dummy)
{
   (void)dummy; // Tell the compiler to not warn about this dummy variable. The dummy variable is needed to use the MAPPER_ACTION_TO_SLOT macro.

   QString clipboardStr = "";
   QString delim = "\t"; // Use tab as the delimiter to work best with Excel.

   { // Mutex lock scope.
      QMutexLocker lock(&m_qwtCurvesMutex);
      if(m_qwtSelectedSampleDelta->isAttached && m_qwtSelectedSample->isAttached)
      {
         // Delta
         clipboardStr = 
            QString::number(m_qwtSelectedSampleDelta->m_pointIndex) + delim +
            QString::number(m_qwtSelectedSample->m_pointIndex) + delim +
            QString::number((int64_t)m_qwtSelectedSample->m_pointIndex - (int64_t)m_qwtSelectedSampleDelta->m_pointIndex);

      }
      else if(!m_qwtSelectedSampleDelta->isAttached && m_qwtSelectedSample->isAttached)
      {
         // Non-Delta
         clipboardStr = QString::number(m_qwtSelectedSample->m_pointIndex);
      }
   } // End Mutex lock scope.

   if(clipboardStr != "")
   {
      QClipboard* pClipboard = QApplication::clipboard();
      pClipboard->setText(clipboardStr);
   }
}

void MainWindow::set2dPointIndex(int index)
{
   int curCursorPos = (int)m_qwtSelectedSample->m_pointIndex;
   modifyCursorPos(index - curCursorPos);
}

void MainWindow::setVisibleCurveActionAll()
{
   setVisibleCurveAction(E_VIS_MENU_ALL);
}
void MainWindow::setVisibleCurveActionNone()
{
   setVisibleCurveAction(E_VIS_MENU_NONE);
}
void MainWindow::setVisibleCurveActionInvert()
{
   setVisibleCurveAction(E_VIS_MENU_INVERT);
}
void MainWindow::setVisibleCurveActionSelected()
{
   setVisibleCurveAction(E_VIS_MENU_SELECTED);
}

void MainWindow::setVisibleCurveAction(int actionType)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   switch((eVisMenuActions)actionType)
   {
      // Fall through is intended.
      case E_VIS_MENU_ALL:
      case E_VIS_MENU_NONE:
      {
         bool isVis = ((eVisMenuActions)actionType == E_VIS_MENU_ALL);
         for(int i = 0; i < m_qwtCurves.size(); ++i)
         {
            m_qwtCurves[i]->setVisible(isVis);
         }
      }
      break;
      case E_VIS_MENU_INVERT:
         for(int i = 0; i < m_qwtCurves.size(); ++i)
         {
            m_qwtCurves[i]->setVisible(!m_qwtCurves[i]->isDisplayed());
         }
      break;
      case E_VIS_MENU_SELECTED:
      {
         bool useDelta = m_qwtSelectedSampleDelta->isAttached;
         QString deltaCurveTitle = (useDelta ? m_qwtSelectedSampleDelta->getCurve()->getCurveTitle() : "");
         for(int i = 0; i < m_qwtCurves.size(); ++i)
         {
            bool isVis = ( (i == m_selectedCurveIndex) || (useDelta && (deltaCurveTitle == m_qwtCurves[i]->getCurveTitle())) );
            m_qwtCurves[i]->setVisible(isVis);
         }
      }
      break;
      default:
         // Nothing to do.
      break;
   }

   updateCurveOrder();
   m_curveCommander->curvePropertyChanged();
}

void MainWindow::updateCursors()
{
    if(m_qwtSelectedSample->isAttached)
    {
        m_qwtSelectedSample->showCursor();
    }

    if(m_qwtSelectedSampleDelta->isAttached)
    {
        m_qwtSelectedSampleDelta->showCursor();
    }

    updatePointDisplay();
    if(m_qwtSelectedSample->isAttached || m_qwtSelectedSampleDelta->isAttached)
    {
        replotMainPlot(true, true);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    bool usedEvent = false;
    if(isActiveWindow())
    {
        // Start assuming the event is going to be used
        usedEvent = true;
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *KeyEvent = (QKeyEvent*)event;

            if(KeyEvent->key() == Qt::Key_Enter || KeyEvent->key() == Qt::Key_Return)
            {
                // Pressing the Enter key can cause some weird behavoir (I think this is due to the QwtPlotPicker)
                // So, capture all Enter/Return key presses before they can be handled and drop them on the ground.
            }
            else if(KeyEvent->key() == Qt::Key_Escape)
            {
               // Escape can be used if a mouse selection was accidentally clicked.
               if(m_moveCalcSnrBarActive)
               {
                  // Move the calc bar back to where it was before the user started to move it.
                  m_snrCalcBars->selectedBarPos(m_snrBarStartPoint);
                  m_qwtPlot->replot();

                  m_showCalcSnrBarCursor = false;
               }

               // Turn off all the modifiers. This way the cursor will be set back to it's default.
               m_dragZoomModeActive = false;
               m_moveCalcSnrBarActive = false;
               setCursor();

               usedEvent = false; // Use default Escape button behavoir.
            }
            else if(KeyEvent->key() == Qt::Key_Z && KeyEvent->modifiers().testFlag(Qt::ControlModifier) && !KeyEvent->modifiers().testFlag(Qt::ShiftModifier)) // Check against Ctrl+Shift+Z to avoid hitting that case.
            {
                m_plotZoom->changeZoomFromSavedZooms(-1);
            }
            else if(KeyEvent->key() == Qt::Key_Y && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
                m_plotZoom->changeZoomFromSavedZooms(1);
            }
            else if(KeyEvent->key() == Qt::Key_S && KeyEvent->modifiers().testFlag(Qt::ControlModifier) && KeyEvent->modifiers().testFlag(Qt::ShiftModifier))
            {
               // Toggle Scroll Mode
               scrollModeToggle();
            }
            else if(KeyEvent->key() == Qt::Key_X && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Set to scroll mode and open dialog to set the scroll mode plot size.
               if(!m_scrollMode)
                  scrollModeToggle();
               scrollModeChangePlotSize();
            }
            else if(KeyEvent->key() == Qt::Key_1 && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Set the scroll mode plot size to 10x the plot size specified by the plot messages.
               QMutexLocker lock(&m_qwtCurvesMutex);
               if(!m_scrollMode) // Set to scroll mode
                  scrollModeToggle();
               scrollModeSetPlotSize(10*m_qwtCurves[m_selectedCurveIndex]->getMaxNumPointsFromPlotMsg());
            }
            else if(KeyEvent->key() == Qt::Key_2 && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Set the scroll mode plot size to half the current size.
               QMutexLocker lock(&m_qwtCurvesMutex);
               if(!m_scrollMode) // Set to scroll mode
                  scrollModeToggle();
               int newNumPoints = m_qwtCurves[m_selectedCurveIndex]->getNumPoints() >> 1; // Divide by 2.
               if(newNumPoints < 1){newNumPoints = 1;} // Bound to 1
               scrollModeSetPlotSize(newNumPoints);
            }
            else if(KeyEvent->key() == Qt::Key_U && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Set to Auto Zoom
               autoZoom();
            }
            else if(KeyEvent->key() == Qt::Key_F && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Set to Freeze Zoom
               holdZoom();
            }
            else if(KeyEvent->key() == Qt::Key_M && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Set to Max Hold Zoom
               maxHoldZoom();
            }
            else if(KeyEvent->key() == Qt::Key_R && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               resetZoom();
            }
            else if(KeyEvent->key() == Qt::Key_E && KeyEvent->modifiers().testFlag(Qt::AltModifier) && !m_allowNewCurves)
            {
               // Enable new plot messages.
               togglePlotUpdateAbility();
            }
            else if(KeyEvent->key() == Qt::Key_D && KeyEvent->modifiers().testFlag(Qt::AltModifier) && m_allowNewCurves)
            {
               // Disable new plot messages.
               togglePlotUpdateAbility();
            }
            else if(KeyEvent->key() == Qt::Key_T && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Toggle new plot messages.
               togglePlotUpdateAbility();
            }
            else if(KeyEvent->key() == Qt::Key_D && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               deltaCursorMode();
            }
            else if(KeyEvent->key() == Qt::Key_L && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Toggle Legend Visablity.
               toggleLegend();
            }
            else if(KeyEvent->key() == Qt::Key_A && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Toggle
               toggleCursorCanSelectAnyCurveAction();
            }
            else if(KeyEvent->key() == Qt::Key_P && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               silentSavePlotToFile();
            }
            else if(KeyEvent->key() == Qt::Key_I && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               setZoomLimits_guiSlot();
            }
            else if(KeyEvent->key() == Qt::Key_C && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               QMutexLocker lock(&m_qwtCurvesMutex);
               QVector<CurveData*> curveVect;
               for(int i = 0; i < m_qwtCurves.size(); ++i)
               {
                  if(m_qwtCurves[i]->isDisplayed())
                  {
                     curveVect.push_back(m_qwtCurves[i]);
                  }
               }
               SavePlot savePlot(this, getPlotName(), curveVect, E_SAVE_RESTORE_CLIPBOARD_EXCEL, true);
               PackedCurveData clipboardDataString;
               savePlot.getPackedData(clipboardDataString);

               // Null Terminate.
               size_t origSize = clipboardDataString.size();
               clipboardDataString.resize(origSize + 1);
               clipboardDataString[origSize] = '\0';

               QClipboard* pClipboard = QApplication::clipboard();
               pClipboard->setText(&clipboardDataString[0]);

            }
            else if(KeyEvent->key() == Qt::Key_V && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               QClipboard* pClipboard = QApplication::clipboard();
               QString clipText = pClipboard->text();
               if(clipText != "")
               {
                  m_curveCommander->showCreatePlotFromDataGui(getPlotName(), clipText.toStdString().c_str());
               }
            }
            else if(KeyEvent->key() == Qt::Key_H && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Hide an individual sample point (i.e. replace it with Not A Number(nan))
               QMutexLocker lock(&m_qwtCurvesMutex);
               if(m_qwtSelectedSample != NULL && m_qwtSelectedSample->isAttached) // Only continue if the selected sample is being displayed
               {
                  QMessageBox::StandardButton reply;
                  reply = QMessageBox::question(this, "Hide Selected Sample?", "Hiding the selected sample will set it to Not A Number (nan).",
                                                QMessageBox::Yes | QMessageBox::No);
                  if(reply == QMessageBox::Yes)
                  {
                     m_qwtCurves[m_selectedCurveIndex]->setPointValue(m_qwtSelectedSample->m_pointIndex, NAN);
                     updatePlotWithNewCurveData(true);
                  }
               }
            }
            else if(KeyEvent->key() == Qt::Key_0 && KeyEvent->modifiers().testFlag(Qt::ControlModifier) && !KeyEvent->modifiers().testFlag(Qt::AltModifier))
            {
               // Clear all samples.
               clearCurveSamples(false, true);
            }
            else if(KeyEvent->key() == Qt::Key_0 && KeyEvent->modifiers().testFlag(Qt::AltModifier))
            {
               setDisplayedSamples(true, 0);
            }
            else if(KeyEvent->key() == Qt::Key_S && KeyEvent->modifiers().testFlag(Qt::ControlModifier) && !KeyEvent->modifiers().testFlag(Qt::ShiftModifier))
            {
               // Save Dialog - Save entire plot to file
               openSavePlotDialog(false);
            }
            else if(KeyEvent->key() == Qt::Key_Z && KeyEvent->modifiers().testFlag(Qt::ControlModifier) && KeyEvent->modifiers().testFlag(Qt::ShiftModifier))
            {
               // Save Dialog - Limit save points to current zoom
               openSavePlotDialog(true);
            }
            else
            {
                // Key stroke wasn't anything more specific. Pass to some functions to see if
                // the key stroke was valid.
                switch(m_selectMode)
                {
                case E_ZOOM:
                    usedEvent = keyPressModifyZoom(KeyEvent->key());
                    break;
                case E_CURSOR:
                case E_DELTA_CURSOR:
                    usedEvent = keyPressModifyCursor(KeyEvent->key());
                    break;
                default:
                    usedEvent = false;
                    break;
                }
            }
        }
        else if(event->type() == QEvent::Wheel)
        {
            QKeyEvent *KeyEvent = (QKeyEvent*)event;
            if(KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
                QWheelEvent *wheelEvent = static_cast<QWheelEvent *> (event);

                QSize cavasSize = m_qwtPlot->canvas()->frameSize();
                QPoint mousePos = m_qwtPlot->canvas()->mapFromGlobal(m_qwtPlot->cursor().pos());

                // Map position relative to the curve's canvas.
                // Y axis needs to be inverted so the 0 point is at the bottom.
                QPointF relMousePos(
                    (double)(mousePos.x() - PLOT_CANVAS_OFFSET) / (double)(cavasSize.width() - 2*PLOT_CANVAS_OFFSET),
                    1.0 - (double)(mousePos.y() - PLOT_CANVAS_OFFSET) / (double)(cavasSize.height() - 2*PLOT_CANVAS_OFFSET));

                bool holdYAxis = KeyEvent->modifiers().testFlag(Qt::ShiftModifier);

                auto stdMouseUpDownDelta = wheelEvent->angleDelta().y(); // X axis is for fancy mouses that can scroll left/right.
                if(stdMouseUpDownDelta > 0)
                {
                    m_plotZoom->Zoom(ZOOM_IN_PERCENT, relMousePos, holdYAxis);
                }
                else if(stdMouseUpDownDelta < 0)
                {
                    m_plotZoom->Zoom(ZOOM_OUT_PERCENT, relMousePos, holdYAxis);
                }
            }
            else
            {
                usedEvent = false;
            }

        }
        else if(event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if(mouseEvent->button() == Qt::LeftButton && keyEvent->modifiers().testFlag(Qt::ShiftModifier))
            {
                m_plotZoom->Zoom(ZOOM_OUT_PERCENT);
            }
            else if(mouseEvent->button() == Qt::XButton1)
            {
                m_plotZoom->changeZoomFromSavedZooms(-1);
            }
            else if(mouseEvent->button() == Qt::XButton2)
            {
                m_plotZoom->changeZoomFromSavedZooms(1);
            }
            else
            {
                usedEvent = false;
            }

        }
        else if(event->type() == QEvent::MouseMove)
        {
           // For now this is only keeping track of the Calc SNR Bars. No need to do anything if those are not visible.
           if(m_snrCalcBars != NULL && m_snrCalcBars->isVisable())
           {
              QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

              // Convert mouse point on the canvis to the point on the plot GUI element.
              QPointF plotPoint;
              plotPoint.setX(m_qwtPlot->canvasMap(QwtPlot::xBottom).invTransform(mouseEvent->pos().x()));
              plotPoint.setY(m_qwtPlot->canvasMap(QwtPlot::yLeft).invTransform(mouseEvent->pos().y()));

              if(!m_moveCalcSnrBarActive) // Calling isSelectionCloseToBar when m_moveCalcSnrBarActive is true will screw up the bar that is being moved.
              {
                 m_showCalcSnrBarCursor = isSelectionCloseToBar(plotPoint);
                 setCursor(); // Update Cursor in case Calc SNR Bar cursor changed.
              }
           }
           usedEvent = false; // Never block default functionality for mouse movement.
        }
        else
        {
            usedEvent = false;
        }
    }

    if(usedEvent)
    {
        return true;
    }
    else
    {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }

}

bool MainWindow::keyPressModifyZoom(int key)
{
    bool valid = true;

    switch(key)
    {
        case Qt::Key_Down:
            m_plotZoom->ModSliderPos(E_Y_AXIS, ZOOM_SCROLL_CHANGE_TINY);
        break;
        case Qt::Key_Up:
            m_plotZoom->ModSliderPos(E_Y_AXIS, -ZOOM_SCROLL_CHANGE_TINY);
        break;
        case Qt::Key_Left:
            m_plotZoom->ModSliderPos(E_X_AXIS, -ZOOM_SCROLL_CHANGE_TINY);
        break;
        case Qt::Key_Right:
            m_plotZoom->ModSliderPos(E_X_AXIS, ZOOM_SCROLL_CHANGE_TINY);
        break;
        default:
            valid = false;
        break;
    }

    return valid;
}

bool MainWindow::keyPressModifyCursor(int key)
{
    bool valid = true;

    switch(key)
    {
        case Qt::Key_Down:
            modifySelectedCursor(-1);
        break;
        case Qt::Key_Up:
            modifySelectedCursor(1);
        break;
        case Qt::Key_Left:
            modifyCursorPos(-1);
        break;
        case Qt::Key_Right:
            modifyCursorPos(1);
        break;
        default:
            valid = false;
        break;
    }

    return valid;
}

void MainWindow::modifySelectedCursor(int modDelta)
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    if(modDelta != 0)
    {
        std::vector<int> displayedCurves;
        int indexOfSelectedCursor = -1;

        unsigned int modDeltaAbs = (unsigned int)abs(modDelta);

        for(int i = 0; i < m_qwtCurves.size(); ++i)
        {
            if(m_qwtCurves[i]->isDisplayed())
            {
                if(i == m_selectedCurveIndex)
                {
                    indexOfSelectedCursor = displayedCurves.size();
                }
                displayedCurves.push_back(i);
            }
        }

        if(modDeltaAbs < displayedCurves.size() && indexOfSelectedCursor >= 0)
        {
            int newIndexOfSelectedCursor = indexOfSelectedCursor + modDelta;
            if(newIndexOfSelectedCursor < 0)
            {
                newIndexOfSelectedCursor += displayedCurves.size();
            }
            else if((unsigned int)newIndexOfSelectedCursor >= displayedCurves.size())
            {
                newIndexOfSelectedCursor -= displayedCurves.size();
            }

            setSelectedCurveIndex(displayedCurves[newIndexOfSelectedCursor]);
            updateCursors();
        }
        emit updateCursorMenusSignal();
    }

}

void MainWindow::modifyCursorPos(int modDelta)
{
    if(modDelta != 0)
    {
        if(m_qwtSelectedSample->isAttached)
        {
            int newXPos = (int)m_qwtSelectedSample->m_pointIndex + modDelta;

            while(newXPos < 0)
            {
                newXPos += m_qwtSelectedSample->getCurve()->getNumPoints();
            }
            while((unsigned int)newXPos >= m_qwtSelectedSample->getCurve()->getNumPoints())
            {
                newXPos -= m_qwtSelectedSample->getCurve()->getNumPoints();
            }

            m_qwtSelectedSample->m_pointIndex = newXPos;
            updateCursors();

        }
    }
}


void MainWindow::on_verticalScrollBar_actionTriggered(int action)
{
    switch(action)
    {
    case QAbstractSlider::SliderSingleStepAdd:
        m_plotZoom->ModSliderPos(E_Y_AXIS, -ZOOM_SCROLL_CHANGE_SMALL);
        break;
    case QAbstractSlider::SliderSingleStepSub:
        m_plotZoom->ModSliderPos(E_Y_AXIS, ZOOM_SCROLL_CHANGE_SMALL);
        break;
    case QAbstractSlider::SliderPageStepAdd:
        m_plotZoom->ModSliderPos(E_Y_AXIS, -ZOOM_SCROLL_CHANGE_BIG);
        break;
    case QAbstractSlider::SliderPageStepSub:
        m_plotZoom->ModSliderPos(E_Y_AXIS, ZOOM_SCROLL_CHANGE_BIG);
        break;
    }
}

void MainWindow::on_horizontalScrollBar_actionTriggered(int action)
{
    switch(action)
    {
    case QAbstractSlider::SliderSingleStepAdd:
        m_plotZoom->ModSliderPos(E_X_AXIS, ZOOM_SCROLL_CHANGE_SMALL);
        break;
    case QAbstractSlider::SliderSingleStepSub:
        m_plotZoom->ModSliderPos(E_X_AXIS, -ZOOM_SCROLL_CHANGE_SMALL);
        break;
    case QAbstractSlider::SliderPageStepAdd:
        m_plotZoom->ModSliderPos(E_X_AXIS, ZOOM_SCROLL_CHANGE_BIG);
        break;
    case QAbstractSlider::SliderPageStepSub:
        m_plotZoom->ModSliderPos(E_X_AXIS, -ZOOM_SCROLL_CHANGE_BIG);
        break;
    case QAbstractSlider::SliderMove:
       updateAllCurveGuiPointsReplot();
    break;

    }
}

maxMinXY MainWindow::getPlotDimWithNormalization()
{
   maxMinXY newPlotDim = m_maxMin;

   if(m_selectedCurveIndex >= 0 && m_selectedCurveIndex < m_qwtCurves.size())
   {
      maxMinXY selectedCurveMaxMin = m_qwtCurves[m_selectedCurveIndex]->getMaxMinXYOfData();
      if(m_normalizeCurves_xAxis)
      {
         newPlotDim.maxX  = selectedCurveMaxMin.maxX;
         newPlotDim.minX  = selectedCurveMaxMin.minX;
         newPlotDim.realX = selectedCurveMaxMin.realX;
      }
      if(m_normalizeCurves_yAxis)
      {
         newPlotDim.maxY  = selectedCurveMaxMin.maxY;
         newPlotDim.minY  = selectedCurveMaxMin.minY;
         newPlotDim.realY = selectedCurveMaxMin.realY;
      }
   }

   return newPlotDim;
}

void MainWindow::setSelectedCurveIndex(int index)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   if(index >= 0 && index < m_qwtCurves.size())
   {
      int oldSelectCursor = m_selectedCurveIndex;

      m_qwtSelectedSample->setCurve(m_qwtCurves[index]);

      // If any curve selection is in use, the delta sample curve might not match the selected curve. In that case do not change the delta sample curve.
      bool anyCurve = m_cursorCanSelectAnyCurve && m_toggleCursorCanSelectAnyCurveAction.isVisible(); // If action is invisible, there is only 1 curve. So no need for Any Curve logic.
      bool deltaCouldBeDifferentCurve = anyCurve && m_qwtSelectedSampleDelta->isAttached;
      if(!deltaCouldBeDifferentCurve || getCurveIndex(m_qwtSelectedSampleDelta->getCurve()) < 0) // Also change it if the curve is no longer valid.
      {
         m_qwtSelectedSampleDelta->setCurve(m_qwtCurves[index]);
      }

      m_snrCalcBars->setCurve(m_qwtCurves[index], m_qwtCurves);
      m_selectedCurveIndex = index;
      if(m_normalizeCurves_xAxis || m_normalizeCurves_yAxis)
      {
         // Update zoom so that it will show the same portion of the plot with the new normalization factor.
         if(oldSelectCursor >= 0 && oldSelectCursor < m_qwtCurves.size())
         {
            maxMinXY newZoom;
            maxMinXY oldMaxMinXY = m_plotZoom->getCurZoom();
            tLinearXYAxis newCursorScale = m_qwtCurves[m_selectedCurveIndex]->getNormFactor();
            newZoom.maxX = (oldMaxMinXY.maxX - newCursorScale.xAxis.b) / newCursorScale.xAxis.m;
            newZoom.minX = (oldMaxMinXY.minX - newCursorScale.xAxis.b) / newCursorScale.xAxis.m;
            newZoom.maxY = (oldMaxMinXY.maxY - newCursorScale.yAxis.b) / newCursorScale.yAxis.m;
            newZoom.minY = (oldMaxMinXY.minY - newCursorScale.yAxis.b) / newCursorScale.yAxis.m;

            SetZoomPlotDimensions(getPlotDimWithNormalization(), true);
            m_plotZoom->SetZoom(newZoom);
         }
         else
         {
            m_plotZoom->ResetZoom();
         }
         replotMainPlot();

      }
   }
}

void MainWindow::replotMainPlot(bool changeCausedByUserGuiInput, bool cursorChanged)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   maxMinXY selectedCurveMaxMin = m_maxMin;

   if(m_selectedCurveIndex >= 0 && m_selectedCurveIndex < m_qwtCurves.size())
   {
      selectedCurveMaxMin = m_qwtCurves[m_selectedCurveIndex]->getMaxMinXYOfData();
   }

   int numDisplayedCurves = 0;
   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      m_qwtCurves[i]->setNormalizeFactor(selectedCurveMaxMin, m_normalizeCurves_xAxis, m_normalizeCurves_yAxis);
      m_qwtCurves[i]->setCurveSamples();
      if(m_qwtCurves[i]->isDisplayed())
      {
         ++numDisplayedCurves;
      }
   }

   // If just the cursor point changed, there is no need to update plot dimesions.
   if(cursorChanged == false)
   {
      SetZoomPlotDimensions(getPlotDimWithNormalization(), changeCausedByUserGuiInput);
   }

   if(m_snrCalcBars != NULL)
   {
      m_snrCalcBars->updateZoomDim(m_plotZoom->getCurZoom());
      m_snrCalcBars->updatePlotDim(m_plotZoom->getCurPlotDim());
      m_snrCalcBars->updateSampleRate();
   }

   if(m_qwtSelectedSampleDelta->isAttached && m_cursorCanSelectAnyCurve)
   {
      // Need to adjust the Delta Sample Selected Point position to account for normalization.
      m_qwtSelectedSampleDelta->showCursor();
   }

   // The "Cursor Can Select Any Curve" action only needs to be selectable if there are more than 1 displayed curves.
   bool cursorCanSelectAnyCurveVisible = numDisplayedCurves > 1;
   m_toggleCursorCanSelectAnyCurveAction.setVisible(cursorCanSelectAnyCurveVisible);

   m_qwtPlot->replot();
}

void MainWindow::ShowRightClickForPlot(const QPoint& pos) // this is a slot
{
    m_rightClickMenu.exec(m_qwtPlot->mapToGlobal(pos));
}

void MainWindow::ShowRightClickForDisplayPoints(const QPoint& pos)
{
   // Grab off the current postion of the cursor in the entire window. This will be used to determine
   // which label was right clicked on.
   int cursorPosX = this->mapFromGlobal(QCursor::pos()).x();

   QMutexLocker lock(&m_qwtCurvesMutex);

   QList<QLabel*> labelsToTry; // List of the currently displayed labels.
   QMap<QLabel*, int> labelToCurveIndex; // Add ability to determine the curve index from the label pointer.

   // Fill in the list with valid Normal Display Point Labels
   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      if(m_qwtCurves[i]->pointLabel != NULL)
      {
         labelsToTry.push_back(m_qwtCurves[i]->pointLabel);
         labelToCurveIndex[m_qwtCurves[i]->pointLabel] = i;
      }
   }

   // If no Normal Display Point Labels are being displayed, we must be in Delta Cursor Mode.
   // Fill in the list with valid Delta Display Point Labels
   if(labelsToTry.size() == 0)
   {
      for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
      {
         if( m_deltaLabels[i] != NULL)
         {
            labelsToTry.push_back(m_deltaLabels[i]);
         }
      }
   }

   if(labelsToTry.size() > 0)
   {
      QLabel* matchingLabel = labelsToTry[0]; // Set to a known, valid label.

      // Determine the best label to generate the right click menu from.
      for(int i = 1; i < labelsToTry.size(); ++i)
      {
         // Check if this is a better match.
         int thisLabelStartPosX = labelsToTry[i]->pos().x();
         int thisLabelStopPosX = thisLabelStartPosX + labelsToTry[i]->width();
         if(cursorPosX >= thisLabelStartPosX && cursorPosX <= thisLabelStopPosX)
         {
            matchingLabel = labelsToTry[i];
         }
      }

      // Display the right click menu.
      m_displayPointsMenu_selectedCurveIndex = labelToCurveIndex[matchingLabel]; // Store off the curve index the corresponds to the curve label that was right-clicked on.
      m_displayPointsMenu.exec(matchingLabel->mapToGlobal(pos));
   }

}

void MainWindow::ShowRightClickFor2dPointLabel(const QPoint& pos)
{
   m_2dPointMenu.exec(ui->lbl2dPoint->mapToGlobal(pos));
}

void MainWindow::onApplicationFocusChanged(QWidget* /*old*/, QWidget* /*now*/)
{
  if(isActiveWindow())
  {
      QCoreApplication::instance()->installEventFilter(this);
  }
  else
  {
      QCoreApplication::instance()->removeEventFilter(this);

      // Make sure the drag Calc SNR Bar cursor isn't shown when this plot window isn't in focus any more.
      if(m_showCalcSnrBarCursor)
      {
         m_showCalcSnrBarCursor = false;
         setCursor(); // Update Cursor in case Calc SNR Bar cursor changed.
      }
  }
}

int MainWindow::getNumCurves()
{
   QMutexLocker lock(&m_qwtCurvesMutex);
   return m_qwtCurves.size();
}

int MainWindow::getCurveIndex(const QString& curveTitle)
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i]->getCurveTitle() == curveTitle)
        {
            return i;
        }
    }
    return -1;
}

int MainWindow::getCurveIndex(CurveData* ptr)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
       if(m_qwtCurves[i] == ptr)
       {
           return i;
       }
   }
   return -1;
}

void MainWindow::setCurveIndex(const QString& curveTitle, int newIndex)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   int curIndex = getCurveIndex(curveTitle);
   if(curIndex >= 0)
   {
      int maxCurveIndex = m_qwtCurves.size() - 1;
      if(newIndex > maxCurveIndex)
         newIndex = maxCurveIndex;
      else if(newIndex < 0)
         newIndex = 0;

      if(curIndex != newIndex)
      {
         // Get title of the selected curve.
         QString selectedCurveTitle = (m_selectedCurveIndex >= 0 && m_selectedCurveIndex < m_qwtCurves.size()) ?
            m_qwtCurves[m_selectedCurveIndex]->getCurveTitle() : "";

         // Move the curve its new position.
         m_qwtCurves.move(curIndex, newIndex);

         // Update the selected curve index for its new position.
         if(selectedCurveTitle != "")
            m_selectedCurveIndex = getCurveIndex(selectedCurveTitle);

         // Update the GUI.
         updateCurveOrder();
      }
      // Else new and cur index are the same, nothing to do.
   }
}

void MainWindow::closeEvent(QCloseEvent* /*event*/)
{
    m_curveCommander->plotWindowClose(this->getPlotName());
}

void MainWindow::resizeEvent(QResizeEvent* /*event*/)
{
    QSize canvasSize = m_qwtPlot->canvas()->frameSize();

    m_canvasWidth_pixels  = canvasSize.width()  - (2*PLOT_CANVAS_OFFSET);
    m_canvasHeight_pixels = canvasSize.height() - (2*PLOT_CANVAS_OFFSET);

    m_canvasXOverYRatio = (double)m_canvasWidth_pixels / (double)m_canvasHeight_pixels;

    updateAllCurveGuiPointsReplot();
}


void MainWindow::setCurveStyleMenu()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   m_stylesCurvesMenu.clear();
   m_curveLineMenu.clear();

   int callbackVal = 0;

   // curveIndex == -1 indicates All Curves
   for(int curveIndex = -1; curveIndex < m_qwtCurves.size(); ++curveIndex)
   {
      if( (curveIndex >= 0 && m_qwtCurves[curveIndex]->isDisplayed()) || curveIndex < 0)
      {
         if(curveIndex >= 0)
            m_curveLineMenu.push_back( QSharedPointer<curveStyleMenu>(new curveStyleMenu(m_qwtCurves[curveIndex]->getCurveTitle(), this)) );
         else
            // Set all curves to this Curve Style
            m_curveLineMenu.push_back( QSharedPointer<curveStyleMenu>(new curveStyleMenu("All Curves", this)) );


         // TODO: must be a better way to get the last element of a list.
         QList<QSharedPointer<curveStyleMenu> >::iterator newMenuItem = m_curveLineMenu.end();
         --newMenuItem;

         m_stylesCurvesMenu.addMenu(&(*newMenuItem)->m_menu);


         for( QVector<curveStyleMenuActionMapper*>::iterator mapIter = (*newMenuItem)->m_actionMapper.begin();
              mapIter != (*newMenuItem)->m_actionMapper.end();
              ++mapIter )
         {

            callbackVal = ((curveIndex << 16) & 0xffff0000) | ((*mapIter)->m_style & 0xffff);

            (*mapIter)->m_qmam.m_mapper.setMapping(&(*mapIter)->m_qmam.m_action, callbackVal);
            connect(&(*mapIter)->m_qmam.m_action,SIGNAL(triggered()),
                          &(*mapIter)->m_qmam.m_mapper,SLOT(map()));

            (*newMenuItem)->m_menu.addAction(&(*mapIter)->m_qmam.m_action);
            connect( &(*mapIter)->m_qmam.m_mapper, SIGNAL(mapped(int)), SLOT(changeCurveStyle(int)) );

            // When curveIndex equals -1, we are dealing with All Curves.
            setCurveStyleMenuIcons();

         }

      }
   }
}
void MainWindow::setCurveStyleMenuIcons()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   // curveIndex == -1 indicates All Curves
   int curveIndex = -1;
   for( QList<QSharedPointer<curveStyleMenu> >::iterator menuItem = m_curveLineMenu.begin();
        menuItem != m_curveLineMenu.end();
        ++menuItem )
   {
      QwtPlotCurve::CurveStyle selectedCurveStyle = QwtPlotCurve::NoCurve;
      if(curveIndex == -1)
      {
         selectedCurveStyle = m_defaultCurveStyle;
      }
      else if(curveIndex >= 0 && curveIndex < m_qwtCurves.size())
      {
         selectedCurveStyle = m_qwtCurves[curveIndex]->getStyle();
      }

      if(selectedCurveStyle != QwtPlotCurve::NoCurve)
      {
         for(int i = 0; i < (*menuItem)->m_actionMapper.size(); ++i)
         {
            if((*menuItem)->m_actionMapper[i]->m_style == selectedCurveStyle)
            {
               switch((*menuItem)->m_actionMapper[i]->m_style)
               {
                  case QwtPlotCurve::Lines:
                     (*menuItem)->m_actionMapper[i]->m_qmam.m_action.setIcon(QIcon(":/CurveStyleIcons/lines_selected.png"));
                  break;
                  case QwtPlotCurve::Dots:
                     (*menuItem)->m_actionMapper[i]->m_qmam.m_action.setIcon(QIcon(":/CurveStyleIcons/dots_selected.png"));
                  break;
                  case QwtPlotCurve::Sticks:
                     (*menuItem)->m_actionMapper[i]->m_qmam.m_action.setIcon(QIcon(":/CurveStyleIcons/sticks_selected.png"));
                  break;
                  case QwtPlotCurve::Steps:
                     (*menuItem)->m_actionMapper[i]->m_qmam.m_action.setIcon(QIcon(":/CurveStyleIcons/steps_selected.png"));
                  break;
                  case QwtPlotCurve::NoCurve:
                  case QwtPlotCurve::UserCurve:
                  default:
                  break;
               }
            }
            else
            {
               switch((*menuItem)->m_actionMapper[i]->m_style)
               {
                  case QwtPlotCurve::Lines:
                     (*menuItem)->m_actionMapper[i]->m_qmam.m_action.setIcon(QIcon(":/CurveStyleIcons/lines.png"));
                  break;
                  case QwtPlotCurve::Dots:
                     (*menuItem)->m_actionMapper[i]->m_qmam.m_action.setIcon(QIcon(":/CurveStyleIcons/dots.png"));
                  break;
                  case QwtPlotCurve::Sticks:
                     (*menuItem)->m_actionMapper[i]->m_qmam.m_action.setIcon(QIcon(":/CurveStyleIcons/sticks.png"));
                  break;
                  case QwtPlotCurve::Steps:
                     (*menuItem)->m_actionMapper[i]->m_qmam.m_action.setIcon(QIcon(":/CurveStyleIcons/steps.png"));
                  break;
                  case QwtPlotCurve::NoCurve:
                  case QwtPlotCurve::UserCurve:
                  default:
                  break;
               }
            }
         }
      }
      else
      {
         // Should probably be an error here.
      }
      ++curveIndex;
   }
}

void MainWindow::changeCurveStyle(int inVal)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   short curveIndex = (inVal >> 16) & 0xffff;
   QwtPlotCurve::CurveStyle curveStyle = (QwtPlotCurve::CurveStyle)((inVal) & 0xffff);

   if(curveIndex >= 0 && curveIndex < m_qwtCurves.size())
   {
      m_qwtCurves[curveIndex]->setCurveAppearance(CurveAppearance(m_qwtCurves[curveIndex]->getColor(), curveStyle));
   }
   else if(curveIndex == -1)
   {
      // Apply to all curves.
      m_defaultCurveStyle = curveStyle;
      for(curveIndex = 0; curveIndex < m_qwtCurves.size(); ++curveIndex)
      {
         m_qwtCurves[curveIndex]->setCurveAppearance(CurveAppearance(m_qwtCurves[curveIndex]->getColor(), m_defaultCurveStyle));
      }

      // If the plot window is closed, the next time a plot with the same name is created, it will initialize to use this value.
      persistentPlotParam_get(g_persistentPlotParams, m_plotName)->m_curveStyle.set(m_defaultCurveStyle);
   }
   setCurveStyleMenuIcons();
}

void MainWindow::updateCurveOrder()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   // Detach all and re-attach visable curves, so as to retain curve order on gui
   QList<CurveData*> visableCurves;
   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      if(m_qwtCurves[i]->isDisplayed() == true)
      {
         visableCurves.push_back(m_qwtCurves[i]);
         m_qwtCurves[i]->setVisible(false); // Detach curve from this plot window.
      }
   }
   for(int i = 0; i < visableCurves.size(); ++i)
   {
      visableCurves[i]->setVisible(true); // Re-attach curve to this plot window.
   }

   // Make sure the selected point index is still valid.
   int numCurves = m_qwtCurves.size();
   if(m_selectedCurveIndex >= numCurves || m_selectedCurveIndex < 0)
   {
      int boundedSelectedCurveIndex = numCurves > 0 ? numCurves-1 : 0;
      setSelectedCurveIndex(boundedSelectedCurveIndex);
   }

   // Make sure cursors are in front of curve data.
   if(m_qwtSelectedSample != NULL && m_qwtSelectedSample->isAttached == true)
   {
      m_qwtSelectedSample->showCursor();
   }
   if(m_qwtSelectedSampleDelta != NULL && m_qwtSelectedSampleDelta->isAttached == true)
   {
      m_qwtSelectedSampleDelta->showCursor();
   }

   updatePointDisplay();
   m_snrCalcBars->moveToFront();

   replotMainPlot();
   maxMinXY maxMin = calcMaxMin();
   SetZoomPlotDimensions(maxMin, true);
   emit updateCursorMenusSignal();
}

void MainWindow::removeCurve(const QString& curveName)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   int curveIndexToRemove = getCurveIndex(curveName);
   if(curveIndexToRemove >= 0)
   {
      int newSelectedCurveIndex = -1; // Initialize to invalid value.
      if(curveIndexToRemove < m_selectedCurveIndex)
      {
         // Selected curve index will need to move to handle the removed curve.
         newSelectedCurveIndex = m_selectedCurveIndex - 1;
      }

      if(curveIndexToRemove == m_selectedCurveIndex)
      {
         // Make sure the selected cursor gets reset to the cursor that will be moved
         // to where the cursor that is about to be deleted was.
         newSelectedCurveIndex = m_selectedCurveIndex;
      }

      // Remove the curve.
      delete m_qwtCurves[curveIndexToRemove];
      m_qwtCurves.removeAt(curveIndexToRemove);

      // Update Selelected Curve Index
      if(newSelectedCurveIndex >= 0 && newSelectedCurveIndex < m_qwtCurves.size())
      {
         setSelectedCurveIndex(newSelectedCurveIndex);
      }

      m_plotZoom->m_plotIs1D = areAllCurves1D(); // The Zoom class needs to know if there are non-1D plots for the Max Hold functionality.
   }
   updateCurveOrder();

   // The selected curve may have changed, make sure the displayed cursor points get updated.
   if(m_qwtCurves.size() > 0)
   {
      updateCursors();
   }
}

unsigned int MainWindow::findNextUnusedColorIndex()
{
   unsigned int nextColorIndex = 0;
   bool nextColorFound = false;
   for(unsigned int colorIndex = 0; colorIndex < ARRAY_SIZE(curveColors) && nextColorFound == false; ++colorIndex)
   {
      bool matchFound = false;
      for(unsigned int curveIndex = 0; curveIndex < (unsigned int)m_qwtCurves.size() && matchFound == false; ++curveIndex)
      {
         if(m_qwtCurves[curveIndex]->getColor() == curveColors[colorIndex])
         {
            matchFound = true;
         }
      }

      if(matchFound == false)
      {
         nextColorFound = true;
         nextColorIndex = colorIndex;
      }
   }

   return nextColorIndex;
}

void MainWindow::toggleCurveVisability(const QString& curveName)
{
   int curveIndex = getCurveIndex(curveName);
   if(curveIndex >= 0)
   {
      visibleCursorMenuSelect(curveIndex);
   }
}

void MainWindow::resetActivityIndicator()
{
   QMutexLocker lock(&m_activityIndicator_mutex); // Scoped mutex lock.

   m_activityIndicator_plotIsActive = true;
   m_activityIndicator_inactiveCount = 0;
   if(m_activityIndicator_timer.isActive() == false)
   {
      m_activityIndicator_timer.start(ACTIVITY_INDICATOR_ON_PERIOD_MS);
   }
}

void MainWindow::activityIndicatorTimerSlot()
{
   QMutexLocker lock(&m_activityIndicator_mutex); // Scoped mutex lock.

   if( m_activityIndicator_plotIsActive == false &&
       ++m_activityIndicator_inactiveCount >= ACTIVITY_INDICATOR_PERIODS_TO_TIMEOUT )
   {
      m_activityIndicator_indicatorState = false;
      m_activityIndicator_timer.stop();
   }
   else
   {
      m_activityIndicator_indicatorState = !m_activityIndicator_indicatorState;
   }

   m_activityIndicator_plotIsActive = false;

   if(m_activityIndicator_indicatorState)
   {
      ui->activityIndicator->setPalette(m_allowNewCurves ?
         m_activityIndicator_onEnabledPallet : m_activityIndicator_onDisabledPallet);
   }
   else
   {
      ui->activityIndicator->setPalette(m_activityIndicator_offPallet);
   }

}

// This function is to be called by the PlotZoom object whenever it is about to change the zoom.
// This allows us to perform some operations whenever the zoom or plot dimensions change.
void MainWindow::plotZoomDimChanged(const tMaxMinXY& plotDimensions, const tMaxMinXY& zoomDimensions, bool xAxisZoomChanged, bool changeCausedByUserGuiInput)
{
   (void)changeCausedByUserGuiInput; // Ignore unused warning.
   if(xAxisZoomChanged)
   {
      // When a plot is a 1D plot, we reduce the samples send to the GUI based on the X Axis zoom dimensions.
      // Thus, we only need to update the samples sent to the GUI when the X Axis zoom has changed.
      updateAllCurveGuiPoints();
   }

   if(m_snrCalcBars != NULL)
   {
      m_snrCalcBars->updateZoomDim(zoomDimensions);
      m_snrCalcBars->updatePlotDim(plotDimensions);
   }
}

void MainWindow::setScrollMode(bool newState, int size)
{
   if(m_scrollMode != newState)
   {
      scrollModeToggle();
   }
   if(size > 0)
   {
      scrollModeSetPlotSize(size);
   }
}


void MainWindow::externalZoomReset()
{
   emit externalResetZoomSignal();
}

void MainWindow::setSnrBarMode(bool newState)
{
   if(m_calcSnrDisplayed != newState)
   {
      calcSnrToggle();
   }
}

void MainWindow::updateAllCurveGuiPoints()
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      // Only need to do this for 1D curves, 2D curves will always send all their points to the GUI.
      m_qwtCurves[i]->setCurveDataGuiPoints(true);
   }
}

void MainWindow::updateAllCurveGuiPointsReplot()
{
   updateAllCurveGuiPoints();
   m_qwtPlot->replot();
}

// If this is not the first time a plot window has been created with this plot name, check if
// there are any persistent parameters that should be restored.
void MainWindow::restorePersistentPlotParams()
{
   if(persistentPlotParam_areThereAnyParams(g_persistentPlotParams, m_plotName))
   {
      persistentPlotParameters* ppp = persistentPlotParam_get(g_persistentPlotParams, m_plotName);

      // Attempt to restore 'm_displayType'
      if(ppp->m_displayType.isValid())
      {
         m_displayType = ppp->m_displayType.get();
         setDisplayRightClickIcons(); // Set the display type that has the check icon next to it.
      }

      // Attempt to restore 'm_displayPrecision'
      if(ppp->m_displayPrecision.isValid())
      {
         m_displayPrecision = ppp->m_displayPrecision.get();
      }

      // Attempt to restore 'm_curveStyle'
      if(ppp->m_curveStyle.isValid())
      {
         setCurveStyleForCurve(-1, ppp->m_curveStyle.get());
      }

      // Attempt to restore 'm_legend'
      if(ppp->m_legend.isValid())
      {
         setLegendState(ppp->m_legend.get());
      }

      // Attempt to restore 'm_cursorCanSelectAnyCurve'
      if(ppp->m_cursorCanSelectAnyCurve.isValid())
      {
         bool restoreValue = ppp->m_cursorCanSelectAnyCurve.get();
         if(restoreValue != m_cursorCanSelectAnyCurve)
         {
            toggleCursorCanSelectAnyCurveAction();
         }
      }

   }
}

void MainWindow::setCurveStyleForCurve(int curveIndex, QwtPlotCurve::CurveStyle curveStyle) // curveIndex of -1 will set all curves.
{
   curveIndex = curveIndex < 0 ? -1 : curveIndex; // If negative set to -1, otherwise leave the same.
   int packedIndexAndStyle = ((curveIndex << 16) & 0xffff0000) | (curveStyle & 0xffff);
   changeCurveStyle(packedIndexAndStyle);
}

void MainWindow::setCursor()
{
   QwtPicker::RubberBand pickerRubberBand = QwtPicker::CrossRubberBand; // Default picker.
   if(m_dragZoomModeActive)
   {
      m_qwtPlot->canvas()->setCursor(Qt::OpenHandCursor);
   }
   else if(m_moveCalcSnrBarActive || m_showCalcSnrBarCursor)
   {
      m_qwtPlot->canvas()->setCursor(Qt::SizeHorCursor);
   }
   else if(m_selectMode == E_ZOOM)
   {
      m_qwtPlot->canvas()->setCursor(*m_zoomCursor);
      pickerRubberBand = QwtPicker::RectRubberBand; // When in zoom mode, draw a rectangle to show the user the new zoom area.
   }
   else
   {
      m_qwtPlot->canvas()->setCursor(Qt::CrossCursor);
   }
   m_qwtMainPicker->setRubberBand(pickerRubberBand);
}

void MainWindow::on_radClearWrite_clicked()
{
   if(ui->radClearWrite->isChecked())
   {
      specAn_setTraceType(fftSpecAnFunc::E_CLEAR_WRITE);
   }
}

void MainWindow::on_radMaxHold_clicked()
{
   // If the Max Hold Radio Button has been double clicked, do a Clear Write to reset the Max Hold values.
   static QElapsedTimer doubleClickTimer;
   qint64 timeSinceLastClick_ms = doubleClickTimer.elapsed();
   doubleClickTimer.start();
   if(timeSinceLastClick_ms < 500)
      specAn_setTraceType(fftSpecAnFunc::E_CLEAR_WRITE);

   // Set to Max Hold.
   if(ui->radMaxHold->isChecked())
   {
      specAn_setTraceType(fftSpecAnFunc::E_MAX_HOLD);
   }
}

void MainWindow::on_radAverage_clicked()
{
   // If the Average Radio Button has been double clicked, do a Clear Write to reset the Average.
   static QElapsedTimer doubleClickTimer;
   qint64 timeSinceLastClick_ms = doubleClickTimer.elapsed();
   doubleClickTimer.start();
   if(timeSinceLastClick_ms < 500)
      specAn_setTraceType(fftSpecAnFunc::E_CLEAR_WRITE);

   // Set to Average.
   if(ui->radAverage->isChecked())
   {
      on_spnSpecAnAvgAmount_valueChanged(ui->spnSpecAnAvgAmount->value()); // Make sure the current GUI average amount is used.
      specAn_setTraceType(fftSpecAnFunc::E_AVERAGE);
   }
}

void MainWindow::specAn_setTraceType(fftSpecAnFunc::eFftSpecAnTraceType newTraceType)
{
   QMutexLocker lock(&m_qwtCurvesMutex);
   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      m_qwtCurves[i]->specAn_setTraceType(newTraceType);
   }
}

void MainWindow::setSpecAnGuiVisible(bool visible)
{
#ifdef Q_OS_LINUX // Windows 10 seems to override all the black on black nonsense by itself. Linux doesn't.
   // The default plotter Palette has a black background. This can cause some issues with
   // certain GUI elements in different Windows theme styles. The 'Standard Palette' seems
   // to work best in all situations.
   QPalette stdPalette = this->style()->standardPalette();
   ui->spnSpecAnAvgAmount->setPalette(stdPalette);
   ui->cmdPeakSearch->setPalette(stdPalette);

   // Radio buttons can use the 'Standard Palette', but they need to have their text
   // set to white to be readable against the black background.
   QPalette whiteTextPalette = stdPalette;
   whiteTextPalette.setColor(QPalette::WindowText, Qt::white);
   ui->radClearWrite->setPalette(whiteTextPalette);
   ui->radMaxHold->setPalette(whiteTextPalette);
   ui->radAverage->setPalette(whiteTextPalette);
   ui->groupSpecAnTrace->setPalette(whiteTextPalette);
   ui->groupSpecAnMarker->setPalette(whiteTextPalette);
#endif
   ui->groupSpecAnTrace->setVisible(visible);
   ui->groupSpecAnMarker->setVisible(visible);
}

void MainWindow::on_spnSpecAnAvgAmount_valueChanged(int arg1)
{
   QMutexLocker lock(&m_qwtCurvesMutex);
   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      m_qwtCurves[i]->specAn_setAvgSize(arg1);
   }
}

void MainWindow::on_cmdPeakSearch_clicked()
{
   QMutexLocker lock(&m_qwtCurvesMutex); // Make sure multiple threads can't modify the curves.

   bool validPointSelected = false;
   if(m_qwtSelectedSample != NULL)
   {
      validPointSelected = m_qwtSelectedSample->peakSearch(m_plotZoom->getCurZoom());
   }

   if(validPointSelected)
   {
      m_qwtSelectedSample->showCursor();
      updatePointDisplay();
      replotMainPlot(true, true);
   }
}

void MainWindow::on_cmdSpecAnResetZoom_clicked()
{
   resetZoom();
}

double MainWindow::getFftMeasurement(eFftSigNoiseMeasurements type)
{
   double retVal = NAN;
   QMutexLocker lock(&m_qwtCurvesMutex); // Make sure multiple threads can't modify the curves.
   if(m_snrCalcBars != NULL)
   {
      retVal = m_snrCalcBars->getMeasurement(type);
   }
   return retVal;
}

double MainWindow::getCurveStat(QString& curveName, eCurveStats type)
{
   double retVal = 0.0;
   QMutexLocker lock(&m_qwtCurvesMutex); // Make sure multiple threads can't modify the curves.

   int curveIndex = getCurveIndex(curveName);
   if(curveIndex >= 0 && curveIndex < m_qwtCurves.size())
   {
      CurveData* curve = m_qwtCurves[curveIndex];
      switch(type)
      {
         case E_CURVE_STATS__NUM_SAMP:
            retVal = curve->getNumPoints();
         break;
         case E_CURVE_STATS__X_MIN:
            retVal = curve->getMaxMinXYOfData().minX;
         break;
         case E_CURVE_STATS__X_MAX:
            retVal = curve->getMaxMinXYOfData().maxX;
         break;
         case E_CURVE_STATS__Y_MIN:
            retVal = curve->getMaxMinXYOfData().minY;
         break;
         case E_CURVE_STATS__Y_MAX:
            retVal = curve->getMaxMinXYOfData().maxY;
         break;
         case E_CURVE_STATS__SAMP_RATE:
            retVal = curve->getCalculatedSampleRateFromPlotMsgs();
         break;
         default:
         case E_CURVE_STATS__NO_CURVE_STAT:
            // Do Nothing, just return 0.
         break;
      }
   }
   return retVal;
}

bool MainWindow::areFftMeasurementsVisible()
{
   bool retVal = false;
   QMutexLocker lock(&m_qwtCurvesMutex); // Make sure multiple threads can't modify the curves.
   if(m_snrCalcBars != NULL)
   {
      retVal = m_snrCalcBars->isVisable();
   }
   return retVal;
}


void MainWindow::silentSavePlotToFile()
{
   std::string saveDir;
   if(persistentParam_getParam_str(PERSIST_PARAM_CURVE_SAVE_PREV_DIR_STR, saveDir))
   {
      if(fso::DirExists(saveDir))
      {
         double dubSaveType;
         if(persistentParam_getParam_f64(PERSIST_PARAM_PLOT_SAVE_PREV_SAVE_SELECTION_INDEX, dubSaveType))
         {
            QString plotName = getPlotName();
            eSaveRestorePlotCurveType saveType = (eSaveRestorePlotCurveType)((int)dubSaveType);
            std::string fileName = plotName.toStdString();

            // Determine Ext from Save Type
            std::string ext;
            switch(saveType)
            {
               case E_SAVE_RESTORE_CSV:
                  ext = ".csv";
               break;
               // Fall through on all header types.
               case E_SAVE_RESTORE_C_HEADER_AUTO_TYPE:
               case E_SAVE_RESTORE_C_HEADER_INT:
               case E_SAVE_RESTORE_C_HEADER_FLOAT:
                  ext = ".h";
               break;
               default:
                  ext = ".plot";
               break;
            }

            int fileNameAppendNum = 0;
            std::string fullPath = saveDir + fso::dirSep() + fileName + ext;
            while(fso::FileExists(fullPath))
            {
               fileNameAppendNum++;
               fullPath = saveDir + fso::dirSep() + fileName + "_" + QString::number(fileNameAppendNum).toStdString() + ext;
            }

            // Fill in vector of curve data in the correct order.
            tCurveCommanderInfo allPlots = m_curveCommander->getCurveCommanderInfo();
            QVector<CurveData*> curves;
            curves.resize(allPlots[plotName].curves.size());
            foreach( QString key, allPlots[plotName].curves.keys() )
            {
               int index = allPlots[plotName].plotGui->getCurveIndex(key);
               curves[index] = allPlots[plotName].curves[key];
            }

            // Save the plot data.
            SavePlot savePlot(this, plotName, curves, saveType);
            PackedCurveData dataToWriteToFile;
            savePlot.getPackedData(dataToWriteToFile);
            fso::WriteFile(fullPath, &dataToWriteToFile[0], dataToWriteToFile.size());
         }
         else
         {
            // TODO?
         }
      }
      else
      {
         // TODO?
      }
   }
}


void MainWindow::saveCurveAppearance(QString curveName, CurveAppearance& appearance)
{
   auto ppp = persistentPlotParam_get(g_persistentPlotParams, m_plotName);
   auto map = ppp->m_curveAppearanceMap.get();
   map[curveName] = appearance;
   ppp->m_curveAppearanceMap.set(map);
}

void MainWindow::fillWithSavedAppearance(QString& curveName, CurveAppearance& appearance)
{
   if(persistentPlotParam_get(g_persistentPlotParams, m_plotName)->m_curveAppearanceMap.isValid())
   {
      auto map = persistentPlotParam_get(g_persistentPlotParams, m_plotName)->m_curveAppearanceMap.get();
      if(map.find(curveName) != map.end())
      {
         appearance = map[curveName];
      }
   }
}

bool MainWindow::closeSubWindows()
{
   bool subWindowClosed = false;

   // Handle Zoom Limit Dialog Window.
   {
      QMutexLocker lock(&m_zoomLimitMutex);
      if(m_zoomLimitDialog != NULL)
      {
         m_zoomLimitDialog->cancel();
         subWindowClosed = true;
      }
   }

   return subWindowClosed;
}

void MainWindow::openSavePlotDialog(bool limitToZoom)
{
   SavePlotCurveDialog dialog(this, m_curveCommander, limitToZoom);
   dialog.savePlot(m_plotName);
}

