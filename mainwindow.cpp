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
    //m_qwtSelectedSample(NULL),
    m_qwtSelectedSampleDelta(NULL),
    m_qwtPicker(NULL),
    m_selectMode(E_CURSOR),
    m_qwtGrid(NULL),
    selectedCurveIndex(0)
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




    // Init the cursor curves
    //m_qwtSelectedSample = new QwtPlotCurve("");
    m_qwtSelectedSampleDelta = new QwtPlotCurve("");


    // Connect menu commands
    connect(ui->actionZoom, SIGNAL(triggered(bool)), this, SLOT(zoomMode()));
    connect(ui->actionSelect_Point, SIGNAL(triggered(bool)), this, SLOT(cursorMode()));
    connect(ui->actionReset_Zoom, SIGNAL(triggered(bool)), this, SLOT(resetZoom()));

    resetPlot();
    add1dCurve("Curve1", md_y);
    add1dCurve("Curve2", md_x);
    add1dCurve("Curve3", md_z);


    m_tcpMsgReader = new TCPMsgReader(this, 2000);

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
    //if(m_qwtGrid != NULL)
    //{
    //    delete m_qwtGrid;
    //}
    //if(m_qwtPicker != NULL)
    //{
    //    delete m_qwtPicker;
    //}
    //if(m_qwtSelectedSample != NULL)
    //{
    //    delete m_qwtSelectedSample;
    //}
    if(m_qwtSelectedSampleDelta != NULL)
    {
        delete m_qwtSelectedSampleDelta;
    }


    //delete mt_qwfPlotCurve;
    //delete mt_qwtPlot;
    //delete [] md_x;
    //delete [] md_y;

 }

