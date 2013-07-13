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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <QSignalMapper>
#include <QKeyEvent>
#include <sstream>


QColor curveColors[] =
{
    QColor(0,255,255),
    QColor(255,0,255),
    QColor(255,255,0),
    QColor(255,128,128),
    QColor(128,255,128),
    QColor(128,128,255)
};



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_qwtPlot(NULL),
    m_qwtSelectedSample(NULL),
    m_qwtSelectedSampleDelta(NULL),
    m_qwtPicker(NULL),
    m_qwtGrid(NULL),
    m_selectMode(E_CURSOR),
    m_selectedCurveIndex(0),
    m_plotZoom(NULL),
    m_checkedIcon("check.png"),
    m_normalizeCurves(false),
    m_legendDisplayed(false),
    m_zoomAction("Zoom", this),
    m_cursorAction("Cursor", this),
    m_deltaCursorAction("Delta Cursor", this),
    m_resetZoomAction("Reset Zoom", this),
    m_normalizeAction("Normalize Curves", this),
    m_toggleLegendAction("Legend", this),
    m_selectedCurvesMenu("Selected Curve"),
    m_visibleCurvesMenu("Visible Curves")
{
    ui->setupUi(this);

    srand((unsigned)time(0));

    QPalette palette = this->palette();
    palette.setColor( QPalette::WindowText, Qt::white);
    palette.setColor( QPalette::Text, Qt::white);
    this->setPalette(palette);

    connect(this, SIGNAL(updateCursorMenusSignal()),
            this, SLOT(updateCursorMenus()), Qt::QueuedConnection);

    // Connect menu commands
    connect(&m_zoomAction, SIGNAL(triggered(bool)), this, SLOT(zoomMode()));
    connect(&m_cursorAction, SIGNAL(triggered(bool)), this, SLOT(cursorMode()));
    connect(&m_resetZoomAction, SIGNAL(triggered(bool)), this, SLOT(resetZoom()));
    connect(&m_deltaCursorAction, SIGNAL(triggered(bool)), this, SLOT(deltaCursorMode()));
    connect(&m_normalizeAction, SIGNAL(triggered(bool)), this, SLOT(normalizeCurves()));
    connect(&m_toggleLegendAction, SIGNAL(triggered(bool)), this, SLOT(toggleLegend()));

    m_cursorAction.setIcon(m_checkedIcon);

    resetPlot();

    ui->verticalScrollBar->setRange(0,0);
    ui->horizontalScrollBar->setRange(0,0);

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
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i] != NULL)
        {
            delete m_qwtCurves[i];
            m_qwtCurves[i] = NULL;
        }
    }
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
        this, SLOT(ShowContextMenu(const QPoint&)));

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

        if(m_qwtCurves[i]->displayed)
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
    if(m_qwtCurves[m_selectedCurveIndex]->displayed == false && numDisplayedCurves > 0)
    {
        int newSelectedCurveIndex = 0;
        for(int i = 1; i < m_qwtCurves.size(); ++i)
        {
            newSelectedCurveIndex = (m_selectedCurveIndex - i);
            if(newSelectedCurveIndex < 0)
            {
                newSelectedCurveIndex += m_qwtCurves.size();
            }
            if(m_qwtCurves[newSelectedCurveIndex]->displayed)
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
        if(m_qwtCurves[i]->displayed)
        {
            m_qwtCurves[i]->curveAction = new QAction(m_checkedIcon, m_qwtCurves[i]->GetCurveTitle(), this);
        }
        else
        {
            m_qwtCurves[i]->curveAction = new QAction(m_qwtCurves[i]->GetCurveTitle(), this);
        }
        m_qwtCurves[i]->mapper = new QSignalMapper(this);

        m_qwtCurves[i]->mapper->setMapping(m_qwtCurves[i]->curveAction, i);
        connect(m_qwtCurves[i]->curveAction,SIGNAL(triggered()),
                         m_qwtCurves[i]->mapper,SLOT(map()));

        m_visibleCurvesMenu.addAction(m_qwtCurves[i]->curveAction);
        connect( m_qwtCurves[i]->mapper, SIGNAL(mapped(int)), SLOT(visibleCursorMenuSelect(int)) );

        if(m_qwtCurves[i]->displayed)
        {
            tMenuActionMapper actionMapper;
            if(i == m_selectedCurveIndex)
            {
                actionMapper.action = new QAction(m_checkedIcon, m_qwtCurves[i]->GetCurveTitle(), this);
            }
            else
            {
                actionMapper.action = new QAction(m_qwtCurves[i]->GetCurveTitle(), this);
            }
            actionMapper.mapper = new QSignalMapper(this);

            actionMapper.mapper->setMapping(actionMapper.action, i);
            connect(actionMapper.action,SIGNAL(triggered()),
                             actionMapper.mapper,SLOT(map()));

            m_selectedCurvesMenu.addAction(actionMapper.action);
            connect( actionMapper.mapper, SIGNAL(mapped(int)), SLOT(selectedCursorMenuSelect(int)) );

            m_selectedCursorActions.push_back(actionMapper);
        }

    }

}

