/* Copyright 2013 - 2014 Dan Williams. All Rights Reserved.
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

#include "createfftplot.h"
#include "fftHelper.h"

//#define TEST_CURVES

plotGuiMain::plotGuiMain(QWidget *parent, unsigned short tcpPort, bool showTrayIcon) :
    QMainWindow(parent),
    ui(new Ui::plotGuiMain),
    m_tcpMsgReader(NULL),
    m_trayIcon(NULL),
    m_trayExitAction("Exit", this),
    m_trayEnDisNewCurvesAction("Disable New Curves", this),
    m_propertiesWindowAction("Properties", this),
    m_trayMenu(NULL),
    m_curveCommander(this),
    m_allowNewCurves(true)
{
    ui->setupUi(this);
    this->setFixedSize(165, 95);

    QObject::connect(this, SIGNAL(readPlotMsgSignal(const char*, unsigned int)),
                     this, SLOT(readPlotMsgSlot(const char*, unsigned int)), Qt::QueuedConnection);

#ifdef TEST_CURVES
    QString plotName = "Test Plot";
    m_plotGuis[plotName] = new MainWindow(this);
    m_plotGuis[plotName]->setWindowTitle(plotName);
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

    if(tcpPort != 0)
    {
        m_tcpMsgReader = new TCPMsgReader(this, tcpPort);

        char tcpPortStr[10];
        snprintf(tcpPortStr, sizeof(tcpPortStr), "%d", tcpPort);
        ui->lblPort->setText(tcpPortStr);
    }
    else
    {
       ui->lblPort->setVisible(false);
       ui->lblPortLable->setVisible(false);
    }

    connect(&m_trayExitAction, SIGNAL(triggered(bool)), QCoreApplication::instance(), SLOT(quit()));
    connect(&m_trayEnDisNewCurvesAction, SIGNAL(triggered(bool)), this, SLOT(enDisNewCurves()));
    connect(&m_propertiesWindowAction, SIGNAL(triggered(bool)), this, SLOT(showPropertiesGui()));

    m_trayMenu = new QMenu("Plot", this);

    m_trayMenu->addAction(&m_trayEnDisNewCurvesAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(&m_propertiesWindowAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(&m_trayExitAction);

    ui->menubar->addMenu(m_trayMenu);
    
    if(showTrayIcon == true)
    {
       m_trayIcon = new QSystemTrayIcon(QIcon(":/plot.png"), this);
       m_trayIcon->setContextMenu(m_trayMenu);
       m_trayIcon->show();
    }

    QObject::connect(this, SIGNAL(closeAllPlotsSignal()),
                     this, SLOT(closeAllPlotsSlot()), Qt::QueuedConnection);
}

plotGuiMain::~plotGuiMain()
{
    if(m_tcpMsgReader != NULL)
    {
        delete m_tcpMsgReader;
    }

    if(m_trayIcon != NULL)
    {
       delete m_trayIcon;
    }

    if(m_trayMenu != NULL)
    {
       delete m_trayMenu;
    }

    m_curveCommander.destroyAllPlots();
    delete ui;
}

void plotGuiMain::closeAllPlots()
{
   emit closeAllPlotsSignal();
   m_sem.acquire();
}

void plotGuiMain::closeAllPlotsSlot()
{
   m_curveCommander.destroyAllPlots();
   m_sem.release();
}

void plotGuiMain::readPlotMsg(const char* msg, unsigned int size)
{
   if(m_allowNewCurves == true)
   {
      char* msgCopy = new char[size];
      memcpy(msgCopy, msg, size);
      emit readPlotMsgSignal(msgCopy,size);
   }
}

void plotGuiMain::readPlotMsgSlot(const char* msg, unsigned int size)
{
    UnpackPlotMsg msgUnpacker(msg, size);

    if(validPlotAction(msgUnpacker.m_plotAction))
    {
        QString plotName(msgUnpacker.m_plotName.c_str());
        switch(msgUnpacker.m_plotAction)
        {
        case E_CREATE_1D_PLOT:
            m_curveCommander.create1dCurve(plotName, msgUnpacker.m_curveName.c_str(), E_PLOT_TYPE_1D, msgUnpacker.m_yAxisValues);
            break;
        case E_CREATE_2D_PLOT:
            m_curveCommander.create2dCurve(plotName, msgUnpacker.m_curveName.c_str(), msgUnpacker.m_xAxisValues, msgUnpacker.m_yAxisValues);
            break;
        case E_UPDATE_1D_PLOT:
            m_curveCommander.update1dCurve(plotName, msgUnpacker.m_curveName.c_str(), E_PLOT_TYPE_1D, msgUnpacker.m_sampleStartIndex, msgUnpacker.m_yAxisValues);
            break;
        case E_UPDATE_2D_PLOT:
            m_curveCommander.update2dCurve(plotName, msgUnpacker.m_curveName.c_str(), msgUnpacker.m_sampleStartIndex, msgUnpacker.m_xAxisValues, msgUnpacker.m_yAxisValues);
            break;
        default:
            break;
        }
    }

    delete[] msg;
}

void plotGuiMain::enDisNewCurves()
{
   m_allowNewCurves = !m_allowNewCurves;
   if(m_allowNewCurves == true)
   {
      m_trayEnDisNewCurvesAction.setText("Disable New Curves");
   }
   else
   {
      m_trayEnDisNewCurvesAction.setText("Enable New Curves");
   }
}

void plotGuiMain::showPropertiesGui()
{
   m_curveCommander.showCurvePropertiesGui();
}

void plotGuiMain::plotWindowClose(QString plotName)
{
}


void plotGuiMain::curveUpdated(QString plotName, QString curveName)
{
}

void plotGuiMain::on_cmdClose_clicked()
{
    QApplication::quit();
}

void plotGuiMain::closeEvent(QCloseEvent* /*event*/)
{
    QApplication::quit();
}