void MainWindow::resetPlot()
{
    if(m_qwtPlot != NULL)
    {
        ui->GraphLayout->removeWidget(m_qwtPlot);
        delete m_qwtPlot;
    }
    m_qwtPlot = new QwtPlot(this);

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
    // TODO: make this thread safe
    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        ui->menuCurves->removeAction(m_qwtCurves[i].curveAction);

        // TODO: Do I need to disconnect the action / mapper?
        if(m_qwtCurves[i].curveAction != NULL)
        {
            delete m_qwtCurves[i].curveAction;
        }
        if(m_qwtCurves[i].mapper != NULL)
        {
            delete m_qwtCurves[i].mapper;
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


    for(int i = 0; i < m_qwtCurves.size(); ++i)
    {
        m_qwtCurves[i].curveAction = new QAction(m_qwtCurves[i].title.c_str(), this);
        m_qwtCurves[i].mapper = new QSignalMapper(this);

        m_qwtCurves[i].mapper->setMapping(m_qwtCurves[i].curveAction, i);
        connect(m_qwtCurves[i].curveAction,SIGNAL(triggered()),
                         m_qwtCurves[i].mapper,SLOT(map()));

        ui->menuCurves->addAction(m_qwtCurves[i].curveAction);
        connect( m_qwtCurves[i].mapper, SIGNAL(mapped(int)), SLOT(visibleCursorMenuSelect(int)) );

        if(m_qwtCurves[i].displayed)
        {
            tMenuActionMapper actionMapper;
            actionMapper.action = new QAction(m_qwtCurves[i].title.c_str(), this);
            actionMapper.mapper = new QSignalMapper(this);

            actionMapper.mapper->setMapping(actionMapper.action, i);
            connect(actionMapper.action,SIGNAL(triggered()),
                             actionMapper.mapper,SLOT(map()));

            ui->menuSelected->addAction(actionMapper.action);
            connect( actionMapper.mapper, SIGNAL(mapped(int)), SLOT(selectedCursorMenuSelect(int)) );

            m_selectedCursorActions.push_back(actionMapper);
        }

    }


    // Handle situation where a cursor has been made invisible but it was the selected cursor
    if(m_qwtCurves[selectedCurveIndex].displayed == false && m_selectedCursorActions.size() > 0)
    {
        int i = 1;
        int newSelectedCurveIndex = 0;
        while(i < m_qwtCurves.size())
        {
            newSelectedCurveIndex = (selectedCurveIndex - i);
            if(newSelectedCurveIndex < 0)
            {
                newSelectedCurveIndex += m_qwtCurves.size();
            }
            if(m_qwtCurves[newSelectedCurveIndex].displayed)
            {
                selectedCurveIndex = newSelectedCurveIndex;
                break;
            }
        }
    }


    //m_qwtCurves[curveIndex].curveAction = new QAction(m_qwtCurves[curveIndex].title.c_str(), this);
    //m_qwtCurves[curveIndex].mapper = new QSignalMapper(this);
    ////m_qwtCurves[curveIndex].curveAction = ui->menuCurves->addAction(name.c_str());
    //m_qwtCurves[curveIndex].mapper->setMapping(m_qwtCurves[curveIndex].curveAction, curveIndex);
    //connect(m_qwtCurves[curveIndex].curveAction,SIGNAL(triggered()),
    //                 m_qwtCurves[curveIndex].mapper,SLOT(map()));

    ////QMetaObject::invokeMethod(ui->menuCurves, "addAction", Qt::QueuedConnection, Q_ARG(QAction*, m_qwtCurves[curveIndex].curveAction));

    //ui->menuCurves->addAction(m_qwtCurves[curveIndex].curveAction);
    //connect( m_qwtCurves[curveIndex].mapper, SIGNAL(mapped(int)), SLOT(visibleCursorMenuSelect(int)) );
}

void MainWindow::add1dCurve(std::string name, dubVect yPoints)
{
    m_qwtCurves.push_back(CurveData(name, yPoints));

    QwtPlotCurve* curve = m_qwtCurves[m_qwtCurves.size()-1].getCurve();

    int vectSize = yPoints.size();
    int curveIndex = m_qwtCurves.size()-1;
    m_qwtCurves[curveIndex].xPoints.resize(vectSize);
    m_qwtCurves[curveIndex].yPoints = yPoints;

    //addMenuItem(curveIndex);

    //myThread->start();


    for(int i = 0; i < vectSize; ++i)
    {
        m_qwtCurves[curveIndex].xPoints[i] = (double)i;
    }

    m_qwtCurves[curveIndex].maxMin.minX = 0;
    m_qwtCurves[curveIndex].maxMin.maxX = vectSize-1;
    m_qwtCurves[curveIndex].maxMin.minY = m_qwtCurves[curveIndex].yPoints[0];
    m_qwtCurves[curveIndex].maxMin.maxY = m_qwtCurves[curveIndex].yPoints[0];

    for(int i = 1; i < vectSize; ++i)
    {
        if(m_qwtCurves[curveIndex].maxMin.minY > m_qwtCurves[curveIndex].yPoints[i])
        {
            m_qwtCurves[curveIndex].maxMin.minY = m_qwtCurves[curveIndex].yPoints[i];
        }
        if(m_qwtCurves[curveIndex].maxMin.maxY < m_qwtCurves[curveIndex].yPoints[i])
        {
            m_qwtCurves[curveIndex].maxMin.maxY = m_qwtCurves[curveIndex].yPoints[i];
        }
    }

    m_qwtCurves[curveIndex].displayed = true;

    int colorLookupIndex = curveIndex % ARRAY_SIZE(curveColors);
    m_qwtCurves[curveIndex].color = QColor(
                curveColors[colorLookupIndex].red,
                curveColors[colorLookupIndex].green,
                curveColors[colorLookupIndex].blue);

    curve->setPen(m_qwtCurves[curveIndex].color);

    curve->setSamples( &m_qwtCurves[curveIndex].xPoints[0],
                       &m_qwtCurves[curveIndex].yPoints[0],
                       vectSize);

    curve->attach(m_qwtPlot);

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
    m_qwtPicker->setStateMachine( new QwtPickerDragRectMachine() );
}


void MainWindow::deltaCursorMode()
{

}


void MainWindow::zoomMode()
{
    m_selectMode = E_ZOOM;
    //m_qwtPicker->setStateMachine( new QwtPickerDragPointMachine() );
}


void MainWindow::resetZoom()
{
    calcMaxMin();
    m_qwtPlot->setAxisScale(
        QwtPlot::yLeft, maxMin.minY, maxMin.maxY);
    m_qwtPlot->setAxisScale(
        QwtPlot::xBottom, maxMin.minX, maxMin.maxX);
    m_qwtPlot->replot();
}

void MainWindow::visibleCursorMenuSelect(int index)
{
    m_qwtCurves[index].displayed = !m_qwtCurves[index].displayed;
    if(!m_qwtCurves[index].displayed)
    {
        m_qwtCurves[index].curve->detach();
    }
    else
    {
        m_qwtCurves[index].curve->attach(m_qwtPlot);
    }
    m_qwtPlot->replot();
    calcMaxMin();
    emit updateCursorMenusSignal();
}

void MainWindow::selectedCursorMenuSelect(int index)
{
    selectedCurveIndex = index;
}

void MainWindow::calcMaxMin()
{
    int i = 0;
    while(i < m_qwtCurves.size() && m_qwtCurves[i].displayed == false)
    {
        ++i;
    }
    if(i < m_qwtCurves.size())
    {
        maxMin = m_qwtCurves[i].maxMin;
        while(i < m_qwtCurves.size())
        {
            if(m_qwtCurves[i].displayed)
            {
                if(maxMin.minX > m_qwtCurves[i].maxMin.minX)
                {
                    maxMin.minX = m_qwtCurves[i].maxMin.minX;
                }
                if(maxMin.minY > m_qwtCurves[i].maxMin.minY)
                {
                    maxMin.minY = m_qwtCurves[i].maxMin.minY;
                }
                if(maxMin.maxX < m_qwtCurves[i].maxMin.maxX)
                {
                    maxMin.maxX = m_qwtCurves[i].maxMin.maxX;
                }
                if(maxMin.maxY < m_qwtCurves[i].maxMin.maxY)
                {
                    maxMin.maxY = m_qwtCurves[i].maxMin.maxY;
                }
            }
            ++i;
        }
    }

}




void MainWindow::pointSelected(const QPointF &pos)
{
    if(m_selectMode == E_CURSOR)
    {
        // 1d assumed
        int posIndex = (int)(pos.x() + 0.5);

        if(posIndex < 0)
        {
            posIndex = 0;
        }
        else if(posIndex >= m_qwtCurves[selectedCurveIndex].xPoints.size())
        {
            posIndex = m_qwtCurves[selectedCurveIndex].xPoints.size()-1;
        }

        m_qwtSelectedSample.showCursor(m_qwtPlot,
                                       m_qwtCurves[selectedCurveIndex].xPoints[posIndex],
                                       m_qwtCurves[selectedCurveIndex].yPoints[posIndex],
                                       m_qwtCurves[selectedCurveIndex].color,
                                       QwtSymbol::Ellipse);

        m_qwtPlot->replot();

        for(int i = 0; i < m_qwtCurves.size(); ++i)
        {
            if(m_qwtCurves[i].pointLabel != NULL)
            {
                ui->InfoLayout->removeWidget(m_qwtCurves[i].pointLabel);
                delete m_qwtCurves[i].pointLabel;
                m_qwtCurves[i].pointLabel = NULL;
            }

            m_qwtCurves[i].pointLabel = new QLabel("");

            char tempText[100];
            snprintf(tempText, sizeof(tempText), "%d : %f", posIndex, (float)m_qwtCurves[i].yPoints[posIndex]);
            m_qwtCurves[i].pointLabel->setText(tempText);

            QPalette palette = this->palette();
            palette.setColor( QPalette::WindowText, m_qwtCurves[i].color);
            palette.setColor( QPalette::Text, m_qwtCurves[i].color);
            m_qwtCurves[i].pointLabel->setPalette(palette);

            ui->InfoLayout->addWidget(m_qwtCurves[i].pointLabel);
        }


    }


}


void MainWindow::rectSelected(const QRectF &pos)
{
    if(m_selectMode == E_ZOOM)
    {
        QRectF rectCopy = pos;
        qreal x1, y1, x2, y2;

        rectCopy.getCoords(&x1, &y1, &x2, &y2);

        m_qwtPlot->setAxisScale(QwtPlot::yLeft, y1, y2);
        m_qwtPlot->setAxisScale(QwtPlot::xBottom, x1, x2);

        m_qwtSelectedSample.hideCursor();

        m_qwtPlot->replot();
    }

}
