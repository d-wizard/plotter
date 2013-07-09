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

#include "mainwindow.h"
#include "TCPMsgReader.h"
#include "PlotHelperTypes.h"

#include <QMap>

namespace Ui {
class plotGuiMain;
}

class plotGuiMain : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit plotGuiMain(QWidget *parent = 0);
    ~plotGuiMain();
    QMap<QString, MainWindow*> m_plotGuis;

    void readPlotMsg(const char *msg, unsigned int size);
    
private:
    Ui::plotGuiMain *ui;
    TCPMsgReader* m_tcpMsgReader;

    void removeHiddenPlotWindows();


    QSystemTrayIcon* m_trayIcon;
    QAction* m_trayExitAction;
    QMenu* m_trayMenu;



public slots:
    void readPlotMsgSlot(const char *msg, unsigned int size);
signals:
    void readPlotMsgSignal(const char*, unsigned int);
};

#endif // PLOTGUIMAIN_H
