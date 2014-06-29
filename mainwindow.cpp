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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <QSignalMapper>
#include <QKeyEvent>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <QBitmap>
#include <qwt_plot_canvas.h>

#include "CurveCommander.h"
#include "plotguimain.h"

// curveColors array is created from .h file, probably should be made into its own class at some point.
#include "curveColors.h"


#define MAPPER_ACTION_TO_SLOT(menu, mapperAction, intVal, callback) \
menu.addAction(&mapperAction.m_action); \
mapperAction.m_mapper.setMapping(&mapperAction.m_action, (int)intVal); \
connect(&mapperAction.m_action,SIGNAL(triggered()), &mapperAction.m_mapper,SLOT(map())); \
connect(&mapperAction.m_mapper, SIGNAL(mapped(int)), SLOT(callback(int)) );


MainWindow::MainWindow(CurveCommander* curveCmdr, plotGuiMain* plotGui, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_curveCommander(curveCmdr),
    m_plotGuiMain(plotGui),
    m_qwtPlot(NULL),
    m_qwtCurvesMutex(QMutex::Recursive),
    m_qwtSelectedSample(NULL),
    m_qwtSelectedSampleDelta(NULL),
    m_qwtPicker(NULL),
    m_qwtGrid(NULL),
    m_selectMode(E_CURSOR),
    m_selectedCurveIndex(0),
    m_plotZoom(NULL),
    m_checkedIcon(":/check.png"),
    m_zoomCursor(NULL),
    m_normalizeCurves(false),
    m_legendDisplayed(false),
    m_canvasXOverYRatio(1.0),
    m_allowNewCurves(true),
    m_zoomAction("Zoom", this),
    m_cursorAction("Cursor", this),
    m_deltaCursorAction("Delta Cursor", this),
    m_resetZoomAction("Reset Zoom", this),
    m_normalizeAction("Normalize Curves", this),
    m_toggleLegendAction("Legend", this),
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
    m_displayPointsPrecisionDownBigAction("Precision -3", this)
{
    ui->setupUi(this);

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

    connect(this, SIGNAL(updateCursorMenusSignal()),
            this, SLOT(updateCursorMenus()), Qt::QueuedConnection);

    // Connect menu commands
    connect(&m_zoomAction, SIGNAL(triggered(bool)), this, SLOT(zoomMode()));
    connect(&m_cursorAction, SIGNAL(triggered(bool)), this, SLOT(cursorMode()));
    connect(&m_resetZoomAction, SIGNAL(triggered(bool)), this, SLOT(resetZoom()));
    connect(&m_deltaCursorAction, SIGNAL(triggered(bool)), this, SLOT(deltaCursorMode()));
    connect(&m_normalizeAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurves()));
    connect(&m_toggleLegendAction, SIGNAL(triggered(bool)), this, SLOT(toggleLegend()));
    connect(&m_enableDisablePlotUpdate, SIGNAL(triggered(bool)), this, SLOT(togglePlotUpdateAbility()));
    connect(&m_curveProperties, SIGNAL(triggered(bool)), this, SLOT(showCurveProperties()));

    m_cursorAction.setIcon(m_checkedIcon);

    resetPlot();

    ui->verticalScrollBar->setRange(0,0);
    ui->horizontalScrollBar->setRange(0,0);
    ui->verticalScrollBar->setVisible(false);
    ui->horizontalScrollBar->setVisible(false);

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
      this, SLOT(onApplicationFocusChanged(QWidget*,QWidget*)));

    m_rightClickMenu.addAction(&m_zoomAction);
    m_rightClickMenu.addAction(&m_cursorAction);
    m_rightClickMenu.addAction(&m_deltaCursorAction);
    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_resetZoomAction);
    m_rightClickMenu.addAction(&m_normalizeAction);
    m_rightClickMenu.addAction(&m_toggleLegendAction);
    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addMenu(&m_visibleCurvesMenu);
    m_rightClickMenu.addMenu(&m_selectedCurvesMenu);

    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addMenu(&m_stylesCurvesMenu);

    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_enableDisablePlotUpdate);

    m_rightClickMenu.addSeparator();
    m_rightClickMenu.addAction(&m_curveProperties);

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
}

