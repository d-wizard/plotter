/* Copyright 2013 - 2019 Dan Williams. All Rights Reserved.
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

// curveColors array is created from .h file, probably should be made into its own class at some point.
#include "curveColors.h"

#define DISPLAY_POINT_START "("
#define DISPLAY_POINT_MID   ","
#define DISPLAY_POINT_STOP  ")"
static const QString CAPITAL_DELTA = QChar(0x94, 0x03);

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

MainWindow::MainWindow(CurveCommander* curveCmdr, plotGuiMain* plotGui, QString plotName, QWidget *parent) :
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
   m_plotZoom(NULL),
   m_checkedIcon(":/check.png"),
   m_zoomCursor(NULL),
   m_normalizeCurves_xAxis(false),
   m_normalizeCurves_yAxis(false),
   m_legendDisplayed(false),
   m_calcSnrDisplayed(false),
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
   m_scrollModeAction("Scroll Mode", this),
   m_scrollModeChangePlotSizeAction("Change Scroll Plot Size", this),
   m_resetZoomAction("Reset Zoom", this),
   m_normalizeNoneAction("Disable", this),
   m_normalizeYOnlyAction("Y Axis Only", this),
   m_normalizeXOnlyAction("X Axis Only", this),
   m_normalizeBothAction("Both Axes", this),
   m_toggleLegendAction("Legend", this),
   m_toggleSnrCalcAction("Calculate SNR", this),
   m_toggleCursorCanSelectAnyCurveAction("Any Curve Cursor", this),
   m_normalizeMenu("Normalize Curves"),
   m_zoomSettingsMenu("Zoom Settings"),
   m_selectedCurvesMenu("Selected Curve"),
   m_visibleCurvesMenu("Visible Curves"),
   m_stylesCurvesMenu("Curve Style"),
   m_enableDisablePlotUpdate("Disable New Curves", this),
   m_curveProperties("Properties", this),
   m_defaultCurveStyle(QwtPlotCurve::Lines),
   m_displayType(E_DISPLAY_POINT_AUTO),
   m_displayPrecision(8),
   m_displayPointsAutoAction("Auto", this),
   m_displayPointsFixedAction("Decimal", this),
   m_displayPointsScientificAction("Scientific", this),
   m_displayPointsPrecisionAutoAction("Auto", this),
   m_displayPointsPrecisionUpAction("Precision +1", this),
   m_displayPointsPrecisionDownAction("Precision -1", this),
   m_displayPointsPrecisionUpBigAction("Precision +3", this),
   m_displayPointsPrecisionDownBigAction("Precision -3", this),
   m_displayPointsCopyToClipboard("Copy to Clipboard", this),
   m_activityIndicator_plotIsActive(true),
   m_activityIndicator_indicatorState(true),
   m_activityIndicator_inactiveCount(0),
   m_snrCalcBars(NULL),
   m_dragZoomModeActive(false),
   m_moveCalcSnrBarActive(false),
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
    connect(&m_autoZoomAction, SIGNAL(triggered(bool)), this, SLOT(autoZoom()));
    connect(&m_holdZoomAction, SIGNAL(triggered(bool)), this, SLOT(holdZoom()));
    connect(&m_maxHoldZoomAction, SIGNAL(triggered(bool)), this, SLOT(maxHoldZoom()));
    connect(&m_scrollModeAction, SIGNAL(triggered(bool)), this, SLOT(scrollModeToggle()));
    connect(&m_scrollModeChangePlotSizeAction, SIGNAL(triggered(bool)), this, SLOT(scrollModeChangePlotSize()));
    connect(&m_normalizeNoneAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurvesNone()));
    connect(&m_normalizeYOnlyAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurvesYOnly()));
    connect(&m_normalizeXOnlyAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurvesXOnly()));
    connect(&m_normalizeBothAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurvesBoth()));
    connect(&m_toggleLegendAction, SIGNAL(triggered(bool)), this, SLOT(toggleLegend()));
    connect(&m_toggleSnrCalcAction, SIGNAL(triggered(bool)), this, SLOT(calcSnrToggle()));
    connect(&m_toggleCursorCanSelectAnyCurveAction, SIGNAL(triggered(bool)), this, SLOT(toggleCursorCanSelectAnyCurveAction()));
    connect(&m_enableDisablePlotUpdate, SIGNAL(triggered(bool)), this, SLOT(togglePlotUpdateAbility()));
    connect(&m_curveProperties, SIGNAL(triggered(bool)), this, SLOT(showCurveProperties()));

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
    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addMenu(&m_stylesCurvesMenu);

    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_scrollModeAction);
    m_rightClickMenu.addAction(&m_scrollModeChangePlotSizeAction);

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
    MAPPER_ACTION_TO_SLOT(m_displayPointsMenu, m_displayPointsCopyToClipboard,     0, displayPointsCopyToClipboard);


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
    m_snrCalcBars = new plotSnrCalc(m_qwtPlot, ui->snrLabel);

    m_toggleCursorCanSelectAnyCurveAction.setVisible(false); // Set to Invisible until more than 1 curve is displayed

    restorePersistentPlotParams();
}

MainWindow::~MainWindow()
{
    delete m_snrCalcBars;

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
    m_qwtMainPicker->setRubberBand( QwtPicker::CrossRubberBand );
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
         UnpackPlotMsg* plotMsg = (*iter);
         QString curveName( plotMsg->m_curveName.c_str() );
         int curveIndex = getCurveIndex(curveName);
         CurveData* curveDataPtr = curveIndex >= 0 ? m_qwtCurves[curveIndex] : NULL;
         m_curveCommander->curveUpdated(plotMsg, curveDataPtr, false);
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
            m_curveCommander->curveUpdated(plotMsg, m_qwtCurves[curveIndex], true);

            // Make sure the SNR Calc bars are updated. If a new curve is plotted that is a
            // valid SNR curve, make sure the SNR Calc action is made visable.
            bool validSnrCurve = m_snrCalcBars->curveUpdated(m_qwtCurves[curveIndex], m_qwtCurves);
            if(validSnrCurve && m_toggleSnrCalcAction.isVisible() == false)
            {
               m_toggleSnrCalcAction.setVisible(true);
            }

            // If this is a new FFT plot / curve, initialize to Max Hold Zoom mode. This is because
            // the max / min of the Y axis can jitter around in FFT plots (especially the min of
            // the Y axis).
            if(validSnrCurve && newCurveAdded && firstCurve && !m_plotZoom->m_maxHoldZoom)
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


void MainWindow::setCurveHidden(QString curveName, bool hidden)
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    int curveIndex = getCurveIndex(curveName);
    if(curveIndex >= 0)
    {
       bool curveChanged = m_qwtCurves[curveIndex]->setHidden(hidden);

       if(curveChanged)
       {
          handleCurveDataChange(curveIndex);
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
         m_qwtCurves[curveIndex]->UpdateCurveSamples(unpackPlotMsg, m_scrollMode);
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
            // For 1D, prepend vector with zeros.
            unpackPlotMsg->m_yAxisValues.insert(unpackPlotMsg->m_yAxisValues.begin(), unpackPlotMsg->m_sampleStartIndex, 0.0);
         }
         else
         {
            // For 2D, prepend vector with 'Not a Number' values (i.e. values that won't show up on the plot).
            unpackPlotMsg->m_xAxisValues.insert(unpackPlotMsg->m_xAxisValues.begin(), unpackPlotMsg->m_sampleStartIndex, NAN);
            unpackPlotMsg->m_yAxisValues.insert(unpackPlotMsg->m_yAxisValues.begin(), unpackPlotMsg->m_sampleStartIndex, NAN);
         }
      }

      CurveAppearance newCurveAppearance(curveColors[colorLookupIndex], m_defaultCurveStyle);

      m_qwtCurves.push_back(new CurveData(m_qwtPlot, newCurveAppearance, unpackPlotMsg));

      m_plotZoom->m_plotIs1D = areAllCurves1D(); // The Zoom class needs to know if there are non-1D plots for the Max Hold functionality.

      // This is a new curve. If this is a child curve, there may be some final initialization that still needs to be done.
      m_curveCommander->doFinalChildCurveInit(getPlotName(), name);

      // The curve style for 2D plots may be different from the default value (i.e. Lines).
      if(plotDim == E_PLOT_DIM_2D)
      {
         setNew2dPlotStyle(name);
      }
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

void MainWindow::handleCurveDataChange(int curveIndex)
{
   initCursorIndex(curveIndex);

   updatePlotWithNewCurveData(false);

   // inform parent that a curve has been added / changed
   m_curveCommander->curveUpdated( this->getPlotName(),
                                   m_qwtCurves[curveIndex]->getCurveTitle(),
                                   m_qwtCurves[curveIndex],
                                   0,
                                   m_qwtCurves[curveIndex]->getNumPoints() );
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
   m_qwtPlot->replot();
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
   m_qwtMainPicker->setRubberBand( QwtPicker::CrossRubberBand );
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

        m_qwtMainPicker->setRubberBand( QwtPicker::CrossRubberBand );
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
    m_qwtMainPicker->setRubberBand( QwtPicker::RectRubberBand);
    setCursor();
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
   m_plotZoom->SetPlotDimensions(maxMin, true);
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
   m_plotZoom->SetPlotDimensions(maxMin, true);
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
      m_scrollModeChangePlotSizeAction.setVisible(true);
   }
   else
   {
      m_scrollModeAction.setIcon(QIcon());
      m_scrollModeChangePlotSizeAction.setVisible(false);
   }
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

void MainWindow::scrollModeSetPlotSize(int newPlotSize)
{
   QMutexLocker lock(&m_qwtCurvesMutex);
   size_t numCurves = m_qwtCurves.size();
   for(size_t curveIndex = 0; curveIndex < numCurves; ++curveIndex)
   {
      m_qwtCurves[curveIndex]->setNumPoints(newPlotSize, true);
      handleCurveDataChange(curveIndex);
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
    if(m_qwtCurves[index]->isDisplayed())
    {
        m_qwtCurves[index]->detach();
    }
    else
    {
        m_qwtCurves[index]->attach();
    }
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

maxMinXY MainWindow::calcMaxMin()
{
   QMutexLocker lock(&m_qwtCurvesMutex); // Make sure multiple threads can't modify m_maxMin at the same time.
   
   int i = 0;
   while(i < m_qwtCurves.size() && m_qwtCurves[i]->isDisplayed() == false)
   {
      ++i;
   }
   if(i < m_qwtCurves.size())
   {
      m_maxMin = m_qwtCurves[i]->getMaxMinXYOfCurve();
      while(i < m_qwtCurves.size())
      {
         if(m_qwtCurves[i]->isDisplayed())
         {
            maxMinXY curveMaxMinXY = m_qwtCurves[i]->getMaxMinXYOfCurve();

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


void MainWindow::pointSelected(const QPointF &pos)
{
   // Check if we should switch to dragging SNR Calc Bar mode.
   if( m_selectMode == E_CURSOR && m_snrCalcBars->isVisable() &&
       m_snrCalcBars->isSelectionCloseToBar(pos, m_plotZoom->getCurZoom(),
                                       m_canvasWidth_pixels, m_canvasHeight_pixels))
   {
      // The user clicked on a SNR Calc Bar. Activate the slot that tracks the cursor
      // movement while the left mouse button is held down.
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

         if(m_cursorCanSelectAnyCurve)
         {
            // Find the closest point amoung all the curves.
            unsigned int selectedCurvePointIndex = 0;
            int selectCurveIndex = findIndexWithClosestPoint(pos, selectedCurvePointIndex);

            validPointSelected = selectCurveIndex >= 0; // If curve index is negative, something when wrong.

            if(validPointSelected)
            {
               // Update the selected curve index.
               setSelectedCurveIndex(selectCurveIndex);
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
   if(m_selectMode == E_ZOOM)
   {
      QRectF rectCopy = pos;
      maxMinXY zoomDim;

      rectCopy.getCoords(&zoomDim.minX, &zoomDim.minY, &zoomDim.maxX, &zoomDim.maxY);

      m_plotZoom->SetZoom(zoomDim);

   }

   // This function is called when the user unclicks the left mouse button. Reset the state if a Calc SNR was being moved.
   if(m_moveCalcSnrBarActive)
   {
      m_moveCalcSnrBarActive = false;
      setCursor(); // Set the cursor back to normal.
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
}


void MainWindow::displayPointLabels_getLabelText(std::stringstream& lblText, CurveData* curve, unsigned int cursorIndex)
{
   bool displayFormatSet = setDisplayIoMapipXAxis(lblText, curve);
   lblText << DISPLAY_POINT_START << curve->getXPoints()[cursorIndex] << DISPLAY_POINT_MID;

   if(displayFormatSet == false)
   {
       setDisplayIoMapipYAxis(lblText);
       displayFormatSet = true;
   }
   lblText << curve->getYPoints()[cursorIndex] << DISPLAY_POINT_STOP;

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
            displayPointLabels_getLabelText(lblText, m_qwtCurves[i], m_qwtSelectedSample->m_pointIndex);
            
            m_qwtCurves[i]->pointLabel->setText(lblText.str().c_str());

            QPalette palette = this->palette();
            palette.setColor( QPalette::WindowText, m_qwtCurves[i]->getColor());
            palette.setColor( QPalette::Text, m_qwtCurves[i]->getColor());
            m_qwtCurves[i]->pointLabel->setPalette(palette);

            ui->InfoLayout->addWidget(m_qwtCurves[i]->pointLabel);

            connectPointLabelToRightClickMenu(m_qwtCurves[i]->pointLabel);

        }
    }
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
            displayPointLabels_getLabelText(lblText, m_qwtCurves[i], m_qwtSelectedSample->m_pointIndex);
            
            m_qwtCurves[i]->pointLabel->setText(lblText.str().c_str());
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
}


void MainWindow::initDeltaLabels()
{
   for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
   {
      m_deltaLabels[i] = NULL;
   }
}

void MainWindow::displayDeltaLabel_getLabelText(QString& anchored, QString& current, QString& delta)
{
   CurveData* curve = m_qwtSelectedSample->m_parentCurve;
   std::stringstream lblText;

   // Get Anchored text.
   lblText.str("");
   lblText << DISPLAY_POINT_START;
   setDisplayIoMapipXAxis(lblText, curve);
   lblText << m_qwtSelectedSampleDelta->m_xPoint << DISPLAY_POINT_MID;
   setDisplayIoMapipYAxis(lblText);
   lblText << m_qwtSelectedSampleDelta->m_yPoint << DISPLAY_POINT_STOP;
   anchored = QString(lblText.str().c_str());

   // Get Current text.
   lblText.str("");
   lblText << DISPLAY_POINT_START;
   setDisplayIoMapipXAxis(lblText, curve);
   lblText << m_qwtSelectedSample->m_xPoint << DISPLAY_POINT_MID;
   setDisplayIoMapipYAxis(lblText);
   lblText << m_qwtSelectedSample->m_yPoint << DISPLAY_POINT_STOP;
   current = QString(lblText.str().c_str());

   // Get Delta text.
   lblText.str("");
   lblText << DISPLAY_POINT_START;
   setDisplayIoMapipXAxis(lblText, curve);
   lblText << (m_qwtSelectedSample->m_xPoint-m_qwtSelectedSampleDelta->m_xPoint) << DISPLAY_POINT_MID;
   setDisplayIoMapipYAxis(lblText);
   lblText << (m_qwtSelectedSample->m_yPoint-m_qwtSelectedSampleDelta->m_yPoint) << DISPLAY_POINT_STOP;
   delta = QString(lblText.str().c_str());
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

      QColor anchoredColor = m_qwtSelectedSampleDelta->getCurve()->getColor();
      QColor currentColor  = m_qwtSelectedSample->getCurve()->getColor();

      QPalette anchoredPalette = this->palette();
      anchoredPalette.setColor( QPalette::WindowText, anchoredColor);
      anchoredPalette.setColor( QPalette::Text, anchoredColor);

      QPalette currentPalette = this->palette();
      currentPalette.setColor( QPalette::WindowText, currentColor);
      currentPalette.setColor( QPalette::Text, currentColor);

      m_deltaLabels[DELTA_LABEL_ANCHORED]->setPalette(anchoredPalette);
      m_deltaLabels[DELTA_LABEL_CURRENT]->setPalette(currentPalette);

      if(m_qwtSelectedSampleDelta->getCurve() == m_qwtSelectedSample->getCurve())
      {
         // Delta and Current Selected Curves match, set the rest of the labels to match the same color
         m_deltaLabels[DELTA_LABEL_DELTA]->setPalette(currentPalette);
         m_deltaLabels[DELTA_LABEL_SEP1]->setPalette(currentPalette);
         m_deltaLabels[DELTA_LABEL_SEP2]->setPalette(currentPalette);
      }
      else
      {
         // Delta and Current Selected Curves are different, set the rest of the labels colors to white.
         QPalette nullPalette = this->palette();
         nullPalette.setColor( QPalette::WindowText, Qt::white);
         nullPalette.setColor( QPalette::Text, Qt::white);

         m_deltaLabels[DELTA_LABEL_DELTA]->setPalette(nullPalette);
         m_deltaLabels[DELTA_LABEL_SEP1]->setPalette(nullPalette);
         m_deltaLabels[DELTA_LABEL_SEP2]->setPalette(nullPalette);
      }

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

   QString anchored, current, delta;
   displayDeltaLabel_getLabelText(anchored, current, delta);

   m_deltaLabels[DELTA_LABEL_ANCHORED]->setText(anchored);
   m_deltaLabels[DELTA_LABEL_CURRENT]->setText(current);
   m_deltaLabels[DELTA_LABEL_DELTA]->setText(delta);
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
         QString anchored, current, delta;
         displayDeltaLabel_getLabelText(anchored, current, delta);

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

            if(KeyEvent->key() == Qt::Key_Z && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
                m_plotZoom->changeZoomFromSavedZooms(-1);
            }
            else if(KeyEvent->key() == Qt::Key_Y && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
                m_plotZoom->changeZoomFromSavedZooms(1);
            }
            else if(KeyEvent->key() == Qt::Key_S && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
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
            else if(KeyEvent->key() == Qt::Key_E && KeyEvent->modifiers().testFlag(Qt::ControlModifier) && !m_allowNewCurves)
            {
               // Enable new plot messages.
               togglePlotUpdateAbility();
            }
            else if(KeyEvent->key() == Qt::Key_D && KeyEvent->modifiers().testFlag(Qt::ControlModifier) && m_allowNewCurves)
            {
               // Disable new plot messages.
               togglePlotUpdateAbility();
            }
            else if(KeyEvent->key() == Qt::Key_T && KeyEvent->modifiers().testFlag(Qt::ControlModifier))
            {
               // Toggle new plot messages.
               togglePlotUpdateAbility();
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
               SavePlot savePlot(this, getPlotName(), curveVect, E_SAVE_RESTORE_CLIPBOARD_EXCEL);
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

                if(wheelEvent->delta() > 0)
                {
                    m_plotZoom->Zoom(ZOOM_IN_PERCENT, relMousePos, holdYAxis);
                }
                else if(wheelEvent->delta() < 0)
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
      bool deltaCouldBeDifferentCurve = m_cursorCanSelectAnyCurve && m_qwtSelectedSampleDelta->isAttached;
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

            m_plotZoom->SetPlotDimensions(getPlotDimWithNormalization(), true);
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
      m_plotZoom->SetPlotDimensions(getPlotDimWithNormalization(), changeCausedByUserGuiInput);
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

   if(!cursorCanSelectAnyCurveVisible && m_cursorCanSelectAnyCurve)
   {
      // Not enough curves to all for "Cursor Can Select Any Curve" mode, but we are in that mode. Toggle out of the mode.
      toggleCursorCanSelectAnyCurveAction();
   }

   m_qwtPlot->replot();
}

void MainWindow::ShowRightClickForPlot(const QPoint& pos) // this is a slot
{
    m_rightClickMenu.exec(m_qwtPlot->mapToGlobal(pos));
}

void MainWindow::ShowRightClickForDisplayPoints(const QPoint& pos)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   bool menuHasBeenDisplayed = false;

   QPoint labelPos;

   // Kinda ugly, but this is the simplist way I found to display the right click menu
   // when right clicking in the display area. Basically, for loop through all the curves
   // until a valid point label is found. Then use that point label for the right click menu and exit.
   for(int i = 0; i < m_qwtCurves.size(); ++i)
   {
      if(m_qwtCurves[i]->pointLabel != NULL)
      {
         labelPos = m_qwtCurves[i]->pointLabel->pos();
         m_displayPointsMenu.exec(m_qwtCurves[i]->pointLabel->mapToGlobal(pos));
         menuHasBeenDisplayed = true;
         break;
      }
   }

   // Try to display the menu via the Delta Labels
   if(menuHasBeenDisplayed == false)
   {
      for(int i = 0; i < DELTA_LABEL_NUM_LABELS; ++i)
      {
         if(m_deltaLabels[i] != NULL)
         {
            m_displayPointsMenu.exec(m_deltaLabels[i]->mapToGlobal(pos));
            menuHasBeenDisplayed = true;
            break;
         }
      }
   }
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
         m_qwtCurves.move(curIndex, newIndex);
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
           m_qwtCurves[i]->detach();
       }
   }
   for(int i = 0; i < visableCurves.size(); ++i)
   {
       visableCurves[i]->attach();
   }

   // Make sure the selected point index is still valid.
   int numCurves = m_qwtCurves.size();
   if(m_selectedCurveIndex >= numCurves || m_selectedCurveIndex < 0)
   {
      int boundedSelectedCurveIndex = numCurves > 0 ? numCurves-1 : 0;
      setSelectedCurveIndex(boundedSelectedCurveIndex);
   }

   updatePointDisplay();
   m_snrCalcBars->moveToFront();
   replotMainPlot();
   maxMinXY maxMin = calcMaxMin();
   m_plotZoom->SetPlotDimensions(maxMin, true);
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
   if(m_dragZoomModeActive)
   {
      m_qwtPlot->canvas()->setCursor(Qt::OpenHandCursor);
   }
   else if(m_moveCalcSnrBarActive)
   {
      m_qwtPlot->canvas()->setCursor(Qt::SizeHorCursor);
   }
   else if(m_selectMode == E_ZOOM)
   {
      m_qwtPlot->canvas()->setCursor(*m_zoomCursor);
   }
   else
   {
      m_qwtPlot->canvas()->setCursor(Qt::CrossCursor);
   }
}
