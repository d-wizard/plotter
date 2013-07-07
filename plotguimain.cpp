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
#include <string>
#include "PlotHelperTypes.h"

#include "plotguimain.h"
#include "ui_plotguimain.h"

plotGuiMain::plotGuiMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::plotGuiMain),
    m_trayIcon(NULL),
    m_trayExitAction(NULL),
    m_trayMenu(NULL)
{
    ui->setupUi(this);

    QObject::connect(this, SIGNAL(readPlotMsgSignal(const char*, unsigned int)),
                     this, SLOT(readPlotMsgSlot(const char*, unsigned int)), Qt::QueuedConnection);

    //m_plotGuis.push_back(new MainWindow());
    //m_plotGuis[m_plotGuis.size()-1]->show();

    m_tcpMsgReader = new TCPMsgReader(this, 2000);

    m_trayIcon = new QSystemTrayIcon(QIcon("plot.png"), this);
    m_trayExitAction = new QAction("Exit", this);
    m_trayMenu = new QMenu("Exit", this);

    m_trayMenu->addAction(m_trayExitAction);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayExitAction, SIGNAL(triggered(bool)), QCoreApplication::instance(), SLOT(quit()));

    m_trayIcon->show();


}

plotGuiMain::~plotGuiMain()
{
    delete m_tcpMsgReader;

    delete m_trayIcon;

    std::map<std::string, MainWindow*>::iterator iter;
    for(iter = m_plotGuis.begin(); iter != m_plotGuis.end(); ++iter)
    {
        delete iter->second;
    }

    delete ui;
}

void plotGuiMain::removeHiddenPlotWindows()
{
    std::map<std::string, MainWindow*>::iterator iter = m_plotGuis.begin();
    while(iter != m_plotGuis.end())
    {
        if(iter->second->isVisible() == false)
        {
            delete iter->second;
            m_plotGuis.erase(iter++);
        }
        else
        {
            ++iter;
        }
    }
}

void plotGuiMain::readPlotMsg(const char* msg, unsigned int size)
{
    char* msgCopy = new char[size];
    memcpy(msgCopy, msg, size);
    emit readPlotMsgSignal(msgCopy,size);
}

void plotGuiMain::readPlotMsgSlot(const char* msg, unsigned int size)
{
    removeHiddenPlotWindows();

    UnpackPlotMsg msgUnpacker(msg, size);

    if(validPlotAction(msgUnpacker.m_plotAction))
    {
        if(m_plotGuis.find(msgUnpacker.m_plotName) == m_plotGuis.end())
        {
           m_plotGuis[msgUnpacker.m_plotName] = new MainWindow();
           m_plotGuis[msgUnpacker.m_plotName]->setWindowTitle(msgUnpacker.m_plotName.c_str());
        }
        switch(msgUnpacker.m_plotAction)
        {
        case E_PLOT_1D:
            m_plotGuis[msgUnpacker.m_plotName]->add1dCurve(msgUnpacker.m_curveName, msgUnpacker.m_yAxisValues);
            m_plotGuis[msgUnpacker.m_plotName]->show();
            break;
        case E_PLOT_2D:
            m_plotGuis[msgUnpacker.m_plotName]->add2dCurve(msgUnpacker.m_curveName, msgUnpacker.m_xAxisValues, msgUnpacker.m_yAxisValues);
            m_plotGuis[msgUnpacker.m_plotName]->show();
            break;
        default:
            break;
        }
    }

    delete[] msg;
}

