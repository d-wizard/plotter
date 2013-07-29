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
#ifndef CREATEFFTPLOT_H
#define CREATEFFTPLOT_H

#include <QWidget>

#include "PlotHelperTypes.h"
#include "CurveCommander.h"

class plotGuiMain;


namespace Ui {
class createFFTPlot;
}

class createFFTPlot : public QWidget
{
    Q_OBJECT
    
public:
    explicit createFFTPlot(plotGuiMain* guiParent, tCurveCommanderInfo &initPlotsCurves, eFFTType fftType, QWidget *parent = 0);
    ~createFFTPlot();

    void plotsCurvesChanged(const tCurveCommanderInfo &pc);
    bool getFFTData(tFFTCurve& fftCurveData);


private slots:
    void on_cmdOk_clicked();
    void on_cmdCancel_clicked();

    void on_cmbRePlot_currentIndexChanged(int index);

    void on_cmbImPlot_currentIndexChanged(int index);

    void on_cmbOutPlot_currentIndexChanged(int index);

private:
    Ui::createFFTPlot *ui;
    plotGuiMain* m_guiMainParent;
    tCurveCommanderInfo m_plotsAndCurves;

    bool m_guiChanging;

    void updateGui();

    bool m_validUserInput;
    tFFTCurve m_fftCurveData;

    void closeEvent(QCloseEvent* event);
};

#endif // CREATEFFTPLOT_H