void MainWindow::add1dCurve(QString name, dubVect &yPoints)
{
    addCurve(name, NULL, &yPoints);
}


void MainWindow::add2dCurve(QString name, dubVect &xPoints, dubVect &yPoints)
{
    addCurve(name, &xPoints, &yPoints);
}

void MainWindow::addCurve(QString& name, dubVect* xPoints, dubVect* yPoints)
{
    int curveIndex = findMatchingCurve(name);
    if(curveIndex >= 0)
    {
        if(xPoints == NULL)
        {
            m_qwtCurves[curveIndex]->SetNewCurveSamples(*yPoints);
        }
        else
        {
            m_qwtCurves[curveIndex]->SetNewCurveSamples(*xPoints, *yPoints);
        }
    }
    else
    {
        curveIndex = m_qwtCurves.size();
        int colorLookupIndex = curveIndex % ARRAY_SIZE(curveColors);

        if(xPoints == NULL)
        {
            m_qwtCurves.push_back(new CurveData(name, *yPoints, curveColors[colorLookupIndex]));
        }
        else
        {
            m_qwtCurves.push_back(new CurveData(name, *xPoints, *yPoints, curveColors[colorLookupIndex]));
        }

        m_qwtCurves[curveIndex]->displayed = true;
        m_qwtCurves[curveIndex]->curve->attach(m_qwtPlot);
    }

    calcMaxMin();

    if(m_qwtSelectedSample->getCurve() == NULL)
    {
        setSelectedCurveIndex(curveIndex);
    }
    m_plotZoom->ResetZoom();
    replotMainPlot();
    emit updateCursorMenusSignal();
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
    updatePointDisplay();
    replotMainPlot();
}


void MainWindow::deltaCursorMode()
{
    m_selectMode = E_DELTA_CURSOR;
    m_deltaCursorAction.setIcon(m_checkedIcon);
    m_zoomAction.setIcon(QIcon());
    m_cursorAction.setIcon(QIcon());

    m_qwtSelectedSampleDelta->setCurve(m_qwtSelectedSample->getCurve());
    m_qwtSelectedSampleDelta->showCursor(
        QPointF(m_qwtSelectedSample->m_xPoint,m_qwtSelectedSample->m_yPoint));

    m_qwtPicker->setRubberBand( QwtPicker::CrossRubberBand );
    m_qwtSelectedSample->hideCursor();
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
    }
    replotMainPlot();
    resetZoom();
}