MainWindow::~MainWindow()
{
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
    if(m_qwtPicker != NULL)
    {
        delete m_qwtPicker;
        m_qwtPicker = NULL;
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
    m_plotZoom = new PlotZoom(m_qwtPlot, ui->verticalScrollBar, ui->horizontalScrollBar);

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

    if(m_qwtPicker != NULL)
    {
        delete m_qwtPicker;
    }

    m_qwtPicker = new QwtPlotPicker(m_qwtPlot->canvas());

    // setStateMachine wants to be called with new and handles delete.
    m_qwtPicker->setStateMachine(new QwtPickerDragRectMachine());//QwtPickerDragPointMachine());//

    m_qwtPicker->setRubberBandPen( QColor( Qt::green ) );
    m_qwtPicker->setRubberBand( QwtPicker::CrossRubberBand );
    m_qwtPicker->setTrackerPen( QColor( Qt::white ) );

    m_qwtPlot->canvas()->setCursor(Qt::CrossCursor);

    // Initialize in cursor mode
    cursorMode();

    m_qwtPlot->show();
    m_qwtGrid->show();

    // TODO: Do I need to disconnect the action
    connect(m_qwtPicker, SIGNAL(appended(QPointF)),
            this, SLOT(pointSelected(QPointF)));
    connect(m_qwtPicker, SIGNAL(selected(QRectF)),
            this, SLOT(rectSelected(QRectF)));
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

void MainWindow::create1dCurve(QString name, ePlotType plotType, dubVect &yPoints)
{
    createUpdateCurve(name, true, 0, plotType, NULL, &yPoints);
}

void MainWindow::create2dCurve(QString name, dubVect &xPoints, dubVect &yPoints)
{
    createUpdateCurve(name, true, 0, E_PLOT_TYPE_2D, &xPoints, &yPoints);
}

void MainWindow::update1dCurve(QString name, unsigned int sampleStartIndex, ePlotType plotType, dubVect& yPoints)
{
    createUpdateCurve(name, false, sampleStartIndex, plotType, NULL, &yPoints);
}

void MainWindow::update2dCurve(QString name, unsigned int sampleStartIndex, dubVect& xPoints, dubVect& yPoints)
{
    createUpdateCurve(name, false, sampleStartIndex, E_PLOT_TYPE_2D, &xPoints, &yPoints);
}

void MainWindow::setCurveSampleRate(QString curveName, double sampleRate, bool userSpecified)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   int curveIndex = getCurveIndex(curveName);
   if(curveIndex >= 0)
   {
      if(m_qwtCurves[curveIndex]->setSampleRate(sampleRate, userSpecified))
      {
         handleCurveDataChange(curveIndex, 0, m_qwtCurves[curveIndex]->getNumPoints());
      }
   }
}


void MainWindow::setCurveProperties(QString curveName, eAxis axis, double sampleRate, tMathOpList& mathOps, bool hidden)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   int curveIndex = getCurveIndex(curveName);
   if(curveIndex >= 0)
   {
      bool curveChanged = false;
      curveChanged |= m_qwtCurves[curveIndex]->setSampleRate(sampleRate, true);
      curveChanged |= m_qwtCurves[curveIndex]->setMathOps(mathOps, axis);
      curveChanged |= m_qwtCurves[curveIndex]->setHidden(hidden);

      if(curveChanged)
      {
         handleCurveDataChange(curveIndex, 0, m_qwtCurves[curveIndex]->getNumPoints());
      }
   }
}

