/* Copyright 2013 - 2017 Dan Williams. All Rights Reserved.
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
#include <queue>
#include <list>

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMap>
#include <QSemaphore>
#include <QMutex>

#include "mainwindow.h"
#include "TCPMsgReader.h"
#include "PlotHelperTypes.h"
#include "CurveData.h"
#include "CurveCommander.h"

#define STORED_MSG_SIZE (40*1024*1024)

class createFFTPlot;

namespace Ui {
class plotGuiMain;
}

class plotGuiMain : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit plotGuiMain(QWidget *parent, std::vector<unsigned short> tcpPorts, bool showTrayIcon);
    ~plotGuiMain();

    void startPlotMsgProcess(tIncomingMsg* inMsg);

    void closeAllPlotsFromLib();

    void readPlotMsg(UnpackMultiPlotMsg *plotMsg);
    void restorePlotMsg(const char *msg, unsigned int size, tPlotCurveName plotCurveName);

    CurveCommander& getCurveCommander(){return m_curveCommander;}

    void restorePlotFile(std::string plotFilePath);

private:

    Ui::plotGuiMain *ui;
    std::vector<TCPMsgReader*> m_tcpMsgReaders;

    QSystemTrayIcon* m_trayIcon;
    QAction m_trayExitAction;
    QAction m_trayEnDisNewCurvesAction;
    QAction m_propertiesWindowAction;
    QAction m_closeAllPlotsAction;
    QAction m_revDateStampAction;
    QAction m_updateBinaryAction;
    QMenu* m_trayMenu;

    QSemaphore m_sem;

    CurveCommander m_curveCommander;

    bool m_allowNewCurves;

    char* m_storedMsgBuff;
    unsigned int m_storedMsgBuffIndex;
    QMutex m_storedMsgBuffMutex;

    std::queue<UnpackMultiPlotMsg*> m_multiPlotMsgs;
    QMutex m_multiPlotMsgsQueueMutex;

    std::list<std::string> m_plotFilesToRestoreList;
    QMutex m_plotFilesToRestoreMutex;

    void closeEvent(QCloseEvent* event);

    bool processNonSamplePlotMsgs(UnpackMultiPlotMsg* plotMsg);

public slots:
    void readPlotMsgSlot();
    void enDisNewCurves();
    void showPropertiesGui();
    void closeAllPlotsFromLibSlot();
    void closeAllPlotsSlot();
    void restorePlotFilesInListSlot();
    void updateBinarySlot();
signals:
    void readPlotMsgSignal();
    void closeAllPlotsFromLibSignal();
    void restorePlotFilesInListSignal();
private slots:
    void on_cmdClose_clicked();
};

#endif // PLOTGUIMAIN_H
