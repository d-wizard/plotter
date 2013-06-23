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


QwtPlotGrid *grid;
int mi_size;
int mi_index;
dubVect md_x;
dubVect md_y;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_qwtPlot(NULL),
    m_qwtSelectedSample(NULL),
    m_qwtSelectedSampleDelta(NULL),
    m_qwtPicker(NULL),
    m_selectMode(E_CURSOR)
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

    resetPlot();
    md_x.resize(mi_size);
    md_y.resize(mi_size);


    for(int i_index = 0; i_index < mi_size; ++i_index)
    {
       md_x[i_index] = i_index;
       md_y[i_index] = sin((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
    }

    add1dCurve("Curve1", md_y);

}

MainWindow::~MainWindow()
{
    delete ui;
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
    m_qwtGrid = new QwtPlotGrid();
    m_qwtGrid->attach( m_qwtPlot );
    m_qwtGrid->setPen(QColor(55,55,55));
    ui->GraphLayout->addWidget(m_qwtPlot);

    if(m_qwtPlot != NULL)
    {
        delete m_qwtPicker;
    }
    m_qwtPicker = new QwtPlotPicker(m_qwtPlot->canvas());
    m_qwtPicker->setStateMachine(new QwtPickerDragRectMachine());

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

    for(int i = 0; i < vectSize; ++i)
    {
        m_qwtCurves[curveIndex].xPoints[i] = (double)i;
    }

    m_qwtCurves[curveIndex].color = QColor(0,255,255);
    curve->setPen(m_qwtCurves[curveIndex].color);

    curve->setSamples( &m_qwtCurves[curveIndex].xPoints[0],
                       &m_qwtCurves[curveIndex].yPoints[0],
                       vectSize);

    curve->attach(m_qwtPlot);

    m_qwtPlot->replot();
    m_qwtPlot->show();
    m_qwtGrid->show();
}


void MainWindow::add2dCurve(std::string name, dubVect xPoints, dubVect yPoints)
{

}


void MainWindow::toggleLegend()
{

}


void MainWindow::cursorMode()
{

}


void MainWindow::deltaCursorMode()
{

}


void MainWindow::zoomMode()
{

}


void MainWindow::resetZoom()
{

}




void MainWindow::pointSelected(const QPointF &pos)
{
#if 0
    QPoint blah = pos.toPoint();
    int i = mt_qwfPlotCurve->closestPoint(blah, NULL);
    QPointF found = mt_qwfPlotCurve->sample(i);
    qreal x = pos.x();
    int roundX = (int)(x + 0.5);
    double yVal = md_y[roundX];


    if(roundX < 0)
    {
        roundX = 0;
    }
    else if(roundX >= mi_size)
    {
        roundX = mi_size - 1;
    }

    QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
        QBrush( Qt::yellow ), QPen( Qt::red, 2 ), QSize( 8, 8 ) );
    mt_qwfPlotCurveSelectedSample->setSymbol( symbol );
    mt_qwfPlotCurveSelectedSample->setSamples(&md_x[roundX], &md_y[roundX], 1);

    mt_qwfPlotCurveSelectedSample->attach(mt_qwtPlot);

    mt_qwtPlot->replot();
    //mt_qwtPlot->show();

    if(yVal == 0 || i == 0 || found.x() == 0.0)
    {
        mi_index = (int)blah.rx();
    }
    else
    {
        mi_index = 1;
    }
#endif


}


void MainWindow::rectSelected(const QRectF &pos)
{
    QRectF rectCopy = pos;
    qreal x1, y1, x2, y2;
#if 0
    rectCopy.getCoords(&x1, &y1, &x2, &y2);

    mt_qwtPlot->setAxisScale(QwtPlot::yLeft, y1, y2);
    mt_qwtPlot->setAxisScale(QwtPlot::xBottom, x1, x2);
    mt_qwtPlot->replot();
    if(x1 == 0.0 && x2 == 0.0 && y1 == 0.0 && y2 == 0.0)
    {
        mi_index =0;
    }
#endif
}
