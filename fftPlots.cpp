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
#include "fftPlots.h"
#include "plotguimain.h"
#include "fftHelper.h"

fftPlots::fftPlots(plotGuiMain *plotGuiParent):
   m_plotGuiMain(plotGuiParent),
   m_createFFTPlotGUI(NULL)
{
   QObject::connect(this, SIGNAL(createFftGuiFinishedSignal()),
                    this, SLOT(createFftGuiFinishedSlot()), Qt::QueuedConnection);
}

fftPlots::~fftPlots()
{
   if(m_createFFTPlotGUI != NULL)
   {
       delete m_createFFTPlotGUI;
   }
}

CurveCommander& fftPlots::getCurveCommander()
{
   return m_plotGuiMain->getCurveCommander();
}

void fftPlots::showCreateFftGui(eFFTType fftType)
{
   if(m_createFFTPlotGUI == NULL)
   {
       m_createFFTPlotGUI = new createFFTPlot(this, getCurveCommander().getCurveCommanderInfo(), fftType);
   }
}

void fftPlots::plotChanged()
{
   if(m_createFFTPlotGUI != NULL)
   {
       m_createFFTPlotGUI->plotsCurvesChanged(getCurveCommander().getCurveCommanderInfo());
   }
}

void fftPlots::createFftGuiFinishedSlot()
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

void fftPlots::makeFFTPlot(tFFTCurve fftCurve)
{
    dubVect rePoints;
    dubVect imPoints;

    CurveData* srcRe = getCurveCommander().getCurveData(fftCurve.srcRePlotName, fftCurve.srcReCurveName);
    CurveData* srcIm = getCurveCommander().getCurveData(fftCurve.srcImPlotName, fftCurve.srcImCurveName);

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
            getCurveCommander().create1dCurve(fftCurve.plotName, fftCurve.curveName, rePoints);
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

            getCurveCommander().create2dCurve(fftCurve.plotName, curveNameRe, fftXPoints, rePoints);
            getCurveCommander().create2dCurve(fftCurve.plotName, curveNameIm, fftXPoints, imPoints);
        }
    }
}

void fftPlots::curveChanged(QString srcPlotName, QString srcCurveName)
{
    for(int i = 0; i < m_fftCurves.size(); ++i)
    {
        if( (m_fftCurves[i].srcRePlotName == srcPlotName && m_fftCurves[i].srcReCurveName == srcCurveName) ||
            (m_fftCurves[i].srcImPlotName == srcPlotName && m_fftCurves[i].srcImCurveName == srcCurveName) )
        {
            makeFFTPlot(m_fftCurves[i]);
        }
    }
    plotChanged();
}

void fftPlots::updateFFTList(const tFFTCurve& fftCurve)
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

void fftPlots::removeFromFFTList(QString plotName)
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

