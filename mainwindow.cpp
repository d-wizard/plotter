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

QwtPlotGrid *grid;
int mi_size;
int mi_index;
dubVect md_x;
dubVect md_y;
dubVect md_z;

typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
}tRGB;

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

tRGB curveColors[] =
{
    {0,255,255},
    {255,0,255},
    {255,255,0},
    {255,128,128},
    {128,255,128},
    {128,128,255}
};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_qwtPlot(NULL),
    m_qwtSelectedSample(NULL),
    m_qwtSelectedSampleDelta(NULL),
    m_qwtPicker(NULL),
    m_selectMode(E_CURSOR),
    m_qwtGrid(NULL),
    m_plotZoom(NULL),
    m_selectedCurveIndex(0)
{
    ui->setupUi(this);
    mi_size = 100000;
    mi_index = 0;

    QPalette palette = this->palette();
    palette.setColor( QPalette::WindowText, Qt::white);
    palette.setColor( QPalette::Text, Qt::white);
    this->setPalette(palette);


    connect(this, SIGNAL(updateCursorMenusSignal()),
            this, SLOT(updateCursorMenus()), Qt::QueuedConnection);

    // init test samples.
    md_x.resize(mi_size);
    md_y.resize(mi_size);
    md_z.resize(mi_size);


    for(int i_index = 0; i_index < mi_size; ++i_index)
    {
       md_x[i_index] = cos((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;//i_index;
       md_y[i_index] = sin((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
       md_z[i_index] = sin((3.14159 * 4.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
    }


    // Connect menu commands
    connect(ui->actionZoom, SIGNAL(triggered(bool)), this, SLOT(zoomMode()));
    connect(ui->actionSelect_Point, SIGNAL(triggered(bool)), this, SLOT(cursorMode()));
    connect(ui->actionReset_Zoom, SIGNAL(triggered(bool)), this, SLOT(resetZoom()));
    connect(ui->actionDelta_Cursor, SIGNAL(triggered(bool)), this, SLOT(deltaCursorMode()));

    ui->actionSelect_Point->setIcon(QIcon("CheckBox.png"));

    resetPlot();
    add1dCurve("Curve1", md_y);
    add1dCurve("Curve2", md_x);
    add1dCurve("Curve3", md_z);


    m_tcpMsgReader = new TCPMsgReader(this, 2000);

    ui->verticalScrollBar->setRange(0,0);
    ui->horizontalScrollBar->setRange(0,0);

    // For multi-window will probably need to do this whenever the window get focus.
    QCoreApplication::instance()->installEventFilter(this);

}

MainWindow::~MainWindow()
{
    delete m_tcpMsgReader;
    delete ui;

    if(m_qwtPlot != NULL)
    {
        delete m_qwtPlot;
        m_qwtPlot = NULL;
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
    //if(m_qwtGrid != NULL)
    //{
    //    delete m_qwtGrid;
    //}
    //if(m_qwtPicker != NULL)
    //{
    //    delete m_qwtPicker;
    //}


 }

void MainWindow::resetPlot()
{
    if(m_qwtPlot != NULL)
    {
        ui->GraphLayout->removeWidget(m_qwtPlot);
        delete m_qwtPlot;
    }
    m_qwtPlot = new QwtPlot(this);

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
    m_qwtPicker->setStateMachine(new QwtPickerDragRectMachine());
    //m_qwtPicker->setRubberBandPen( QColor( Qt::green ) );
    //m_qwtPicker->setRubberBand( QwtPicker::CrossRubberBand );

    //m_qwtPicker = new QwtPlotPicker( QwtPlot::xBottom, QwtPlot::yLeft,
    //    QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn,
    //    m_qwtPlot->canvas() );
    //m_qwtPicker->setStateMachine( new QwtPickerDragPointMachine() );
    m_qwtPicker->setRubberBandPen( QColor( Qt::green ) );
    m_qwtPicker->setRubberBand( QwtPicker::CrossRubberBand );
    m_qwtPicker->setTrackerPen( QColor( Qt::white ) );

    m_qwtPlot->show();
    m_qwtGrid->show();

    //m_qwtMagnifier = new QwtPlotMagnifier(m_qwtPlot->canvas());
    //m_qwtMagnifier->setMouseButton(Qt::MidButton);

    connect(m_qwtPicker, SIGNAL(appended(QPointF)),
            this, SLOT(pointSelected(QPointF)));
    connect(m_qwtPicker, SIGNAL(selected(QRectF)),
            this, SLOT(rectSelected(QRectF)));
}

void MainWindow::updateCursorMenus()
{
    QIcon checkIcon("CheckBox.png");
    int numDisplayedCurves = 0;


    // TODO: make this thread safe
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        ui->menuCurves->removeAction(m_qwtCurves[i]->curveAction);

        // TODO: Do I need to disconnect the action / mapper?
        if(m_qwtCurves[i]->curveAction != NULL)
        {
            delete m_qwtCurves[i]->curveAction;
        }
        if(m_qwtCurves[i]->mapper != NULL)
        {
            delete m_qwtCurves[i]->mapper;
        }

        if(m_qwtCurves[i]->displayed)
        {
            ++numDisplayedCurves;
        }
    }

    for(int i = 0; i < m_selectedCursorActions.size(); ++i)
    {
        ui->menuSelected->removeAction(m_selectedCursorActions[i].action);
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
            m_qwtCurves[i]->curveAction = new QAction(checkIcon, m_qwtCurves[i]->title.c_str(), this);
        }
        else
        {
            m_qwtCurves[i]->curveAction = new QAction(m_qwtCurves[i]->title.c_str(), this);
        }
        m_qwtCurves[i]->mapper = new QSignalMapper(this);

        m_qwtCurves[i]->mapper->setMapping(m_qwtCurves[i]->curveAction, i);
        connect(m_qwtCurves[i]->curveAction,SIGNAL(triggered()),
                         m_qwtCurves[i]->mapper,SLOT(map()));

        ui->menuCurves->addAction(m_qwtCurves[i]->curveAction);
        connect( m_qwtCurves[i]->mapper, SIGNAL(mapped(int)), SLOT(visibleCursorMenuSelect(int)) );

        if(m_qwtCurves[i]->displayed)
        {
            tMenuActionMapper actionMapper;
            if(i == m_selectedCurveIndex)
            {
                actionMapper.action = new QAction(checkIcon, m_qwtCurves[i]->title.c_str(), this);
            }
            else
            {
                actionMapper.action = new QAction(m_qwtCurves[i]->title.c_str(), this);
            }
            actionMapper.mapper = new QSignalMapper(this);

            actionMapper.mapper->setMapping(actionMapper.action, i);
            connect(actionMapper.action,SIGNAL(triggered()),
                             actionMapper.mapper,SLOT(map()));

            ui->menuSelected->addAction(actionMapper.action);
            connect( actionMapper.mapper, SIGNAL(mapped(int)), SLOT(selectedCursorMenuSelect(int)) );

            m_selectedCursorActions.push_back(actionMapper);
        }

    }

}

void MainWindow::add1dCurve(std::string name, dubVect yPoints)
{
    m_qwtCurves.push_back(new CurveData(name, yPoints));

    QwtPlotCurve* curve = m_qwtCurves[m_qwtCurves.size()-1]->getCurve();

    int vectSize = yPoints.size();
    int curveIndex = m_qwtCurves.size()-1;
    m_qwtCurves[curveIndex]->xPoints.resize(vectSize);
    m_qwtCurves[curveIndex]->yPoints = yPoints;

    for(int i = 0; i < vectSize; ++i)
    {
        m_qwtCurves[curveIndex]->xPoints[i] = (double)i;
    }

    m_qwtCurves[curveIndex]->maxMin.minX = 0;
    m_qwtCurves[curveIndex]->maxMin.maxX = vectSize-1;
    m_qwtCurves[curveIndex]->maxMin.minY = m_qwtCurves[curveIndex]->yPoints[0];
    m_qwtCurves[curveIndex]->maxMin.maxY = m_qwtCurves[curveIndex]->yPoints[0];

    for(int i = 1; i < vectSize; ++i)
    {
        if(m_qwtCurves[curveIndex]->maxMin.minY > m_qwtCurves[curveIndex]->yPoints[i])
        {
            m_qwtCurves[curveIndex]->maxMin.minY = m_qwtCurves[curveIndex]->yPoints[i];
        }
        if(m_qwtCurves[curveIndex]->maxMin.maxY < m_qwtCurves[curveIndex]->yPoints[i])
        {
            m_qwtCurves[curveIndex]->maxMin.maxY = m_qwtCurves[curveIndex]->yPoints[i];
        }
    }

    m_qwtCurves[curveIndex]->displayed = true;

    int colorLookupIndex = curveIndex % ARRAY_SIZE(curveColors);
    m_qwtCurves[curveIndex]->color = QColor(
                curveColors[colorLookupIndex].red,
                curveColors[colorLookupIndex].green,
                curveColors[colorLookupIndex].blue);

    curve->setPen(m_qwtCurves[curveIndex]->color);

    curve->setSamples( &m_qwtCurves[curveIndex]->xPoints[0],
                       &m_qwtCurves[curveIndex]->yPoints[0],
                       vectSize);

    curve->attach(m_qwtPlot);
    calcMaxMin();

    if(m_qwtSelectedSample->getCurve() == NULL)
    {
        setSelectedCurveIndex(curveIndex);
    }

    m_qwtPlot->replot();
    emit updateCursorMenusSignal();
}


void MainWindow::add2dCurve(std::string name, dubVect xPoints, dubVect yPoints)
{

}


void MainWindow::toggleLegend()
{

}


void MainWindow::cursorMode()
{
    m_selectMode = E_CURSOR;
    ui->actionSelect_Point->setIcon(QIcon("CheckBox.png"));
    ui->actionZoom->setIcon(QIcon());
    ui->actionDelta_Cursor->setIcon(QIcon());
    m_qwtPicker->setStateMachine( new QwtPickerDragRectMachine() );
    m_qwtSelectedSampleDelta->hideCursor();
    m_qwtPlot->replot();
}


void MainWindow::deltaCursorMode()
{
    m_selectMode = E_DELTA_CURSOR;
    ui->actionDelta_Cursor->setIcon(QIcon("CheckBox.png"));
    ui->actionZoom->setIcon(QIcon());
    ui->actionSelect_Point->setIcon(QIcon());

    m_qwtSelectedSampleDelta->setCurve(m_qwtSelectedSample->getCurve());
    m_qwtSelectedSampleDelta->showCursor(
        QPointF(m_qwtSelectedSample->m_xPoint,m_qwtSelectedSample->m_yPoint));

    m_qwtSelectedSample->hideCursor();
    updatePointDisplay();
    m_qwtPlot->replot();
}


void MainWindow::zoomMode()
{
    m_selectMode = E_ZOOM;
    ui->actionZoom->setIcon(QIcon("CheckBox.png"));
    ui->actionSelect_Point->setIcon(QIcon());
    ui->actionDelta_Cursor->setIcon(QIcon());
    //m_qwtPicker->setStateMachine( new QwtPickerDragPointMachine() );
}


void MainWindow::resetZoom()
{
    calcMaxMin();
    m_plotZoom->SetZoom(maxMin);
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
    m_qwtPlot->replot();
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
        maxMin = m_qwtCurves[i]->maxMin;
        while(i < m_qwtCurves.size())
        {
            if(m_qwtCurves[i]->displayed)
            {
                if(maxMin.minX > m_qwtCurves[i]->maxMin.minX)
                {
                    maxMin.minX = m_qwtCurves[i]->maxMin.minX;
                }
                if(maxMin.minY > m_qwtCurves[i]->maxMin.minY)
                {
                    maxMin.minY = m_qwtCurves[i]->maxMin.minY;
                }
                if(maxMin.maxX < m_qwtCurves[i]->maxMin.maxX)
                {
                    maxMin.maxX = m_qwtCurves[i]->maxMin.maxX;
                }
                if(maxMin.maxY < m_qwtCurves[i]->maxMin.maxY)
                {
                    maxMin.maxY = m_qwtCurves[i]->maxMin.maxY;
                }
            }
            ++i;
        }
    }
    else
    {
        // TODO: Should do something when there are no curves displayed
    }

    m_plotZoom->SetPlotDimensions(maxMin);

}




void MainWindow::pointSelected(const QPointF &pos)
{
    if(m_selectMode == E_CURSOR || m_selectMode == E_DELTA_CURSOR)
    {
        m_qwtSelectedSample->showCursor(pos);

        updatePointDisplay();
        m_qwtPlot->replot();

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
        if(m_qwtCurves[i]->displayed)
        {
            m_qwtCurves[i]->pointLabel = new QLabel("");

            char tempText[100];
            snprintf(tempText, sizeof(tempText), "(%f,%f)",
                     (float)m_qwtCurves[i]->xPoints[m_qwtSelectedSample->m_pointIndex],
                     (float)m_qwtCurves[i]->yPoints[m_qwtSelectedSample->m_pointIndex]);
            m_qwtCurves[i]->pointLabel->setText(tempText);

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
        m_qwtCurves[m_selectedCurveIndex]->pointLabel = new QLabel("");

        char tempText[100];
        snprintf(tempText, sizeof(tempText), "(%f,%f) : (%f,%f) d (%f,%f)",
                 (float)m_qwtSelectedSampleDelta->m_xPoint,
                 (float)m_qwtSelectedSampleDelta->m_yPoint,
                 (float)m_qwtSelectedSample->m_xPoint,
                 (float)m_qwtSelectedSample->m_yPoint,
                 (float)(m_qwtSelectedSample->m_xPoint-m_qwtSelectedSampleDelta->m_xPoint),
                 (float)(m_qwtSelectedSample->m_yPoint-m_qwtSelectedSampleDelta->m_yPoint));

        m_qwtCurves[m_selectedCurveIndex]->pointLabel->setText(tempText);
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
        m_qwtPlot->replot();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        bool validKey = true;
        QKeyEvent *KeyEvent = (QKeyEvent*)event;

        switch(m_selectMode)
        {
        case E_ZOOM:
            validKey = keyPressModifyZoom(KeyEvent->key());
            break;
        case E_CURSOR:
        case E_DELTA_CURSOR:
            validKey = keyPressModifyCursor(KeyEvent->key());
            break;
        default:
            validKey = false;
            break;
        }


        if(validKey)
        {
            return true;
        }
        else
        {
            // standard event processing
            return QObject::eventFilter(obj, event);
        }
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

        int modDeltaAbs = (int)abs(modDelta);

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
            else if(newIndexOfSelectedCursor >= displayedCurves.size())
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

            if(newXPos >= 0 && newXPos <= m_qwtSelectedSample->getCurve()->xPoints.size())
            {
                m_qwtSelectedSample->m_pointIndex = newXPos;
                updateCursors();
            }

        }
    }
}


void MainWindow::on_verticalScrollBar_actionTriggered(int action)
{
    switch(action)
    {
    case QAbstractSlider::SliderSingleStepAdd:
        m_plotZoom->ModSliderPos(ui->verticalScrollBar, 1);
        break;
    case QAbstractSlider::SliderSingleStepSub:
        m_plotZoom->ModSliderPos(ui->verticalScrollBar, -1);
        break;
    case QAbstractSlider::SliderPageStepAdd:
        m_plotZoom->ModSliderPos(ui->verticalScrollBar, 10);
        break;
    case QAbstractSlider::SliderPageStepSub:
        m_plotZoom->ModSliderPos(ui->verticalScrollBar, -10);
        break;
    }
}

void MainWindow::on_horizontalScrollBar_actionTriggered(int action)
{
    switch(action)
    {
    case QAbstractSlider::SliderSingleStepAdd:
        m_plotZoom->ModSliderPos(ui->horizontalScrollBar, 1);
        break;
    case QAbstractSlider::SliderSingleStepSub:
        m_plotZoom->ModSliderPos(ui->horizontalScrollBar, -1);
        break;
    case QAbstractSlider::SliderPageStepAdd:
        m_plotZoom->ModSliderPos(ui->horizontalScrollBar, 10);
        break;
    case QAbstractSlider::SliderPageStepSub:
        m_plotZoom->ModSliderPos(ui->horizontalScrollBar, -10);
        break;
    }
}
CurveData* blah;
void MainWindow::setSelectedCurveIndex(int index)
{
    if(index >= 0 && index < m_qwtCurves.size())
    {
        blah = m_qwtCurves[index];




        m_qwtSelectedSample->setCurve(m_qwtCurves[index]);
        m_qwtSelectedSampleDelta->setCurve(m_qwtCurves[index]);
        m_selectedCurveIndex = index;
    }
}