void MainWindow::visibleCursorMenuSelect(int index)
{
    m_qwtCurves[index]->displayed = !m_qwtCurves[index]->displayed;
    if(!m_qwtCurves[index]->displayed)
    {
        m_qwtCurves[index]->curve->detach();
    }
    else
    {
        m_qwtCurves[index]->curve->attach(m_qwtPlot);
    }
    updatePointDisplay();
    replotMainPlot();
    calcMaxMin();
    emit updateCursorMenusSignal();
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

void MainWindow::calcMaxMin()
{
    int i = 0;
    while(i < m_qwtCurves.size() && m_qwtCurves[i]->displayed == false)
    {
        ++i;
    }
    if(i < m_qwtCurves.size())
    {
        m_maxMin = m_qwtCurves[i]->GetMaxMinXYOfCurve();
        while(i < m_qwtCurves.size())
        {
            if(m_qwtCurves[i]->displayed)
            {
                maxMinXY curveMaxMinXY = m_qwtCurves[i]->GetMaxMinXYOfCurve();
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
        m_qwtSelectedSample->showCursor(pos);

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

void MainWindow::on_verticalScrollBar_sliderMoved(int position)
{
    m_plotZoom->VertSliderMoved();
}

void MainWindow::on_horizontalScrollBar_sliderMoved(int position)
{
    m_plotZoom->HorzSliderMoved();
}

void MainWindow::clearPointLabels()
{
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
    clearPointLabels();
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i]->displayed && m_qwtSelectedSample->m_pointIndex < m_qwtCurves[i]->numPoints)
        {
            if(m_qwtCurves[i]->pointLabel != NULL)
            {
                delete m_qwtCurves[i]->pointLabel;
                m_qwtCurves[i]->pointLabel = NULL;
            }
            m_qwtCurves[i]->pointLabel = new QLabel("");

            std::stringstream lblText;
            lblText << "(" <<
                m_qwtCurves[i]->getXPoints()[m_qwtSelectedSample->m_pointIndex] << "," <<
                m_qwtCurves[i]->getYPoints()[m_qwtSelectedSample->m_pointIndex] << ")";
            m_qwtCurves[i]->pointLabel->setText(lblText.str().c_str());

            QPalette palette = this->palette();
            palette.setColor( QPalette::WindowText, m_qwtCurves[i]->color);
            palette.setColor( QPalette::Text, m_qwtCurves[i]->color);
            m_qwtCurves[i]->pointLabel->setPalette(palette);

            ui->InfoLayout->addWidget(m_qwtCurves[i]->pointLabel);
        }
    }
}
void MainWindow::displayDeltaLabel()
{
    clearPointLabels();
    if(m_qwtCurves[m_selectedCurveIndex]->displayed)
    {
        if(m_qwtCurves[m_selectedCurveIndex]->pointLabel != NULL)
        {
            delete m_qwtCurves[m_selectedCurveIndex]->pointLabel;
            m_qwtCurves[m_selectedCurveIndex]->pointLabel = NULL;
        }
        m_qwtCurves[m_selectedCurveIndex]->pointLabel = new QLabel("");

        std::stringstream lblText;
        lblText << "(" <<
            m_qwtSelectedSampleDelta->m_xPoint << "," <<
            m_qwtSelectedSampleDelta->m_yPoint << ") : (" <<
            m_qwtSelectedSample->m_xPoint << "," <<
            m_qwtSelectedSample->m_yPoint << ") d (" <<
            (m_qwtSelectedSample->m_xPoint-m_qwtSelectedSampleDelta->m_xPoint) << "," <<
            (m_qwtSelectedSample->m_yPoint-m_qwtSelectedSampleDelta->m_yPoint) << ")";

        m_qwtCurves[m_selectedCurveIndex]->pointLabel->setText(lblText.str().c_str());
        QPalette palette = this->palette();
        palette.setColor( QPalette::WindowText, m_qwtCurves[m_selectedCurveIndex]->color);
        palette.setColor( QPalette::Text, m_qwtCurves[m_selectedCurveIndex]->color);
        m_qwtCurves[m_selectedCurveIndex]->pointLabel->setPalette(palette);

        ui->InfoLayout->addWidget(m_qwtCurves[m_selectedCurveIndex]->pointLabel);
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

                const int OFFSET = 6;

                // Map position relative to the curve's canvas.
                // Y axis needs to be inverted so the 0 point is at the bottom.
                QPointF relMousePos(
                    (double)(mousePos.x() - OFFSET) / (double)(cavasSize.width() - 2*OFFSET),
                    1.0 - (double)(mousePos.y() - OFFSET) / (double)(cavasSize.height() - 2*OFFSET));

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
    if(modDelta != 0)
    {
        std::vector<int> displayedCurves;
        int indexOfSelectedCursor = -1;

        unsigned int modDeltaAbs = (unsigned int)abs(modDelta);

        for(int i = 0; i < m_qwtCurves.size(); ++i)
        {
            if(m_qwtCurves[i]->displayed)
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
                newXPos += m_qwtSelectedSample->getCurve()->numPoints;
            }
            while((unsigned int)newXPos >= m_qwtSelectedSample->getCurve()->numPoints)
            {
                newXPos -= m_qwtSelectedSample->getCurve()->numPoints;
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
    if(index >= 0 && index < m_qwtCurves.size())
    {
        m_qwtSelectedSample->setCurve(m_qwtCurves[index]);
        m_qwtSelectedSampleDelta->setCurve(m_qwtCurves[index]);
        m_selectedCurveIndex = index;
        if(m_normalizeCurves)
        {
            replotMainPlot();
        }
    }
}

void MainWindow::replotMainPlot()
{
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_normalizeCurves)
        {
            m_qwtCurves[i]->setNormalizeFactor(m_qwtCurves[m_selectedCurveIndex]->GetMaxMinXYOfData());
        }
        else
        {
            m_qwtCurves[i]->resetNormalizeFactor();
        }
        m_qwtCurves[i]->setCurveSamples();
    }
    m_qwtPlot->replot();
}

void MainWindow::ShowContextMenu(const QPoint& pos) // this is a slot
{
    m_rightClickMenu.exec(m_qwtPlot->mapToGlobal(pos));
}

void MainWindow::onApplicationFocusChanged(QWidget* old, QWidget* now)
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

int MainWindow::findMatchingCurve(const QString& curveTitle)
{
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        if(m_qwtCurves[i]->GetCurveTitle() == curveTitle)
        {
            return i;
        }
    }
    return -1;
}


