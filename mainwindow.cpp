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


#if 0
    int i_index;
    mt_qwtPlot = new QwtPlot( this);
    mt_qwfPlotCurve = new QwtPlotCurve("Curve 1");
    ui->GraphLayout->addWidget(mt_qwtPlot);

    mt_qwfPlotCurve->setPen(Qt::cyan);

    mt_qwtPlot->insertLegend(new QwtLegend() );
    //mt_qwtPlot->insertLegend(NULL);

    grid = new QwtPlotGrid();
    grid->attach( mt_qwtPlot );
    grid->setPen(QColor(55,55,55));


    md_x = new double[mi_size];
    md_y = new double[mi_size];


    for(i_index = 0; i_index < mi_size; ++i_index)
    {
       md_x[i_index] = i_index;
       md_y[i_index] = sin((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
    }

    mt_qwfPlotCurve->setSamples(md_x, md_y, mi_size);
    mt_qwfPlotCurveSelectedSample = new QwtPlotCurve("");

    mt_qwfPlotCurve->attach(mt_qwtPlot);


    //mt_qwtPlot->setAxisScale(QwtPlot::yLeft, -25, 25);
    //mt_qwtPlot->setAxisScale(QwtPlot::xBottom, -25, 25);
    mt_qwtPlot->replot();
    mt_qwtPlot->show();
    grid->show();


    //picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
    //        QwtPlotPicker::NoRubberBand, QwtPicker::AlwaysOn,
    //        mt_qwtPlot->canvas());
    picker = new QwtPlotPicker(mt_qwtPlot->canvas());
    picker->setStateMachine(new QwtPickerDragRectMachine());//QwtPickerClickPointMachine());
    //picker->setMousePattern(QwtEventPattern::MouseSelect1,Qt::LeftButton);

    connect(picker, SIGNAL(appended(QPointF)),
            this, SLOT(pointSelected(QPointF)));
    connect(picker, SIGNAL(selected(QRectF)),
            this, SLOT(rectSelected(QRectF)));
#endif
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
    //add1dCurve("Curve1", md_y);
    //add1dCurve("Curve2", md_x);
    //add1dCurve("Curve3", md_z);


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

    m_qwtPlot->show();
    m_qwtGrid->show();

    connect(m_qwtPicker, SIGNAL(appended(QPointF)),
            this, SLOT(pointSelected(QPointF)));
    connect(m_qwtPicker, SIGNAL(selected(QRectF)),
            this, SLOT(rectSelected(QRectF)));
}

void MainWindow::add1dCurve(std::string name, dubVect yPoints)
{
    QwtPlotCurve* curve = new QwtPlotCurve(name.c_str());
    CurveData curvDat = CurveData(curve);

    m_qwtCurves.push_back(curvDat);

    int vectSize = yPoints.size();
    int curveIndex = m_qwtCurves.size()-1;
    m_qwtCurves[curveIndex].xPoints.resize(vectSize);
    m_qwtCurves[curveIndex].yPoints = yPoints;

    //m_qwtCurves[curveIndex].curveAction = new QAction(name.c_str(), this);
    //m_qwtCurves[curveIndex].curveAction = ui->menuCurves->addAction(name.c_str());
    //connect(m_qwtCurves[curveIndex].curveAction,SIGNAL(triggered()),
     //                this,SLOT(cursorMenuSelect()));

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
}


void MainWindow::deltaCursorMode()
{

}


void MainWindow::zoomMode()
{
    m_selectMode = E_ZOOM;
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

void MainWindow::cursorMenuSelect()
{
    calcMaxMin();
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
