/* Copyright 2013 - 2017, 2019, 2021 - 2022 Dan Williams. All Rights Reserved.
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
#include "createplotfromdata.h"
#include "ChildCurves.h"
#include "spectrumAnalyzerModeTypes.h"
#include "curveStatsChildParam.h"

#define MAX_NUM_STORED_CURVES (1000)

CurveCommander::CurveCommander(plotGuiMain *parent):
   m_plotGuiMain(parent),
   m_curvePropGui(NULL),
   m_createPlotFromDataGui(NULL)
{
   QObject::connect(this, SIGNAL(plotWindowCloseSignal(QString)),
                    this, SLOT(plotWindowCloseSlot(QString)), Qt::QueuedConnection);
   QObject::connect(this, SIGNAL(curvePropertiesGuiCloseSignal()),
                    this, SLOT(curvePropertiesGuiCloseSlot()), Qt::QueuedConnection);
   QObject::connect(this, SIGNAL(createPlotFromDataGuiCloseSignal()),
                    this, SLOT(createPlotFromDataGuiCloseSlot()), Qt::QueuedConnection);
}

CurveCommander::~CurveCommander()
{
   curvePropertiesGuiCloseSlot(); // Close Curve Properties GUI (if it exists)
   createPlotFromDataGuiCloseSlot(); // Close Create Plot From Data GUI (if it exists)
   destroyAllPlots();

   for(std::list<ChildCurve*>::iterator iter = m_childCurves.begin(); iter != m_childCurves.end(); ++iter)
   {
      delete *iter;
   }

}

void CurveCommander::curveUpdated( plotMsgGroup* groupMsg,
                                   UnpackPlotMsg* plotMsg,
                                   CurveData* curveData,
                                   bool plotDataWasChanged )
{
   QString plotName(plotMsg->m_plotName.c_str());
   QString curveName(plotMsg->m_curveName.c_str());
   unsigned int updateMsgSize = plotDataWasChanged ? plotMsg->m_yAxisValues.size() : 0;

   m_childPlots_mutex.lock();
   childPlots_addParentMsgIdToProcessedList(plotMsg->m_plotMsgID);
   m_childPlots_mutex.unlock();

   curveUpdated( plotName,
                 curveName,
                 curveData,
                 plotMsg->m_sampleStartIndex,
                 updateMsgSize,
                 groupMsg != NULL ? groupMsg->m_groupMsgId : PLOT_MSG_ID_TYPE_NO_PARENT_MSG,
                 plotMsg->m_plotMsgID );

   m_childPlots_mutex.lock();
   childPlots_plot(plotMsg->m_plotMsgID);
   m_childPlots_mutex.unlock();
}

void CurveCommander::curveUpdated( QString plotName,
                                   QString curveName,
                                   CurveData* curveData,
                                   unsigned int sampleStartIndex,
                                   unsigned int numPoints,
                                   PlotMsgIdType parentGroupMsgId,
                                   PlotMsgIdType parentCurveMsgId )
{
   if(curveData != NULL)
   {
      bool newCurve = !validCurve(plotName, curveName);
      m_allCurves[plotName].curves[curveName] = curveData;

      if(m_curvePropGui != NULL)
      {
         if(newCurve)
         {
            m_curvePropGui->updateGuiPlotCurveInfo();
         }
         else
         {
            m_curvePropGui->existingPlotsChanged();
         }
      }
      if(m_createPlotFromDataGui != NULL)
      {
         m_createPlotFromDataGui->updateGuiPlotCurveInfo();
      }

      // New curves can't have children, so only check for children if this isn't a new curve.
      if(newCurve == false && numPoints > 0)
      {
         notifyChildCurvesOfParentChange(plotName, curveName, sampleStartIndex, numPoints, parentGroupMsgId, parentCurveMsgId);
      }
   }
   showHidePlotGui(plotName);
}


void CurveCommander::plotMsgGroupRemovedWithoutBeingProcessed(plotMsgGroup* plotMsgGroup)
{
   if(plotMsgGroup != NULL && plotMsgGroup->m_plotMsgs.size() > 0)
   {
      // The plot messages in the plot message group haven't been processed the normal way.
      // Here is where any processing should be done.
      m_childPlots_mutex.lock();

      // No processing is needed. Just add the messages to the processed list.
      childPlots_addMsgGroupToParentMsgIdProcessedList(plotMsgGroup);

      // All plot message in the plotMsgGroup will be in the same tParentMsgIdGroup list. So we only
      // need to call this function once with any one of the m_plotMsgs's m_plotMsgID's.
      childPlots_plot((*plotMsgGroup->m_plotMsgs.begin())->m_plotMsgID);
      m_childPlots_mutex.unlock();
   }

}

void CurveCommander::curvePropertyChanged()
{
   if(m_curvePropGui != NULL)
   {
      m_curvePropGui->updateGuiPlotCurveInfo();
   }
   if(m_createPlotFromDataGui != NULL)
   {
      m_createPlotFromDataGui->updateGuiPlotCurveInfo();
   }
}

void CurveCommander::plotRemoved(QString plotName)
{
   bool plotWasRemoved = false;
   tCurveCommanderInfo::iterator iter = m_allCurves.find(plotName);
   if(iter != m_allCurves.end())
   {
      iter.value().plotGui->closeSubWindows();
      delete iter.value().plotGui;
      m_allCurves.erase(iter);
      plotWasRemoved = true;
   }
   if(plotWasRemoved)
   {
      removeOrphanedChildCurves();
      curveStatsChildParam_plotRemoved(plotName);

      // Update Curve Properties GUI
      if(m_curvePropGui != NULL)
      {
         m_curvePropGui->updateGuiPlotCurveInfo();
      }
      if(m_createPlotFromDataGui != NULL)
      {
         m_createPlotFromDataGui->updateGuiPlotCurveInfo();
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

void CurveCommander::createPlot(QString plotName, bool startInScrollMode)
{
   if(validPlot(plotName) == false)
   {
      m_allCurves[plotName].plotGui = new MainWindow(this, m_plotGuiMain, plotName, startInScrollMode);
   }
}

void CurveCommander::destroyAllPlots()
{
   tCurveCommanderInfo::iterator iter = m_allCurves.begin();
   while(iter != m_allCurves.end())
   {
      plotWindowCloseSlot(iter.key()); // Cleans everything up and removes the instance from m_allCurves.
      iter = m_allCurves.begin(); // Set iter to the next plot to destroy.
   }
}

void CurveCommander::readPlotMsg(UnpackMultiPlotMsg* plotMsg)
{
   childPlots_createParentMsgIdGroup(plotMsg);
   for(std::map<std::string, plotMsgGroup*>::iterator allMsgs = plotMsg->m_plotMsgs.begin(); allMsgs != plotMsg->m_plotMsgs.end(); ++allMsgs)
   {
      plotMsgGroup* group = allMsgs->second;
      QString plotName = allMsgs->first.c_str();

      if(!m_ipBlocker.isInBlockList(plotMsg->m_msgSourceIpAddr, plotName))
      {
         // IP Address / Plot Name is not blocked. Create the plot.
         createPlot(plotName);
         m_allCurves[plotName].plotGui->readPlotMsg(group);
         showHidePlotGui(plotName);

         // If we are in "Spectrum Analyzer Mode", this function will check if any Child Plots should be created / updated.
         configForSpectrumAnalyzerView(plotName);
      }
      else
      {
         // IP Address / Plot Name are blocked. Delete the plot message group.
         delete group;
      }

   }
}

void CurveCommander::create1dCurve(QString plotName, QString curveName, ePlotType plotType, dubVect& yPoints, tCurveMathProperties* mathProps)
{
   createPlot(plotName);

   UnpackPlotMsg* plotMsg = new UnpackPlotMsg();
   plotMsg->m_plotAction = E_CREATE_1D_PLOT;
   plotMsg->m_plotName = plotName.toStdString();
   plotMsg->m_curveName = curveName.toStdString();
   plotMsg->m_plotType = plotType;
   plotMsg->m_yAxisValues = yPoints;
   if(mathProps != NULL)
   {
      plotMsg->m_useCurveMathProps = true;
      plotMsg->m_curveMathProps = *mathProps;
   }

   plotMsgGroup* groupMsg = new plotMsgGroup(plotMsg);
   childPlots_createParentMsgIdGroup(groupMsg);
   m_allCurves[plotName].plotGui->readPlotMsg(groupMsg);

   showHidePlotGui(plotName);
}

void CurveCommander::create2dCurve(QString plotName, QString curveName, dubVect& xPoints, dubVect& yPoints, tCurveMathProperties* mathProps)
{
   createPlot(plotName);

   UnpackPlotMsg* plotMsg = new UnpackPlotMsg();
   plotMsg->m_plotAction = E_CREATE_2D_PLOT;
   plotMsg->m_plotName = plotName.toStdString();
   plotMsg->m_curveName = curveName.toStdString();
   plotMsg->m_plotType = E_PLOT_TYPE_2D;
   plotMsg->m_xAxisValues = xPoints;
   plotMsg->m_yAxisValues = yPoints;
   if(mathProps != NULL)
   {
      plotMsg->m_useCurveMathProps = true;
      plotMsg->m_curveMathProps = *mathProps;
   }

   plotMsgGroup* groupMsg = new plotMsgGroup(plotMsg);
   childPlots_createParentMsgIdGroup(groupMsg);
   m_allCurves[plotName].plotGui->readPlotMsg(groupMsg);

   showHidePlotGui(plotName);
}

void CurveCommander::update1dChildCurve( QString& plotName, 
                                         QString& curveName, 
                                         ePlotType plotType, 
                                         unsigned int sampleStartIndex, 
                                         dubVect& yPoints, 
                                         PlotMsgIdType parentMsgId,
                                         bool startInScrollMode )
{
   createPlot(plotName, startInScrollMode);

   UnpackPlotMsg* plotMsg = new UnpackPlotMsg(); // Will be deleted when it is processed in mainwindow.cpp
   plotMsg->m_plotAction = E_UPDATE_1D_PLOT;
   plotMsg->m_plotName = plotName.toStdString();
   plotMsg->m_curveName = curveName.toStdString();
   plotMsg->m_sampleStartIndex = sampleStartIndex;
   plotMsg->m_plotType = plotType;
   plotMsg->m_yAxisValues = yPoints;

   if(parentMsgId == PLOT_MSG_ID_TYPE_NO_PARENT_MSG)
   {
      plotMsgGroup* groupMsg = new plotMsgGroup(plotMsg);
      childPlots_createParentMsgIdGroup(groupMsg);
      m_allCurves[plotName].plotGui->readPlotMsg(groupMsg);
      showHidePlotGui(plotName);
   }
   else
   {
      // Don't process right now, queue up the message so it can be processed when all child plot messages have been
      // generated for a particular parent plot message group.
      tChildAndParentID childAndParentID;
      childAndParentID.parentMsgID = parentMsgId;
      childAndParentID.childMsg = plotMsg;
      childPlots_addChildUpdateToList(childAndParentID);
   }
}

void CurveCommander::update2dChildCurve( QString& plotName, 
                                         QString& curveName, 
                                         unsigned int sampleStartIndex, 
                                         dubVect& xPoints, 
                                         dubVect& yPoints, 
                                         PlotMsgIdType parentMsgId,
                                         bool startInScrollMode )
{
   createPlot(plotName, startInScrollMode);

   UnpackPlotMsg* plotMsg = new UnpackPlotMsg(); // Will be deleted when it is processed in mainwindow.cpp
   plotMsg->m_plotAction = E_UPDATE_2D_PLOT;
   plotMsg->m_plotName = plotName.toStdString();
   plotMsg->m_curveName = curveName.toStdString();
   plotMsg->m_sampleStartIndex = sampleStartIndex;
   plotMsg->m_plotType = E_PLOT_TYPE_2D;
   plotMsg->m_xAxisValues = xPoints;
   plotMsg->m_yAxisValues = yPoints;

   if(parentMsgId == PLOT_MSG_ID_TYPE_NO_PARENT_MSG)
   {
      plotMsgGroup* groupMsg = new plotMsgGroup(plotMsg);
      childPlots_createParentMsgIdGroup(groupMsg);
      m_allCurves[plotName].plotGui->readPlotMsg(groupMsg);
      showHidePlotGui(plotName);
   }
   else
   {
      // Don't process right now, queue up the message so it can be processed when all child plot messages have been
      // generated for a particular parent plot message group.
      tChildAndParentID childAndParentID;
      childAndParentID.parentMsgID = parentMsgId;
      childAndParentID.childMsg = plotMsg;
      childPlots_addChildUpdateToList(childAndParentID);
   }
}

void CurveCommander::plotWindowCloseSlot(QString plotName)
{
    plotRemoved(plotName);
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


void CurveCommander::createChildCurve( QString plotName, 
                                       QString curveName, 
                                       ePlotType plotType, 
                                       bool forceContiguousParentPoints,
                                       bool startChildInScrollMode,
                                       tParentCurveInfo yAxis ) // 1D
{
   if(validCurve(plotName, curveName) == false)
   {
      m_childCurves.push_back( new ChildCurve(this, plotName, curveName, plotType, forceContiguousParentPoints, startChildInScrollMode, yAxis) );
   }
}

void CurveCommander::createChildCurve( QString plotName, 
                                       QString curveName, 
                                       ePlotType plotType, 
                                       bool forceContiguousParentPoints,
                                       bool startChildInScrollMode,
                                       tParentCurveInfo xAxis, 
                                       tParentCurveInfo yAxis ) // 2D
{
   if(validCurve(plotName, curveName) == false)
   {
      m_childCurves.push_back( new ChildCurve(this, plotName, curveName, plotType, forceContiguousParentPoints, startChildInScrollMode, xAxis, yAxis) );
   }
}

void CurveCommander::notifyChildCurvesOfParentChange( QString plotName, 
                                                      QString curveName, 
                                                      unsigned int sampleStartIndex, 
                                                      unsigned int numPoints,
                                                      PlotMsgIdType parentGroupMsgId,
                                                      PlotMsgIdType parentCurveMsgId )
{
   std::list<ChildCurve*>::iterator iter = m_childCurves.begin();

   while(iter != m_childCurves.end()) // Iterate over all child curves
   {
      (*iter)->anotherCurveChanged( plotName, 
                                    curveName, 
                                    sampleStartIndex, 
                                    numPoints, 
                                    parentGroupMsgId, 
                                    parentCurveMsgId );
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
   else
   {
      // Change all plot/curve GUI elements to point to this plot/curve.
      m_curvePropGui->updateGuiPlotCurveInfo(plotName, curveName);
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

void CurveCommander::showCreatePlotFromDataGui(QString plotName, const char* dataToPlot)
{
   if(m_createPlotFromDataGui == NULL)
   {
      m_createPlotFromDataGui = new createPlotFromData(this, plotName, dataToPlot);
   }
   else
   {
      m_createPlotFromDataGui->handleNewData(dataToPlot);
   }
   m_createPlotFromDataGui->show();
   m_createPlotFromDataGui->raise();
}

void CurveCommander::createPlotFromDataGuiCloseSlot()
{
   if(m_createPlotFromDataGui != NULL)
   {
      delete m_createPlotFromDataGui;
      m_createPlotFromDataGui = NULL;
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

   while(m_storedMsgs.size() > MAX_NUM_STORED_CURVES)
   {
      m_storedMsgs.pop_front();
   }
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
         if(m_createPlotFromDataGui != NULL)
         {
            m_createPlotFromDataGui->updateGuiPlotCurveInfo();
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

void CurveCommander::doFinalChildCurveInit(const QString& plotName, const QString& curveName)
{
   bool matchFound = false;

   // Search for the child curve that matches the Plot Name and Curve Name specified.
   std::list<ChildCurve*>::iterator iter = m_childCurves.begin();
   while(iter != m_childCurves.end() && matchFound == false) // Iterate over all child curves until match is found.
   {
      if((*iter)->getPlotName() == plotName && (*iter)->getCurveName() == curveName)
      {
         // This is the child curve that the input plot name / curve name are refering to.
         matchFound = true;

         ///////////////////////////////////////////////////
         // Do the Final Child Curve Initialization.
         ///////////////////////////////////////////////////
         (*iter)->setToParentsSampleRate();
      }
      else
      {
         ++iter;
      }
   }
}

void CurveCommander::childPlots_createParentMsgIdGroup(plotMsgGroup* group)
{
   m_childPlots_mutex.lock();
   
   tParentMsgIdGroup parentIds;
   // Cycle through all the individual plot messages and grab the Plot Message ID.
   for(UnpackPlotMsgPtrList::iterator plotMsgs = group->m_plotMsgs.begin(); plotMsgs != group->m_plotMsgs.end(); ++plotMsgs)
   {
      parentIds.push_back((*plotMsgs)->m_plotMsgID);
   }
   m_parentMsgIdGroups.push_back(parentIds);
   
   m_childPlots_mutex.unlock();
}

void CurveCommander::childPlots_createParentMsgIdGroup(UnpackMultiPlotMsg* plotMsg)
{
   m_childPlots_mutex.lock();
   
   tParentMsgIdGroup parentIds;
   // Cycle through all the individual plot messages and grab the Plot Message ID.
   for(std::map<std::string, plotMsgGroup*>::iterator allMsgs = plotMsg->m_plotMsgs.begin(); allMsgs != plotMsg->m_plotMsgs.end(); ++allMsgs)
   {
      plotMsgGroup* group = allMsgs->second;
      for(UnpackPlotMsgPtrList::iterator plotMsgs = group->m_plotMsgs.begin(); plotMsgs != group->m_plotMsgs.end(); ++plotMsgs)
      {
         parentIds.push_back((*plotMsgs)->m_plotMsgID);
      }
   }
   if(parentIds.size() > 0)
   {
      m_parentMsgIdGroups.push_back(parentIds);
   }

   m_childPlots_mutex.unlock();
}

std::list<tParentMsgIdGroup>::iterator CurveCommander::childPlots_getParentMsgIdGroupIter(PlotMsgIdType parentMsgID)
{
   for(std::list<tParentMsgIdGroup>::iterator i = m_parentMsgIdGroups.begin(); i != m_parentMsgIdGroups.end(); ++i)
   {
      for(tParentMsgIdGroup::iterator j = i->begin(); j != i->end(); ++j)
      {
         if((*j) == parentMsgID)
         {
            return i;
         }
      }
   }

   return m_parentMsgIdGroups.end();
}

bool CurveCommander::childPlots_haveAllMsgsBeenProcessed(PlotMsgIdType parentMsgID)
{
   std::list<tParentMsgIdGroup>::iterator msgGroupIter = childPlots_getParentMsgIdGroupIter(parentMsgID);
   if(msgGroupIter != m_parentMsgIdGroups.end())
   {
      for(tParentMsgIdGroup::iterator curParentMsgId = msgGroupIter->begin(); curParentMsgId != msgGroupIter->end(); ++curParentMsgId)
      {
         bool foundMatch = false;
         for( tParentMsgIdGroup::iterator processedParentMsgId = m_processedParentMsgIDs.begin();
              processedParentMsgId != m_processedParentMsgIDs.end();
              ++processedParentMsgId )
         {
            if((*processedParentMsgId) == (*curParentMsgId))
            {
               foundMatch = true;
               break;
            }
         }
         if(foundMatch == false)
         {
            return false;
         }
      }
      return true;
   }
   else
   {
      return false;
   }
}

void CurveCommander::childPlots_eraseParentIdsFromProcessedList(std::list<tParentMsgIdGroup>::iterator& parentMsgIdGroup)
{
   for(tParentMsgIdGroup::iterator parentMsgIdFromGroup = parentMsgIdGroup->begin(); parentMsgIdFromGroup != parentMsgIdGroup->end(); ++parentMsgIdFromGroup)
   {
      tParentMsgIdGroup::iterator parentMsgIdInProcessedList = m_processedParentMsgIDs.begin();
      while(parentMsgIdInProcessedList != m_processedParentMsgIDs.end())
      {
         if( (*parentMsgIdInProcessedList) == (*parentMsgIdFromGroup) )
         {
            m_processedParentMsgIDs.erase(parentMsgIdInProcessedList++);
         }
         else
         {
            ++parentMsgIdInProcessedList;
         }
      }
   }
}
void CurveCommander::childPlots_addParentMsgIdToProcessedList(PlotMsgIdType parentID)
{
   m_processedParentMsgIDs.push_back(parentID);
}

void CurveCommander::childPlots_addMsgGroupToParentMsgIdProcessedList(plotMsgGroup* plotMsgGroup)
{
   if(plotMsgGroup != NULL && plotMsgGroup->m_plotMsgs.size() > 0)
   {
      // Loop through all the messages in the message group.
      for(UnpackPlotMsgPtrList::iterator plotMsgs = plotMsgGroup->m_plotMsgs.begin(); plotMsgs != plotMsgGroup->m_plotMsgs.end(); ++plotMsgs)for(unsigned int i = 0; i < plotMsgGroup->m_plotMsgs.size(); ++i)
      {
         childPlots_addParentMsgIdToProcessedList((*plotMsgs)->m_plotMsgID);
      }
   }
}

void CurveCommander::childPlots_addChildUpdateToList(tChildAndParentID childAndParentID)
{
   m_childPlots_mutex.lock();
   m_queuedChildCurveMsgs.push_back(childAndParentID);
   m_childPlots_mutex.unlock();
}

void CurveCommander::childPlots_removeDuplicateChildData(UnpackMultiPlotMsg* multiPlotMsg)
{
   // Loop through all the plotMsgGroup's with unique Plot Names.
   for(std::map<std::string, plotMsgGroup*>::iterator iter = multiPlotMsg->m_plotMsgs.begin(); iter != multiPlotMsg->m_plotMsgs.end(); ++iter)
   {
      plotMsgGroup* group = iter->second;

      // Loop through the grouped message for a unique Plot Name.
      // Remove messages that contain duplicate data (when a match is found
      // remove the 1st of the 2 entries).
      UnpackPlotMsgPtrList::iterator mainIter = group->m_plotMsgs.begin();
      while(mainIter != group->m_plotMsgs.end())
      {
         UnpackPlotMsg* mainPtr = (*mainIter);
         bool duplicateChildDataFound = false;

         UnpackPlotMsgPtrList::iterator secondaryIter = mainIter;
         secondaryIter++;
         while(secondaryIter != group->m_plotMsgs.end())
         {
            UnpackPlotMsg* secendaryPtr = (*secondaryIter);

            // Check if the 2 entries are duplicates (i.e. they both attempting to update
            // the exact same memory of a curve).
            if( mainPtr->m_curveName          == secendaryPtr->m_curveName &&
                mainPtr->m_sampleStartIndex   == secendaryPtr->m_sampleStartIndex &&
                mainPtr->m_plotType           == secendaryPtr->m_plotType && 
                mainPtr->m_xAxisValues.size() == secendaryPtr->m_xAxisValues.size() && 
                mainPtr->m_yAxisValues.size() == secendaryPtr->m_yAxisValues.size() )
            {
               duplicateChildDataFound = true;
               break;
            }
            else
            {
               ++secondaryIter;
            }
         }
         if(duplicateChildDataFound)
         {
            // This UnpackPlotMsg will never be processed, delete it now and erase it from the list.
            delete mainPtr;
            group->m_plotMsgs.erase(mainIter++);
         }
         else
         {
            ++mainIter;
         }
      }
   }
}

void CurveCommander::childPlots_plot(PlotMsgIdType parentID)
{
   // Note: It is expected that the Child Plot Mutex is locked before this function is called.
   if(childPlots_haveAllMsgsBeenProcessed(parentID))
   {
      // All the parent messages in a message group have been processed. Now we can send all the queued
      // child messages that have parents in the parent message group.
      std::list<tParentMsgIdGroup>::iterator parentMsgGroupIter = childPlots_getParentMsgIdGroupIter(parentID);
      if(parentMsgGroupIter != m_parentMsgIdGroups.end())
      {
         UnpackMultiPlotMsg multiChildPlotMsg;// This will group all child plots from a particular parent plot message group into groups based on the child plot names.

         // Loop through all the Parent Msg IDs in the group.
         for(tParentMsgIdGroup::iterator curParentMsgId = parentMsgGroupIter->begin(); curParentMsgId != parentMsgGroupIter->end(); ++curParentMsgId)
         {
            // Loop through all queued child plot messages to find all the queue child curve that have parents in this message group.
            // While looping through, erase members from queued list that will be processed now.
            std::list<tChildAndParentID>::iterator queuedChilds = m_queuedChildCurveMsgs.begin();
            while(queuedChilds != m_queuedChildCurveMsgs.end())
            {
               if(queuedChilds->parentMsgID == (*curParentMsgId))
               {
                  // This queued child curve is in the parent group. Add it to the multi plot message, and remove it from the queued list.
                  plotMsgGroup* childPlotMsgGroup = multiChildPlotMsg.getPlotMsgGroup(queuedChilds->childMsg->m_plotName);
                  childPlotMsgGroup->m_plotMsgs.push_back(queuedChilds->childMsg);
                  m_queuedChildCurveMsgs.erase(queuedChilds++);
               }
               else
               {
                  ++queuedChilds;
               }
            }
         }

         childPlots_removeDuplicateChildData(&multiChildPlotMsg);
         if(multiChildPlotMsg.m_plotMsgs.size() > 0)
         {
            // It is expected that the mutex is locked before this function is called.
            // But if child curves are going to be plotted, the mutex needs to be
            // unlocked to send the child plot messages to the mainwindow GUI.
            m_childPlots_mutex.unlock();

            readPlotMsg(&multiChildPlotMsg);

            m_childPlots_mutex.lock();
         }

         // Now that all the child plot messages that were spawned from the parent message group have been handled,
         // we can erase the parent message IDs from the list of parent messages that have been processed by their
         // main plot windows.
         childPlots_eraseParentIdsFromProcessedList(parentMsgGroupIter);

         // We are done processing this parent message group, erase it from the list.
         m_parentMsgIdGroups.erase(parentMsgGroupIter);

      } // End if(parentMsgGroupIter != m_parentMsgIdGroups.end())

   } // End if(childPlots_haveAllMsgsBeenProcessed(parentID))

#ifdef CHILD_MSG_GROUPING_DEBUG
   childPlots_debugPrint();
#endif
}

#ifdef CHILD_MSG_GROUPING_DEBUG
void CurveCommander::childPlots_debugPrint()
{
   // Print the size of all the lists that handle Child Message Grouping (the list sizes should never grow).
   std::stringstream ss;
   ss << std::setw(2) << std::setfill('0') << m_parentMsgIdGroups.size() << " : " <<
         std::setw(2) << std::setfill('0') << m_processedParentMsgIDs.size() << " : " <<
         std::setw(2) << std::setfill('0') << m_queuedChildCurveMsgs.size();
   LOG_LINE(ss.str());
}
#endif

void CurveCommander::configForSpectrumAnalyzerView(QString plotName)
{
   extern bool inSpectrumAnalyzerMode; // This value is determined from the ini file.

   if(inSpectrumAnalyzerMode)
   {
      extern tSpecAnModeParam spectrumAnalyzerParams; // This value is determined from the ini file.

      // Check if the Plot should be used as the source when in Spectrum Analyzer Mode
      if( validCurve(plotName, spectrumAnalyzerParams.srcRealCurveName) &&
          validCurve(plotName, spectrumAnalyzerParams.srcImagCurveName) &&
          m_allCurves[plotName].plotGui->m_spectrumAnalyzerViewSet == false)
      {
         m_allCurves[plotName].plotGui->m_spectrumAnalyzerViewSet = true; // Only need to set up Spectrum Analyzer Mode for the source curves once.

         // Create the FFT child curve the will represent the Spectrum Analyzer view of the source plot.
         tParentCurveInfo xParent;
         tParentCurveInfo yParent;

         xParent.dataSrc.axis = E_Y_AXIS;
         xParent.dataSrc.plotName = plotName;
         xParent.dataSrc.curveName = spectrumAnalyzerParams.srcRealCurveName;
         xParent.scaleFftWindow = true;
         xParent.startIndex = 0;
         xParent.stopIndex = 0;
         xParent.windowFFT = true;

         yParent = xParent;
         yParent.dataSrc.curveName = spectrumAnalyzerParams.srcImagCurveName;

         m_allCurves[plotName].plotGui->setScrollMode(true, spectrumAnalyzerParams.srcNumSamples);

         createChildCurve(spectrumAnalyzerParams.specAnPlotName, spectrumAnalyzerParams.specAnCurveName, E_PLOT_TYPE_DB_POWER_FFT_COMPLEX, true, false, xParent, yParent);

      }
      // Check if we need to make any adjustments to the Spectrum Analyzer FFT right after it has been created.
      else if( plotName == spectrumAnalyzerParams.specAnPlotName &&
               validCurve(spectrumAnalyzerParams.specAnPlotName, spectrumAnalyzerParams.specAnCurveName) &&
               m_allCurves[spectrumAnalyzerParams.specAnPlotName].plotGui->m_spectrumAnalyzerViewSet == false)
      {
         MainWindow* plotGui = m_allCurves[spectrumAnalyzerParams.specAnPlotName].plotGui;
         plotGui->m_spectrumAnalyzerViewSet = true; // Only need to set up Spectrum Analyzer Mode for the FFT curve once.

         // Adjust the Spectrum Analyzer FFT to compensate for the source full scale value (make sure to not do a log of 0 or negative).
         if(spectrumAnalyzerParams.srcFullScale > 0)
         {
            tMathOpList toDbFs;
            tOperation subVal;
            subVal.helperNum = 0;
            subVal.num = -20*log10(spectrumAnalyzerParams.srcFullScale);
            subVal.op = E_ADD;
            toDbFs.push_front(subVal);

            plotGui->setCurveProperties(spectrumAnalyzerParams.specAnCurveName, E_Y_AXIS, 0, toDbFs);
         }

         // Apply the final settings to the FFT plot to make sure it initializes to Spectrum Analyzer Mode
         plotGui->setSnrBarMode(true); // Show the SNR bars.
         plotGui->externalZoomReset(); // Reset Zoom to handle the math adjustment made above.
      }
   }
}