void MainWindow::createUpdateCurve( QString& name,
                                    bool resetCurve,
                                    unsigned int sampleStartIndex,
                                    ePlotType plotType,
                                    dubVect *xPoints,
                                    dubVect *yPoints )
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   // Check for any reason to not allow the adding of the new curve.
   if( m_allowNewCurves == false ||
       yPoints == NULL ||
       (xPoints != NULL && xPoints->size() <= 0) ||
       yPoints->size() <= 0 )
   {
      return;
   }

   int curveIndex = getCurveIndex(name);
   if(curveIndex >= 0)
   {
      if(resetCurve == true)
      {
         if(xPoints == NULL)
         {
            m_qwtCurves[curveIndex]->ResetCurveSamples(*yPoints);
         }
         else
         {
            m_qwtCurves[curveIndex]->ResetCurveSamples(*xPoints, *yPoints);
         }
      }
      else
      {
         if(xPoints == NULL)
         {
            m_qwtCurves[curveIndex]->UpdateCurveSamples(*yPoints, sampleStartIndex);
         }
         else
         {
            m_qwtCurves[curveIndex]->UpdateCurveSamples(*xPoints, *yPoints, sampleStartIndex);
         }
      }
   }
   else
   {
      curveIndex = m_qwtCurves.size();
      int colorLookupIndex = curveIndex % ARRAY_SIZE(curveColors);

      if(resetCurve == false && sampleStartIndex > 0)
      {
         // New curve, but starting in the middle. Prepend vector with zeros.
         if(xPoints != NULL)
         {
            xPoints->insert(xPoints->begin(), sampleStartIndex, 0.0);
         }
         yPoints->insert(yPoints->begin(), sampleStartIndex, 0.0);
      }

      CurveAppearance newCurveAppearance(curveColors[colorLookupIndex], m_defaultCurveStyle);

      if(xPoints == NULL)
      {
         m_qwtCurves.push_back(new CurveData(m_qwtPlot, name, plotType, *yPoints, newCurveAppearance));
      }
      else
      {
         m_qwtCurves.push_back(new CurveData(m_qwtPlot, name, *xPoints, *yPoints, newCurveAppearance));
      }
   }

   handleCurveDataChange(curveIndex, sampleStartIndex, yPoints->size());
}

void MainWindow::handleCurveDataChange(int curveIndex, unsigned int sampleStartIndex, unsigned int numPoints)
{
   calcMaxMin();

   if(m_qwtSelectedSample->getCurve() == NULL)
   {
      setSelectedCurveIndex(curveIndex);
   }
   m_plotZoom->ResetZoom();
   replotMainPlot();

   // Make sure the cursors matches the updated point.
   if(m_qwtSelectedSample->isAttached == true)
   {
      m_qwtSelectedSample->showCursor();
   }
   if(m_qwtSelectedSampleDelta->isAttached == true)
   {
      m_qwtSelectedSampleDelta->showCursor();
   }
   updatePointDisplay();

   emit updateCursorMenusSignal();

   // inform parent that a curve has been added / changed
   m_curveCommander->curveUpdated( this->windowTitle(),
                                   m_qwtCurves[curveIndex]->getCurveTitle(),
                                   m_qwtCurves[curveIndex],
                                   sampleStartIndex,
                                   numPoints );
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
}


void MainWindow::cursorMode()
{
    m_selectMode = E_CURSOR;
    m_cursorAction.setIcon(m_checkedIcon);
    m_zoomAction.setIcon(QIcon());
    m_deltaCursorAction.setIcon(QIcon());
    m_qwtPicker->setStateMachine( new QwtPickerDragRectMachine() );
    m_qwtSelectedSampleDelta->hideCursor();
    m_qwtPicker->setRubberBand( QwtPicker::CrossRubberBand );
    m_qwtPlot->canvas()->setCursor(Qt::CrossCursor);
    updatePointDisplay();
    replotMainPlot();
}


void MainWindow::deltaCursorMode()
{
    eSelectMode prevMode = m_selectMode;
    m_selectMode = E_DELTA_CURSOR;
    m_deltaCursorAction.setIcon(m_checkedIcon);
    m_zoomAction.setIcon(QIcon());
    m_cursorAction.setIcon(QIcon());

    if(prevMode != E_ZOOM)
    {
        m_qwtSelectedSampleDelta->setCurve(m_qwtSelectedSample->getCurve());
        m_qwtSelectedSampleDelta->showCursor(
            QPointF(m_qwtSelectedSample->m_xPoint,m_qwtSelectedSample->m_yPoint),
            m_maxMin,
            m_canvasXOverYRatio);

        m_qwtPicker->setRubberBand( QwtPicker::CrossRubberBand );
        m_qwtSelectedSample->hideCursor();
    }
    m_qwtPlot->canvas()->setCursor(Qt::CrossCursor);
    updatePointDisplay();
    replotMainPlot();
}


