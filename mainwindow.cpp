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

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mi_size = 100;
    mi_index = 0;


    QwtText t_plotTitle("t_title");
    int i_index;

    mt_qwtPlot = new QwtPlot(t_plotTitle, this);
    mt_qwfPlotCurve = new QwtPlotCurve("Curve 1");

    md_x = new double[mi_size];
    md_y = new double[mi_size];


    for(i_index = 0; i_index < mi_size; ++i_index)
    {
       md_x[i_index] = i_index;
       md_y[i_index] = (double)i_index;//0.0;
    }

    mt_qwfPlotCurve->setSamples(md_x, md_y, mi_size);
    mt_qwfPlotCurveSelectedSample = new QwtPlotCurve("SelectedSample");

    mt_qwfPlotCurve->attach(mt_qwtPlot);


    //mt_qwtPlot->setAxisScale(QwtPlot::yLeft, -25, 25);
    //mt_qwtPlot->setAxisScale(QwtPlot::xBottom, -25, 25);
    mt_qwtPlot->replot();
    mt_qwtPlot->show();


    //picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
    //        QwtPlotPicker::NoRubberBand, QwtPicker::AlwaysOn,
    //        mt_qwtPlot->canvas());
    picker = new QwtPlotPicker(mt_qwtPlot->canvas());
    picker->setStateMachine(new QwtPickerDragRectMachine());//QwtPickerClickPointMachine());
    //picker->setMousePattern(QwtEventPattern::MouseSelect1,Qt::LeftButton);

    //connect(picker, SIGNAL(appended(QPointF)),
    //        this, SLOT(pointSelected(QPointF)));
    connect(picker, SIGNAL(selected(QRectF)),
            this, SLOT(rectSelected(QRectF)));

}

MainWindow::~MainWindow()
{
    delete ui;delete mt_qwfPlotCurve;
    delete mt_qwtPlot;
    delete [] md_x;
    delete [] md_y;
 }

 void MainWindow::addValue(double i_value)
 {
    md_y[mi_index] = i_value;

    if(++mi_index == mi_size)
    {
       mi_index = 0;
    }
 }

 void MainWindow::updatePlot()
 {
    mt_qwfPlotCurve->setSamples(md_x, md_y, mi_size);
    mt_qwtPlot->replot();
 }

void MainWindow::pointSelected(const QPointF &pos)
{
    lastPointSelected = pos;


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



}


void MainWindow::rectSelected(const QRectF &pos)
{
    QRectF rectCopy = pos;
    qreal x1, y1, x2, y2;

    rectCopy.getCoords(&x1, &y1, &x2, &y2);

    mt_qwtPlot->setAxisScale(QwtPlot::yLeft, y1, y2);
    mt_qwtPlot->setAxisScale(QwtPlot::xBottom, x1, x2);
    mt_qwtPlot->replot();
    if(x1 == 0.0 && x2 == 0.0 && y1 == 0.0 && y2 == 0.0)
    {
        mi_index =0;
    }

}
