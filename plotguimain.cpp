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

//#define TEST_CURVES

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

#ifdef TEST_CURVES
    std::string plotName = "Test Plot";
    m_plotGuis[plotName] = new MainWindow();
    m_plotGuis[plotName]->setWindowTitle(plotName.c_str());
    m_plotGuis[plotName]->show();


    int mi_size = 1000;
    dubVect md_x;
    dubVect md_y;
    dubVect md_z;
    dubVect md_x1;
    dubVect md_y1;

    // init test samples.
    md_x.resize(mi_size);
    md_y.resize(mi_size);
    md_z.resize(mi_size);
    md_x1.resize(mi_size/2);
    md_y1.resize(mi_size/2);
    //md_z.resize(mi_size/2);

    for(int i_index = 0; i_index < mi_size; ++i_index)
    {
        md_x[i_index] = 4.0 *cos((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;//i_index;
        md_y[i_index] = 2.0 * sin((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
        md_z[i_index] = sin((3.14159 * 4.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
    }

    for(int i_index = 0; i_index < mi_size/2; ++i_index)
    {
        //md_x[i_index] = cos((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;//i_index;
        //md_y[i_index] = sin((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
        //md_x1[i_index] = 2.0*cos((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;//i_index;
        //md_y1[i_index] = 2.0*sin((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
        md_x1[i_index] = 2.0*cos((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;//i_index;
        md_y1[i_index] = 2.0*sin((3.14159 * 2.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
        //md_z[i_index] = sin((3.14159 * 4.0 * (double)i_index)/(double)mi_size);//(double)i_index;//0.0;
    }
    m_plotGuis[plotName]->add1dCurve("Curve1", md_y);
    m_plotGuis[plotName]->add1dCurve("Curve2", md_x);
    m_plotGuis[plotName]->add1dCurve("Curve3", md_z);
    //m_plotGuis[plotName]->add2dCurve("Curve1", md_x, md_y);
    //m_plotGuis[plotName]->add2dCurve("Curve2", md_x1, md_y1);
#endif

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

    QMap<std::string, MainWindow*>::iterator iter;
    for(iter = m_plotGuis.begin(); iter != m_plotGuis.end(); ++iter)
    {
        delete iter.value();
    }

    delete ui;
}

void plotGuiMain::removeHiddenPlotWindows()
{
    QMap<std::string, MainWindow*>::iterator iter = m_plotGuis.begin();
    while(iter != m_plotGuis.end())
    {
        if(iter.value()->isVisible() == false)
        {
            delete iter.value();
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
            m_plotGuis[msgUnpacker.m_plotName]->add1dCurve(msgUnpacker.m_curveName.c_str(), msgUnpacker.m_yAxisValues);
            m_plotGuis[msgUnpacker.m_plotName]->show();
            break;
        case E_PLOT_2D:
            m_plotGuis[msgUnpacker.m_plotName]->add2dCurve(msgUnpacker.m_curveName.c_str(), msgUnpacker.m_xAxisValues, msgUnpacker.m_yAxisValues);
            m_plotGuis[msgUnpacker.m_plotName]->show();
            break;
        default:
            break;
        }
    }

    delete[] msg;
}