void MainWindow::zoomMode()
{
    m_selectMode = E_ZOOM;
    m_zoomAction.setIcon(m_checkedIcon);
    m_cursorAction.setIcon(QIcon());
    m_deltaCursorAction.setIcon(QIcon());
    m_qwtPicker->setRubberBand( QwtPicker::RectRubberBand);
    m_qwtPlot->canvas()->setCursor(*m_zoomCursor);
}


void MainWindow::resetZoom()
{
    calcMaxMin();
    m_plotZoom->ResetZoom();
}

void MainWindow::normalizeCurves()
{
    m_normalizeCurves = !m_normalizeCurves;
    if(m_normalizeCurves)
    {
        m_normalizeAction.setIcon(m_checkedIcon);
    }
    else
    {
        m_normalizeAction.setIcon(QIcon());
        m_plotZoom->SetPlotDimensions(m_maxMin);
    }
    replotMainPlot();
    m_plotZoom->ResetZoom();
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
   m_curveCommander->showCurvePropertiesGui(windowTitle(), m_qwtCurves[m_selectedCurveIndex]->getCurveTitle());
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

void MainWindow::calcMaxMin()
{
    QMutexLocker lock(&m_qwtCurvesMutex);

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
                if(m_maxMin.minX > curveMaxMinXY.minX)
                {
                    m_maxMin.minX = curveMaxMinXY.minX;
                }
                if(m_maxMin.minY > curveMaxMinXY.minY)
                {
                    m_maxMin.minY = curveMaxMinXY.minY;
                }
                if(m_maxMin.maxX < curveMaxMinXY.maxX)
                {
                    m_maxMin.maxX = curveMaxMinXY.maxX;
                }
                if(m_maxMin.maxY < curveMaxMinXY.maxY)
                {
                    m_maxMin.maxY = curveMaxMinXY.maxY;
                }
            }
            ++i;
        }
    }
    else
    {
        // TODO: Should do something when there are no curves displayed
    }
    m_plotZoom->SetPlotDimensions(m_maxMin);
}




void MainWindow::pointSelected(const QPointF &pos)
{
    if(m_selectMode == E_CURSOR || m_selectMode == E_DELTA_CURSOR)
    {
        m_qwtSelectedSample->showCursor(pos, m_plotZoom->getCurZoom(), m_canvasXOverYRatio);

        updatePointDisplay();
        replotMainPlot();

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

    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i]->pointLabel != NULL)
        {
            ui->InfoLayout->removeWidget(m_qwtCurves[i]->pointLabel);
            delete m_qwtCurves[i]->pointLabel;
            m_qwtCurves[i]->pointLabel = NULL;
        }
    }
}
void MainWindow::displayPointLabels()
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    clearPointLabels();
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i]->isDisplayed() && m_qwtSelectedSample->m_pointIndex < m_qwtCurves[i]->getNumPoints())
        {
            m_qwtCurves[i]->pointLabel = new QLabel("");

            bool displayFormatSet = false;
            std::stringstream lblText;

            displayFormatSet = setDisplayIoMapipXAxis(lblText, m_qwtCurves[i]);
            lblText << "(" << m_qwtCurves[i]->getXPoints()[m_qwtSelectedSample->m_pointIndex] << ",";

            if(displayFormatSet == false)
            {
                setDisplayIoMapipYAxis(lblText);
                displayFormatSet = true;
            }
            lblText << m_qwtCurves[i]->getYPoints()[m_qwtSelectedSample->m_pointIndex] << ")";

            m_qwtCurves[i]->pointLabel->setText(lblText.str().c_str());

            QPalette palette = this->palette();
            palette.setColor( QPalette::WindowText, m_qwtCurves[i]->getColor());
            palette.setColor( QPalette::Text, m_qwtCurves[i]->getColor());
            m_qwtCurves[i]->pointLabel->setPalette(palette);

            ui->InfoLayout->addWidget(m_qwtCurves[i]->pointLabel);

            m_qwtCurves[i]->pointLabel->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(m_qwtCurves[i]->pointLabel, SIGNAL(customContextMenuRequested(const QPoint&)),
                this, SLOT(ShowRightClickForDisplayPoints(const QPoint&)));

        }
    }
}
void MainWindow::displayDeltaLabel()
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    clearPointLabels();
    if(m_qwtCurves[m_selectedCurveIndex]->isDisplayed())
    {
        m_qwtCurves[m_selectedCurveIndex]->pointLabel = new QLabel("");

        CurveData* curve = m_qwtSelectedSample->m_parentCurve;
        std::stringstream lblText;
        lblText << "(";
        setDisplayIoMapipXAxis(lblText, curve);
        lblText << m_qwtSelectedSampleDelta->m_xPoint << ",";
        setDisplayIoMapipYAxis(lblText);
        lblText << m_qwtSelectedSampleDelta->m_yPoint << ") : (";
        setDisplayIoMapipXAxis(lblText, curve);
        lblText << m_qwtSelectedSample->m_xPoint << ",";
        setDisplayIoMapipYAxis(lblText);
        lblText << m_qwtSelectedSample->m_yPoint << ") d (";
        setDisplayIoMapipXAxis(lblText, curve);
        lblText << (m_qwtSelectedSample->m_xPoint-m_qwtSelectedSampleDelta->m_xPoint) << ",";
        setDisplayIoMapipYAxis(lblText);
        lblText << (m_qwtSelectedSample->m_yPoint-m_qwtSelectedSampleDelta->m_yPoint) << ")";

        m_qwtCurves[m_selectedCurveIndex]->pointLabel->setText(lblText.str().c_str());
        QPalette palette = this->palette();
        palette.setColor( QPalette::WindowText, m_qwtCurves[m_selectedCurveIndex]->getColor());
        palette.setColor( QPalette::Text, m_qwtCurves[m_selectedCurveIndex]->getColor());
        m_qwtCurves[m_selectedCurveIndex]->pointLabel->setPalette(palette);

        ui->InfoLayout->addWidget(m_qwtCurves[m_selectedCurveIndex]->pointLabel);

        m_qwtCurves[m_selectedCurveIndex]->pointLabel->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_qwtCurves[m_selectedCurveIndex]->pointLabel, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(ShowRightClickForDisplayPoints(const QPoint&)));
    }

}

