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
#ifndef PLOTGUIMAIN_H
#define PLOTGUIMAIN_H

#include <string>

#include <QList>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMap>

#include "mainwindow.h"
#include "TCPMsgReader.h"
#include "PlotHelperTypes.h"
#include "CurveData.h"
#include "CurveCommander.h"


class createFFTPlot;

namespace Ui {
class plotGuiMain;
}

class plotGuiMain : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit plotGuiMain(QWidget *parent = 0, unsigned short tcpPort = 0);
    ~plotGuiMain();
    QMap<QString, MainWindow*> m_plotGuis;

    void readPlotMsg(const char *msg, unsigned int size);

    void createFftGuiFinished(){emit createFftGuiFinishedSignal();}
    void plotWindowClose(QString plotName){emit plotWindowCloseSignal(plotName);}
    void closeAllPlots();

    void curveUpdated(QString plotName, QString curveName, CurveData* curveData);


private:

    void removeHiddenPlotWindows();
    void makeFFTPlot(tFFTCurve fftCurve);
    void updateFFTPlot(QString srcPlotName, QString srcCurveName);

    void updateFFTList(const tFFTCurve &fftCurve);

    void removeFromFFTList(QString plotName);
    void removeFromFFTList(QString plotName, QString curveName);

    Ui::plotGuiMain *ui;
    TCPMsgReader* m_tcpMsgReader;


    QSystemTrayIcon* m_trayIcon;
    QAction m_trayExitAction;
    QAction m_trayComplexFFTAction;
    QAction m_trayRealFFTAction;
    QAction m_trayEnDisNewCurvesAction;
    QMenu* m_trayMenu;

    createFFTPlot* m_createFFTPlotGUI;

    CurveCommander m_curveCommander;

    QVector<tFFTCurve> m_fftCurves;

    bool m_allowNewCurves;

    volatile bool m_closingPlots;

public slots:
    void readPlotMsgSlot(const char *msg, unsigned int size);
    void createComplexFFT();
    void createRealFFT();
    void enDisNewCurves();

    void createFftGuiFinishedSlot();
    void plotWindowCloseSlot(QString plotName);
    void closeAllPlotsSlot();

signals:
    void readPlotMsgSignal(const char*, unsigned int);
    void createFftGuiFinishedSignal();
    void plotWindowCloseSignal(QString plotName);
    void closeAllPlotsSignal();
};

#endif // PLOTGUIMAIN_H
