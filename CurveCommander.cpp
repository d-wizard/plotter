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
#include "CurveCommander.h"
#include "plotguimain.h"

CurveCommander::CurveCommander(plotGuiMain *parent):
   m_plotGuiMain(parent)
{
   QObject::connect(this, SIGNAL(plotWindowCloseSignal(QString)),
                    this, SLOT(plotWindowCloseSlot(QString)), Qt::QueuedConnection);
}

CurveCommander::~CurveCommander()
{
   destroyAllPlots();
}


void CurveCommander::curveUpdated(QString plotName, QString curveName, CurveData* curveData)
{
    m_allCurves[plotName].curves[curveName] = curveData;
    m_plotGuiMain->curveUpdated(plotName, curveName);
}

void CurveCommander::plotRemoved(QString plotName)
{
   tCurveCommanderInfo::iterator iter = m_allCurves.find(plotName);
   if(iter != m_allCurves.end())
   {
      delete iter.value().plotGui;
      m_allCurves.erase(iter);
   }
}


bool CurveCommander::validPlot(QString plotName)
{
    return m_allCurves.find(plotName) != m_allCurves.end();
}

bool CurveCommander::validCurve(QString plotName, QString curveName)
{
    if(validPlot(plotName) == true)
    {
        return m_allCurves[plotName].curves.find(curveName) != m_allCurves[plotName].curves.end();
    }
    else
    {
        return false;
    }
}

CurveData* CurveCommander::getCurveData(QString plotName, QString curveName)
{
    if(validCurve(plotName, curveName) == true)
    {
        return m_allCurves[plotName].curves[curveName];
    }
    else
    {
        return NULL;
    }
}

tCurveCommanderInfo& CurveCommander::getCurveCommanderInfo()
{
    return m_allCurves;
}

void CurveCommander::createPlot(QString plotName)
{
   if(validPlot(plotName) == false)
   {
      m_allCurves[plotName].plotGui = new MainWindow(this, m_plotGuiMain);
      m_allCurves[plotName].plotGui->setWindowTitle(plotName);
   }
}

void CurveCommander::destroyAllPlots()
{
   tCurveCommanderInfo::iterator iter = m_allCurves.begin();
   while(iter != m_allCurves.end())
   {
      delete iter.value().plotGui;
      iter = m_allCurves.erase(iter);
   }
}

void CurveCommander::add1dCurve(QString plotName, QString curveName, dubVect yPoints)
{
   createPlot(plotName);
   m_allCurves[plotName].plotGui->add1dCurve(curveName, yPoints);
   m_allCurves[plotName].plotGui->show();
}

void CurveCommander::add2dCurve(QString plotName, QString curveName, dubVect xPoints, dubVect yPoints)
{
   createPlot(plotName);
   m_allCurves[plotName].plotGui->add2dCurve(curveName, xPoints, yPoints);
   m_allCurves[plotName].plotGui->show();
}


void CurveCommander::plotWindowCloseSlot(QString plotName)
{
    plotRemoved(plotName);
    m_plotGuiMain->plotWindowClose(plotName);
}


