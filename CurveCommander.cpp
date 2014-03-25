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
#include "CurveCommander.h"
#include "plotguimain.h"
#include "curveproperties.h"
#include "ChildCurves.h"

CurveCommander::CurveCommander(plotGuiMain *parent):
   m_plotGuiMain(parent),
   m_curvePropGui(NULL)
{
   QObject::connect(this, SIGNAL(plotWindowCloseSignal(QString)),
                    this, SLOT(plotWindowCloseSlot(QString)), Qt::QueuedConnection);
   QObject::connect(this, SIGNAL(curvePropertiesGuiCloseSignal()),
                    this, SLOT(curvePropertiesGuiCloseSlot()), Qt::QueuedConnection);
}

CurveCommander::~CurveCommander()
{
   curvePropertiesGuiCloseSlot(); // Close Curve Properties GUI (if it exists)
   destroyAllPlots();
}


void CurveCommander::curveUpdated(QString plotName, QString curveName, CurveData* curveData)
{
   bool newCurve = !validCurve(plotName, curveName);
   m_allCurves[plotName].curves[curveName] = curveData;
   m_plotGuiMain->curveUpdated(plotName, curveName);

   if(newCurve)
   {
      if(m_curvePropGui != NULL)
      {
         m_curvePropGui->setCreateChildComboBoxes();
      }
   }
   else
   {
      notifyChildCurvesOfParentChange(plotName, curveName);
   }
}

void CurveCommander::plotRemoved(QString plotName)
{
   tCurveCommanderInfo::iterator iter = m_allCurves.find(plotName);
   if(iter != m_allCurves.end())
   {
      delete iter.value().plotGui;
      m_allCurves.erase(iter);
   }
   removeOrphanedChildCurves();
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

void CurveCommander::create1dCurve(QString plotName, QString curveName, ePlotType plotType, dubVect yPoints)
{
   createPlot(plotName);
   m_allCurves[plotName].plotGui->create1dCurve(curveName, plotType, yPoints);
   showHidePlotGui(plotName);
}

void CurveCommander::create2dCurve(QString plotName, QString curveName, dubVect xPoints, dubVect yPoints)
{
   createPlot(plotName);
   m_allCurves[plotName].plotGui->create2dCurve(curveName, xPoints, yPoints);
   showHidePlotGui(plotName);
}

void CurveCommander::update1dCurve(QString plotName, QString curveName, ePlotType plotType, unsigned int sampleStartIndex, dubVect yPoints)
{
   createPlot(plotName);
   m_allCurves[plotName].plotGui->update1dCurve(curveName, sampleStartIndex, plotType, yPoints);
   showHidePlotGui(plotName);
}

void CurveCommander::update2dCurve(QString plotName, QString curveName, unsigned int sampleStartIndex, dubVect xPoints, dubVect yPoints)
{
   createPlot(plotName);
   m_allCurves[plotName].plotGui->update2dCurve(curveName, sampleStartIndex, xPoints, yPoints);
   showHidePlotGui(plotName);
}

void CurveCommander::plotWindowCloseSlot(QString plotName)
{
    plotRemoved(plotName);
    m_plotGuiMain->plotWindowClose(plotName);
}

void CurveCommander::showHidePlotGui(QString plotName)
{
   bool allHidden = true;
   if(validPlot(plotName) == true)
   {
      foreach( QString key, m_allCurves[plotName].curves.keys() )
      {
         if(m_allCurves[plotName].curves[key]->hidden == false)
         {
            allHidden = false;
            break;
         }
      }

      if(allHidden == false)
      {
         m_allCurves[plotName].plotGui->show();
      }
      else
      {
         m_allCurves[plotName].plotGui->hide();
      }

   }
}


void CurveCommander::createChildCurve(QString plotName, QString curveName, ePlotType plotType, tParentCurveAxis yAxis) // 1D
{
   if(validCurve(plotName, curveName) == false)
   {
      m_childCurves.push_back(QSharedPointer<ChildCurve>( new ChildCurve(this, plotName, curveName, plotType, yAxis) ));
   }
}

void CurveCommander::createChildCurve(QString plotName, QString curveName, ePlotType plotType, tParentCurveAxis xAxis, tParentCurveAxis yAxis) // 2D
{
   if(validCurve(plotName, curveName) == false)
   {
      m_childCurves.push_back(QSharedPointer<ChildCurve>( new ChildCurve(this, plotName, curveName, plotType, xAxis, yAxis)) );
   }
}

void CurveCommander::notifyChildCurvesOfParentChange(QString plotName, QString curveName)
{
   QList<QSharedPointer<ChildCurve> >::iterator iter = m_childCurves.begin();

   while(iter != m_childCurves.end()) // Iterate over all child curves
   {
      (*iter)->anotherCurveChanged(plotName, curveName);
      ++iter;
   }
}

void CurveCommander::removeOrphanedChildCurves()
{
   QList<QSharedPointer<ChildCurve> >::iterator iter = m_childCurves.begin();

   while(iter != m_childCurves.end()) // Iterate over all child curves
   {
      // Check if the child curves parents exist.
      QVector<tParentCurveAxis> parents = (*iter)->getParents();
      bool parentExists = false;
      for(int i = 0; i < parents.size(); ++i)
      {
         if(validCurve(parents[i].plotName, parents[i].curveName))
         {
            parentExists = true;
            break;
         }
      }

      if(parentExists == false)
      {
         // Parents have been removed, remove from child curve list.
         m_childCurves.erase(iter++);
      }
      else
      {
         ++iter;
      }
   }
}

void CurveCommander::showCurvePropertiesGui(QString plotName, QString curveName)
{
   if(m_curvePropGui == NULL)
   {
      m_curvePropGui = new curveProperties(this, plotName, curveName);
   }
   m_curvePropGui->show();
}


void CurveCommander::curvePropertiesGuiCloseSlot()
{
   if(m_curvePropGui != NULL)
   {
      delete m_curvePropGui;
      m_curvePropGui = NULL;
   }
}

