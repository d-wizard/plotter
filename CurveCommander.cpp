/* Copyright 2013 - 2016 Dan Williams. All Rights Reserved.
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

   for(std::list<ChildCurve*>::iterator iter = m_childCurves.begin(); iter != m_childCurves.end(); ++iter)
   {
      delete *iter;
   }

}

void CurveCommander::curveUpdated(UnpackPlotMsg* plotMsg, CurveData* curveData, bool plotDataWasChanged)
{
   QString plotName(plotMsg->m_plotName.c_str());
   QString curveName(plotMsg->m_curveName.c_str());
   unsigned int updateMsgSize = plotDataWasChanged ? plotMsg->m_yAxisValues.size() : 0;

   curveUpdated( plotName,
                 curveName,
                 curveData,
                 plotMsg->m_sampleStartIndex,
                 updateMsgSize );
}

void CurveCommander::curveUpdated(QString plotName, QString curveName, CurveData* curveData, unsigned int sampleStartIndex, unsigned int numPoints)
{
   bool newCurve = !validCurve(plotName, curveName);
   m_allCurves[plotName].curves[curveName] = curveData;
   m_plotGuiMain->curveUpdated(plotName, curveName);

   if(newCurve)
   {
      // This is a new curve, thus there can't be any child curves from this curve.
      if(m_curvePropGui != NULL)
      {
         m_curvePropGui->updateGuiPlotCurveInfo();
      }
   }
   else if(numPoints > 0)
   {
      notifyChildCurvesOfParentChange(plotName, curveName, sampleStartIndex, numPoints);
   }
   showHidePlotGui(plotName);
}

void CurveCommander::curvePropertyChanged()
{
   if(m_curvePropGui != NULL)
   {
      m_curvePropGui->updateGuiPlotCurveInfo();
   }
}

void CurveCommander::plotRemoved(QString plotName)
{
   bool plotWasRemoved = false;
   tCurveCommanderInfo::iterator iter = m_allCurves.find(plotName);
   if(iter != m_allCurves.end())
   {
      delete iter.value().plotGui;
      m_allCurves.erase(iter);
      plotWasRemoved = true;
   }
   if(plotWasRemoved)
   {
      removeOrphanedChildCurves();

      // Update Curve Properties GUI
      if(m_curvePropGui != NULL)
      {
         m_curvePropGui->updateGuiPlotCurveInfo();
      }
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

MainWindow* CurveCommander::getMainPlot(QString plotName)
{
   if(validPlot(plotName))
   {
      return m_allCurves[plotName].plotGui;
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

void CurveCommander::readPlotMsg(UnpackMultiPlotMsg* plotMsg)
{
   for(std::map<std::string, plotMsgGroup*>::iterator iter = plotMsg->m_plotMsgs.begin(); iter != plotMsg->m_plotMsgs.end(); ++iter)
   {
      plotMsgGroup* group = iter->second;
      QString plotName = iter->first.c_str();
      createPlot(plotName);
      m_allCurves[plotName].plotGui->readPlotMsg(group);
      showHidePlotGui(plotName);
   }
}

void CurveCommander::create1dCurve(QString plotName, QString curveName, ePlotType plotType, dubVect& yPoints)
{
   createPlot(plotName);

   UnpackPlotMsg* plotMsg = new UnpackPlotMsg(NULL, 0);
   plotMsg->m_plotAction = E_CREATE_1D_PLOT;
   plotMsg->m_plotName = plotName.toStdString();
   plotMsg->m_curveName = curveName.toStdString();
   plotMsg->m_plotType = plotType;
   plotMsg->m_yAxisValues = yPoints;
   plotMsgGroup* groupMsg = new plotMsgGroup(plotMsg);
   m_allCurves[plotName].plotGui->readPlotMsg(groupMsg);

   showHidePlotGui(plotName);
}

void CurveCommander::create2dCurve(QString plotName, QString curveName, dubVect& xPoints, dubVect& yPoints)
{
   createPlot(plotName);

   UnpackPlotMsg* plotMsg = new UnpackPlotMsg(NULL, 0);
   plotMsg->m_plotAction = E_CREATE_2D_PLOT;
   plotMsg->m_plotName = plotName.toStdString();
   plotMsg->m_curveName = curveName.toStdString();
   plotMsg->m_plotType = E_PLOT_TYPE_2D;
   plotMsg->m_xAxisValues = xPoints;
   plotMsg->m_yAxisValues = yPoints;
   plotMsgGroup* groupMsg = new plotMsgGroup(plotMsg);
   m_allCurves[plotName].plotGui->readPlotMsg(groupMsg);

   showHidePlotGui(plotName);
}

void CurveCommander::update1dCurve(QString plotName, QString curveName, ePlotType plotType, unsigned int sampleStartIndex, dubVect& yPoints)
{
   createPlot(plotName);

   UnpackPlotMsg* plotMsg = new UnpackPlotMsg(NULL, 0);
   plotMsg->m_plotAction = E_UPDATE_1D_PLOT;
   plotMsg->m_plotName = plotName.toStdString();
   plotMsg->m_curveName = curveName.toStdString();
   plotMsg->m_sampleStartIndex = sampleStartIndex;
   plotMsg->m_plotType = plotType;
   plotMsg->m_yAxisValues = yPoints;
   plotMsgGroup* groupMsg = new plotMsgGroup(plotMsg);
   m_allCurves[plotName].plotGui->readPlotMsg(groupMsg);

   showHidePlotGui(plotName);
}

void CurveCommander::update2dCurve(QString plotName, QString curveName, unsigned int sampleStartIndex, dubVect& xPoints, dubVect& yPoints)
{
   createPlot(plotName);

   UnpackPlotMsg* plotMsg = new UnpackPlotMsg(NULL, 0);
   plotMsg->m_plotAction = E_UPDATE_2D_PLOT;
   plotMsg->m_plotName = plotName.toStdString();
   plotMsg->m_curveName = curveName.toStdString();
   plotMsg->m_sampleStartIndex = sampleStartIndex;
   plotMsg->m_plotType = E_PLOT_TYPE_2D;
   plotMsg->m_xAxisValues = xPoints;
   plotMsg->m_yAxisValues = yPoints;
   plotMsgGroup* groupMsg = new plotMsgGroup(plotMsg);
   m_allCurves[plotName].plotGui->readPlotMsg(groupMsg);

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
         if(m_allCurves[plotName].curves[key]->getHidden() == false)
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


void CurveCommander::createChildCurve(QString plotName, QString curveName, ePlotType plotType, tParentCurveInfo yAxis) // 1D
{
   if(validCurve(plotName, curveName) == false)
   {
      m_childCurves.push_back( new ChildCurve(this, plotName, curveName, plotType, yAxis) );
   }
}

void CurveCommander::createChildCurve(QString plotName, QString curveName, ePlotType plotType, tParentCurveInfo xAxis, tParentCurveInfo yAxis) // 2D
{
   if(validCurve(plotName, curveName) == false)
   {
      m_childCurves.push_back( new ChildCurve(this, plotName, curveName, plotType, xAxis, yAxis) );
   }
}

void CurveCommander::notifyChildCurvesOfParentChange(QString plotName, QString curveName, unsigned int sampleStartIndex, unsigned int numPoints)
{
   std::list<ChildCurve*>::iterator iter = m_childCurves.begin();

   while(iter != m_childCurves.end()) // Iterate over all child curves
   {
      (*iter)->anotherCurveChanged(plotName, curveName, sampleStartIndex, numPoints);
      ++iter;
   }
}

void CurveCommander::removeOrphanedChildCurves()
{
   // Remove any child curves that no longer exists.
   std::list<ChildCurve*>::iterator iter = m_childCurves.begin();
   while(iter != m_childCurves.end()) // Iterate over all child curves
   {
      if(validCurve((*iter)->getPlotName(), (*iter)->getCurveName()) == false)
      {
         // Curve no longer exists, remove from child curve list.
         delete *iter;
         m_childCurves.erase(iter++);
      }
      else
      {
         ++iter;
      }
   }

   // Remove any child curves whose parents no longer exists.
   iter = m_childCurves.begin();
   while(iter != m_childCurves.end()) // Iterate over all child curves
   {
      // Check if the child curves parents exist.
      QVector<tPlotCurveAxis> parents = (*iter)->getParents();
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
         delete *iter;
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
   m_curvePropGui->raise();
}


void CurveCommander::curvePropertiesGuiCloseSlot()
{
   if(m_curvePropGui != NULL)
   {
      delete m_curvePropGui;
      m_curvePropGui = NULL;
   }
}

void CurveCommander::storePlotMsg(const char* msgPtr, unsigned int msgSize, QString& plotName, QString& curveName)
{
   tStoredMsg newMsg;
   newMsg.msgTime = QDateTime::currentDateTime();
   newMsg.name.curve = curveName;
   newMsg.name.plot = plotName;
   newMsg.msgPtr = msgPtr;
   newMsg.msgSize = msgSize;

   const char* msgEndPtr = msgPtr + msgSize;

   QMutexLocker ml(&m_storedMsgsMutex); // lock until end of function.

   std::list<tStoredMsg>::iterator iter = m_storedMsgs.begin();
   while(iter != m_storedMsgs.end()) // Iterate over all stored msgs
   {
      // If start of old message falls within the new message memory
      // it has been overwritten.
      if((*iter).msgPtr >= msgPtr && (*iter).msgPtr < msgEndPtr)
      {
         // Data has been invalided by new msg, remove from list.
         m_storedMsgs.erase(iter++);
      }
      else
      {
         ++iter;
      }
   }
   m_storedMsgs.push_back(newMsg);

}

void CurveCommander::getStoredPlotMsgs(QVector<tStoredMsg>& storedMsgs)
{
   QMutexLocker ml(&m_storedMsgsMutex); // lock until end of function.
   storedMsgs.clear();
   for(std::list<tStoredMsg>::iterator iter = m_storedMsgs.begin(); iter != m_storedMsgs.end(); ++iter)
   {
      storedMsgs.push_front(*iter);
   }
}


void CurveCommander::restorePlotMsg(tStoredMsg msgToRestore, tPlotCurveName plotCurveName)
{
   bool validInput = false;
   {
      QMutexLocker ml(&m_storedMsgsMutex); // lock until end of scope.

      for(std::list<tStoredMsg>::iterator iter = m_storedMsgs.begin(); iter != m_storedMsgs.end(); ++iter)
      {
         if(iter->msgPtr == msgToRestore.msgPtr && iter->msgSize == msgToRestore.msgSize)
         {
            validInput = true;
            break;
         }
      }
   }
   if(validInput)
   {
      m_plotGuiMain->restorePlotMsg(msgToRestore.msgPtr, msgToRestore.msgSize, plotCurveName);
   }
}

void CurveCommander::removeCurve(const QString& plotName, const QString& curveName)
{
   if(validCurve(plotName, curveName))
   {
      m_allCurves[plotName].plotGui->removeCurve(curveName);
      if(m_allCurves[plotName].plotGui->getNumCurves() <= 0)
      {
         // Remove the Plot window the offical way (but without emitting the signal).
         plotWindowCloseSlot(plotName);
      }
      else
      {
         // Plot still exists, just remove curve from map.
         m_allCurves[plotName].curves.remove(curveName);

         removeOrphanedChildCurves();

         // Update Curve Properties GUI
         if(m_curvePropGui != NULL)
         {
            m_curvePropGui->updateGuiPlotCurveInfo();
         }
      }

   }
}

QVector<tPlotCurveAxis> CurveCommander::getCurveParents(const QString& plotName, const QString& curveName)
{
   // Iterate over all child curves. If a match is found, return the parents of the match.
   std::list<ChildCurve*>::iterator iter = m_childCurves.begin();
   while(iter != m_childCurves.end())
   {
      if((*iter)->getPlotName() == plotName && (*iter)->getCurveName() == curveName)
      {
         return (*iter)->getParents();
      }
      else
      {
         ++iter;
      }
   }

   QVector<tPlotCurveAxis> retVal;
   return retVal; // No match found, return empty vector;
}


void CurveCommander::unlinkChildFromParents(const QString& plotName, const QString& curveName)
{
   bool matchFound = false;

   // Remove the child curve that matches the Plot Name and Curve Name specified.
   std::list<ChildCurve*>::iterator iter = m_childCurves.begin();
   while(iter != m_childCurves.end() && matchFound == false) // Iterate over all child curves until match is found.
   {
      if((*iter)->getPlotName() == plotName && (*iter)->getCurveName() == curveName)
      {
         // This is the child curve that needs to be unlinked.
         matchFound = true;
         delete *iter;
         m_childCurves.erase(iter);
      }
      else
      {
         ++iter;
      }
   }
}




