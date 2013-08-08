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

#include "createfftplot.h"
#include "fftHelper.h"

//#define TEST_CURVES

plotGuiMain::plotGuiMain(QWidget *parent, unsigned short tcpPort) :
    m_tcpMsgReader(NULL),
    m_trayIcon(NULL),
    m_trayExitAction("Exit", this),
    m_trayComplexFFTAction("Create Complex FFT", this),
    m_trayRealFFTAction("Create Real FFT", this),
    m_trayEnDisNewCurvesAction("Disable New Curves", this),
    m_trayMenu(NULL),
    m_createFFTPlotGUI(NULL),
    m_curveCommander(this),
    m_allowNewCurves(true)
{

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
    }

    m_trayIcon = new QSystemTrayIcon(QIcon("plot.png"), this);
    m_trayMenu = new QMenu("Exit", this);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(&m_trayExitAction, SIGNAL(triggered(bool)), QCoreApplication::instance(), SLOT(quit()));
    connect(&m_trayComplexFFTAction, SIGNAL(triggered(bool)), this, SLOT(createComplexFFT()));
    connect(&m_trayRealFFTAction, SIGNAL(triggered(bool)), this, SLOT(createRealFFT()));
    connect(&m_trayEnDisNewCurvesAction, SIGNAL(triggered(bool)), this, SLOT(enDisNewCurves()));


    m_trayMenu->addAction(&m_trayEnDisNewCurvesAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(&m_trayRealFFTAction);
    m_trayMenu->addAction(&m_trayComplexFFTAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(&m_trayExitAction);

    m_trayRealFFTAction.setEnabled(false);
    m_trayComplexFFTAction.setEnabled(false);

    m_trayIcon->show();

    QObject::connect(this, SIGNAL(createFftGuiFinishedSignal()),
                     this, SLOT(createFftGuiFinishedSlot()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(plotWindowCloseSignal(QString)),
                     this, SLOT(plotWindowCloseSlot(QString)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(closeAllPlotsSignal()),
                     this, SLOT(closeAllPlotsSlot()), Qt::QueuedConnection);
}

plotGuiMain::~plotGuiMain()
{
    if(m_createFFTPlotGUI != NULL)
    {
        delete m_createFFTPlotGUI;
    }

    if(m_tcpMsgReader != NULL)
    {
        delete m_tcpMsgReader;
    }

    delete m_trayIcon;
    delete m_trayMenu;

    m_curveCommander.destroyAllPlots();
}

void plotGuiMain::closeAllPlotsEmit()
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
        case E_PLOT_1D:
            m_curveCommander.add1dCurve(plotName, msgUnpacker.m_curveName.c_str(), msgUnpacker.m_yAxisValues);
            break;
        case E_PLOT_2D:
            m_curveCommander.add2dCurve(plotName, msgUnpacker.m_curveName.c_str(), msgUnpacker.m_xAxisValues, msgUnpacker.m_yAxisValues);
            break;
        default:
            break;
        }
    }

    delete[] msg;
}

void plotGuiMain::createComplexFFT()
{
    if(m_createFFTPlotGUI == NULL)
    {
        m_createFFTPlotGUI = new createFFTPlot(this, m_curveCommander.getCurveCommanderInfo(), E_FFT_COMPLEX);
    }
}

