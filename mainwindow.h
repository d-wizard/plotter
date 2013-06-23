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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <vector>
#include <math.h>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_scale_widget.h>


typedef std::vector<double> dubVect;



class CurveData
{
public:
    CurveData(QwtPlotCurve* newCurve):
        curve(newCurve){}
    ~CurveData()
    {
        if(curve != NULL)
        {
            //delete curve;
        }
    }

    QwtPlotCurve* curve;
    dubVect xPoints;
    dubVect yPoints;
    double minX;
    double minY;
    double maxX;
    double maxY;

    QColor color;
private:
    CurveData();
};

typedef enum
{
    E_CURSOR,
    E_ZOOM
}eSelectMode;



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    Ui::MainWindow *ui;


    QwtPlot* m_qwtPlot;
    std::vector<CurveData> m_qwtCurves;
    QwtPlotCurve* m_qwtSelectedSample;
    QwtPlotCurve* m_qwtSelectedSampleDelta;
    QwtPlotPicker* m_qwtPicker;
    QwtPlotGrid* m_qwtGrid;

    eSelectMode m_selectMode;

    int selectedCurveIndex;


    void resetPlot();
    void add1dCurve(std::string name, dubVect yPoints);
    void add2dCurve(std::string name, dubVect xPoints, dubVect yPoints);

private slots:
    void pointSelected(const QPointF &pos);
    void rectSelected(const QRectF &pos);

    // Menu Commands
    void toggleLegend();
    void cursorMode();
    void deltaCursorMode();
    void zoomMode();
    void resetZoom();

};

#endif // MAINWINDOW_H