void MainWindow::updatePointDisplay()
{
    clearPointLabels();
    if(m_qwtSelectedSampleDelta->isAttached)
    {
        if(m_qwtSelectedSample->isAttached)
        {
            displayDeltaLabel();
        }
    }
    else if(m_qwtSelectedSample->isAttached)
    {
        displayPointLabels();
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
        replotMainPlot();
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

                if(wheelEvent->delta() > 0)
                {
                    m_plotZoom->Zoom(ZOOM_IN_PERCENT, relMousePos);
                }
                else if(wheelEvent->delta() < 0)
                {
                    m_plotZoom->Zoom(ZOOM_OUT_PERCENT, relMousePos);
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
    int posMod = 0;
    QScrollBar* scroll = NULL;
    bool valid = false;

    switch(key)
    {
        case Qt::Key_Down:
            scroll = ui->verticalScrollBar;
            posMod = 1;
        break;
        case Qt::Key_Up:
            scroll = ui->verticalScrollBar;
            posMod = -1;
        break;
        case Qt::Key_Left:
            scroll = ui->horizontalScrollBar;
            posMod = -1;
        break;
        case Qt::Key_Right:
            scroll = ui->horizontalScrollBar;
            posMod = 1;
        break;
    }

    if(scroll != NULL)
    {
        valid = true;
        m_plotZoom->ModSliderPos(scroll, posMod);
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
        m_plotZoom->ModSliderPos(ui->verticalScrollBar, ZOOM_SCROLL_CHANGE_SMALL);
        break;
    case QAbstractSlider::SliderSingleStepSub:
        m_plotZoom->ModSliderPos(ui->verticalScrollBar, -ZOOM_SCROLL_CHANGE_SMALL);
        break;
    case QAbstractSlider::SliderPageStepAdd:
        m_plotZoom->ModSliderPos(ui->verticalScrollBar, ZOOM_SCROLL_CHANGE_BIG);
        break;
    case QAbstractSlider::SliderPageStepSub:
        m_plotZoom->ModSliderPos(ui->verticalScrollBar, -ZOOM_SCROLL_CHANGE_BIG);
        break;
    }
}

void MainWindow::on_horizontalScrollBar_actionTriggered(int action)
{
    switch(action)
    {
    case QAbstractSlider::SliderSingleStepAdd:
        m_plotZoom->ModSliderPos(ui->horizontalScrollBar, ZOOM_SCROLL_CHANGE_SMALL);
        break;
    case QAbstractSlider::SliderSingleStepSub:
        m_plotZoom->ModSliderPos(ui->horizontalScrollBar, -ZOOM_SCROLL_CHANGE_SMALL);
        break;
    case QAbstractSlider::SliderPageStepAdd:
        m_plotZoom->ModSliderPos(ui->horizontalScrollBar, ZOOM_SCROLL_CHANGE_BIG);
        break;
    case QAbstractSlider::SliderPageStepSub:
        m_plotZoom->ModSliderPos(ui->horizontalScrollBar, -ZOOM_SCROLL_CHANGE_BIG);
        break;
    }
}

void MainWindow::setSelectedCurveIndex(int index)
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    if(index >= 0 && index < m_qwtCurves.size())
    {
        int oldSelectCursor = m_selectedCurveIndex;
        m_qwtSelectedSample->setCurve(m_qwtCurves[index]);
        m_qwtSelectedSampleDelta->setCurve(m_qwtCurves[index]);
        m_selectedCurveIndex = index;
        if(m_normalizeCurves)
        {
            if(oldSelectCursor >= 0 && oldSelectCursor < m_qwtCurves.size())
            {
                maxMinXY newZoom;
                maxMinXY oldMaxMinXY = m_plotZoom->getCurZoom();
                tLinearXYAxis newCursorScale = m_qwtCurves[m_selectedCurveIndex]->getNormFactor();
                newZoom.maxX = (oldMaxMinXY.maxX - newCursorScale.xAxis.b) / newCursorScale.xAxis.m;
                newZoom.minX = (oldMaxMinXY.minX - newCursorScale.xAxis.b) / newCursorScale.xAxis.m;
                newZoom.maxY = (oldMaxMinXY.maxY - newCursorScale.yAxis.b) / newCursorScale.yAxis.m;
                newZoom.minY = (oldMaxMinXY.minY - newCursorScale.yAxis.b) / newCursorScale.yAxis.m;

                m_plotZoom->SetPlotDimensions(m_qwtCurves[m_selectedCurveIndex]->getMaxMinXYOfData());
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

void MainWindow::replotMainPlot()
{
    QMutexLocker lock(&m_qwtCurvesMutex);

    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_normalizeCurves)
        {
            m_qwtCurves[i]->setNormalizeFactor(m_qwtCurves[m_selectedCurveIndex]->getMaxMinXYOfData());
        }
        else
        {
            m_qwtCurves[i]->resetNormalizeFactor();
        }
        m_qwtCurves[i]->setCurveSamples();
    }

    if(m_normalizeCurves)
    {
        m_plotZoom->SetPlotDimensions(m_qwtCurves[m_selectedCurveIndex]->getMaxMinXYOfData());
    }
    else
    {
        m_plotZoom->SetPlotDimensions(m_maxMin);
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

    // Kinda ugly, but this is the simplist way I found to display the right click menu
    // when right clicking in the display area. Basically, for loop through all the curves
    // until a valid point label is found. Then use that point label for the right click menu and exit.
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i]->pointLabel != NULL)
        {
            m_displayPointsMenu.exec(m_qwtCurves[i]->pointLabel->mapToGlobal(pos));
            break;
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
    m_curveCommander->plotWindowClose(this->windowTitle());
}

void MainWindow::resizeEvent(QResizeEvent* /*event*/)
{
    QSize cavasSize = m_qwtPlot->canvas()->frameSize();
    m_canvasXOverYRatio = (cavasSize.width() - (2.0*PLOT_CANVAS_OFFSET)) / (cavasSize.height() - (2.0*PLOT_CANVAS_OFFSET));
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

   updatePointDisplay();
   replotMainPlot();
   calcMaxMin();
   emit updateCursorMenusSignal();
}

void MainWindow::removeCurve(const QString& curveName)
{
   QMutexLocker lock(&m_qwtCurvesMutex);

   int curveIndex = getCurveIndex(curveName);
   if(curveIndex >= 0)
   {
      delete m_qwtCurves[curveIndex];
      m_qwtCurves.removeAt(curveIndex);
   }
   updateCurveOrder();
}