void plotGuiMain::createRealFFT()
{
    if(m_createFFTPlotGUI == NULL)
    {
        m_createFFTPlotGUI = new createFFTPlot(this, m_curveCommander.getCurveCommanderInfo(), E_FFT_REAL);
    }
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


void plotGuiMain::createFftGuiFinishedSlot()
{
    if(m_createFFTPlotGUI != NULL)
    {
        tFFTCurve temp;
        if(m_createFFTPlotGUI->getFFTData(temp) == true)
        {
            makeFFTPlot(temp);
        }
        delete m_createFFTPlotGUI;
        m_createFFTPlotGUI = NULL;
    }
}

void plotGuiMain::plotWindowCloseSlot(QString plotName)
{
    m_curveCommander.plotRemoved(plotName);
    removeFromFFTList(plotName);

    if(m_curveCommander.getCurveCommanderInfo().size() == 0)
    {
        m_trayRealFFTAction.setEnabled(false);
        m_trayComplexFFTAction.setEnabled(false);
        createFftGuiFinishedSlot();
    }
    else if(m_createFFTPlotGUI != NULL)
    {
        m_createFFTPlotGUI->plotsCurvesChanged(m_curveCommander.getCurveCommanderInfo());
    }


}


void plotGuiMain::curveUpdated(QString plotName, QString curveName, CurveData* curveData)
{
    m_curveCommander.curveUpdated(plotName, curveName, curveData);
    updateFFTPlot(plotName, curveName);
    if(m_createFFTPlotGUI != NULL)
    {
        m_createFFTPlotGUI->plotsCurvesChanged(m_curveCommander.getCurveCommanderInfo());
    }
    if(m_curveCommander.getCurveCommanderInfo().size() > 0)
    {
        m_trayRealFFTAction.setEnabled(true);
        m_trayComplexFFTAction.setEnabled(true);
    }
}

void plotGuiMain::makeFFTPlot(tFFTCurve fftCurve)
{
    dubVect rePoints;
    dubVect imPoints;

    CurveData* srcRe = m_curveCommander.getCurveData(fftCurve.srcRePlotName, fftCurve.srcReCurveName);
    CurveData* srcIm = m_curveCommander.getCurveData(fftCurve.srcImPlotName, fftCurve.srcImCurveName);

    if(fftCurve.fftType == E_FFT_REAL)
    {
        if(srcRe != NULL)
        {
            if(fftCurve.srcReAxis == E_X_AXIS)
            {
                srcRe->getXPoints(rePoints);
            }
            else
            {
                srcRe->getYPoints(rePoints);
            }
            updateFFTList(fftCurve);
            realFFT(rePoints, rePoints);
            m_curveCommander.add1dCurve(fftCurve.plotName, fftCurve.curveName, rePoints);
        }
    }
    else if(fftCurve.fftType == E_FFT_COMPLEX)
    {
        if(srcRe != NULL && srcIm != NULL)
        {
            if(fftCurve.srcReAxis == E_X_AXIS)
            {
                srcRe->getXPoints(rePoints);
            }
            else
            {
                srcRe->getYPoints(rePoints);
            }
            if(fftCurve.srcImAxis == E_X_AXIS)
            {
                srcIm->getXPoints(imPoints);
            }
            else
            {
                srcIm->getYPoints(imPoints);
            }
            updateFFTList(fftCurve);
            dubVect fftXPoints;

            complexFFT(rePoints, imPoints, rePoints, imPoints);
            getFFTXAxisValues(fftXPoints, rePoints.size());

            QString curveNameRe = fftCurve.curveName;
            QString curveNameIm = fftCurve.curveName;
            curveNameRe.append(" FFT Real");
            curveNameIm.append(" FFT Imag");

            m_curveCommander.add2dCurve(fftCurve.plotName, curveNameRe, fftXPoints, rePoints);
            m_curveCommander.add2dCurve(fftCurve.plotName, curveNameIm, fftXPoints, imPoints);
        }
    }
}

void plotGuiMain::updateFFTPlot(QString srcPlotName, QString srcCurveName)
{
    for(int i = 0; i < m_fftCurves.size(); ++i)
    {
        if( (m_fftCurves[i].srcRePlotName == srcPlotName && m_fftCurves[i].srcReCurveName == srcCurveName) ||
            (m_fftCurves[i].srcImPlotName == srcPlotName && m_fftCurves[i].srcImCurveName == srcCurveName) )
        {
            makeFFTPlot(m_fftCurves[i]);
        }
    }
}

void plotGuiMain::updateFFTList(const tFFTCurve& fftCurve)
{
    bool matchFound = false;
    for(int i = 0; i < m_fftCurves.size(); ++i)
    {
        if(m_fftCurves[i].plotName == fftCurve.plotName && m_fftCurves[i].curveName == fftCurve.curveName)
        {
            m_fftCurves[i] = fftCurve;
            matchFound = true;
            break;
        }
    }
    if(matchFound == false)
    {
        m_fftCurves.push_back(fftCurve);
    }
}

void plotGuiMain::removeFromFFTList(QString plotName, QString curveName)
{
    QVector<tFFTCurve>::Iterator iter = m_fftCurves.begin();
    while(iter != m_fftCurves.end())
    {
        if(iter->plotName == plotName && iter->curveName == curveName)
        {
            iter = m_fftCurves.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void plotGuiMain::removeFromFFTList(QString plotName)
{
    QVector<tFFTCurve>::Iterator iter = m_fftCurves.begin();
    while(iter != m_fftCurves.end())
    {
        if(iter->plotName == plotName)
        {
            iter = m_fftCurves.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

