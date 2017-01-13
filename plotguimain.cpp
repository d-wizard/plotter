/* Copyright 2013 - 2017 Dan Williams. All Rights Reserved.
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
#include <QProcess>
#include <QMessageBox>
#include "FileSystemOperations.h"
#include "revDateStamp.h"
#include "PlotHelperTypes.h"

#include "plotguimain.h"
#include "ui_plotguimain.h"

#include "createfftplot.h"
#include "fftHelper.h"

#include "localPlotCreate.h"

#define MAX_NUM_MSGS_IN_QUEUE (1000)


plotGuiMain::plotGuiMain(QWidget *parent, unsigned short tcpPort, bool showTrayIcon) :
   QMainWindow(parent),
   ui(new Ui::plotGuiMain),
   m_tcpMsgReader(NULL),
   m_trayIcon(NULL),
   m_trayExitAction("Exit", this),
   m_trayEnDisNewCurvesAction("Disable New Curves", this),
   m_propertiesWindowAction("Properties", this),
   m_closeAllPlotsAction("Close All Plots", this),
   m_revDateStampAction(REV_DATE_STAMP, this),
   m_updateBinaryAction("Update", this),
   m_trayMenu(NULL),
   m_curveCommander(this),
   m_allowNewCurves(true),
   m_storedMsgBuff(new char[STORED_MSG_SIZE]),
   m_storedMsgBuffIndex(0)
{
    ui->setupUi(this);
    this->setFixedSize(165, 95);

    QObject::connect(this, SIGNAL(readPlotMsgSignal()),
                     this, SLOT(readPlotMsgSlot()), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(restorePlotFilesInListSignal()),
                     this, SLOT(restorePlotFilesInListSlot()), Qt::QueuedConnection);

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
    connect(&m_closeAllPlotsAction, SIGNAL(triggered(bool)), this, SLOT(closeAllPlotsSlot()));
    connect(&m_updateBinaryAction, SIGNAL(triggered(bool)), this, SLOT(updateBinarySlot()));

    m_trayMenu = new QMenu("Plot", this);

    m_revDateStampAction.setEnabled(false);
    m_trayMenu->addAction(&m_revDateStampAction);
    m_trayMenu->addAction(&m_updateBinaryAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(&m_trayEnDisNewCurvesAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(&m_propertiesWindowAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(&m_closeAllPlotsAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(&m_trayExitAction);

    ui->menubar->addMenu(m_trayMenu);
    
    if(showTrayIcon == true)
    {
       m_trayIcon = new QSystemTrayIcon(QIcon(":/plot.png"), this);
       m_trayIcon->setContextMenu(m_trayMenu);
       m_trayIcon->show();
    }

    QObject::connect(this, SIGNAL(closeAllPlotsFromLibSignal()),
                     this, SLOT(closeAllPlotsFromLibSlot()), Qt::QueuedConnection);
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

    delete m_storedMsgBuff;
}

void plotGuiMain::closeAllPlotsFromLib()
{
   emit closeAllPlotsFromLibSignal();
   m_sem.acquire();
}

void plotGuiMain::closeAllPlotsFromLibSlot()
{
   m_curveCommander.destroyAllPlots();
   m_sem.release();
}

void plotGuiMain::closeAllPlotsSlot()
{
   m_curveCommander.destroyAllPlots();
}

void plotGuiMain::restorePlotFilesInListSlot()
{
   m_plotFilesToRestoreMutex.lock();
   std::list<std::string>::iterator iter = m_plotFilesToRestoreList.begin();
   while(iter != m_plotFilesToRestoreList.end())
   {
      std::string plotFilePath = *iter;
      m_plotFilesToRestoreList.erase(iter); // Erase iterator (this will always be begin())
      m_plotFilesToRestoreMutex.unlock();

      localPlotCreate::restorePlotFromFile(&m_curveCommander, plotFilePath.c_str(), "");

      m_plotFilesToRestoreMutex.lock();
      iter = m_plotFilesToRestoreList.begin(); // Grab next iterator from begin()
   }
   m_plotFilesToRestoreMutex.unlock();
}

void plotGuiMain::updateBinarySlot()
{
   std::string pathToThisBinary = QCoreApplication::applicationFilePath().toStdString();

   std::string updateBinaryFileName = "update";
#ifdef Q_OS_WIN32
   updateBinaryFileName = "update.exe";
#endif

   pathToThisBinary = fso::dirSepToOS(pathToThisBinary);
   std::string pathToUpdateBinary = fso::GetDir(pathToThisBinary) + fso::dirSep() + updateBinaryFileName;

   if(fso::FileExists(pathToUpdateBinary))
   {
      // Add quotes around pathToUpdateBinary and pathToThisBinary
      std::string updateCmd = "\"" + pathToUpdateBinary + "\" -p \"" + pathToThisBinary + "\"";

      QProcess process(this);
      process.startDetached(updateCmd.c_str());
      QApplication::quit();
   }
   else
   {
      QMessageBox Msgbox;
      Msgbox.setWindowTitle("Update Failed");
      Msgbox.setText("Failed to find update executable.");
      Msgbox.exec();
   }

}

void plotGuiMain::startPlotMsgProcess(tIncomingMsg* inMsg)
{
   const char* msg = inMsg->msgPtr;
   unsigned int size = inMsg->msgSize;

   m_curveCommander.getIpBlocker()->addIpAddrToList(inMsg->ipAddr); // Store off the IP address

   if(m_allowNewCurves == true)// && size < STORED_MSG_SIZE)
   {
      m_storedMsgBuffMutex.lock(); // Make sure multiple threads cannot modify buffer at same time

      // Make sure new message won't overrun the buffer.
      if( (m_storedMsgBuffIndex + size) > STORED_MSG_SIZE )
      {
         m_storedMsgBuffIndex = 0;
      }

      char* msgCopy = m_storedMsgBuff+m_storedMsgBuffIndex;
      memcpy(msgCopy, msg, size);

      // Update index to position for next packet (make sure index is mod 4)
      m_storedMsgBuffIndex = (m_storedMsgBuffIndex + size + 3) & (~3);
      if( m_storedMsgBuffIndex >= STORED_MSG_SIZE )
      {
         m_storedMsgBuffIndex = 0;
      }

      m_storedMsgBuffMutex.unlock();

      tIncomingMsg msgToUnpack = *inMsg;
      msgToUnpack.msgPtr = msgCopy;

      UnpackMultiPlotMsg* msgUnpacker = new UnpackMultiPlotMsg(&msgToUnpack);
      if(msgUnpacker->m_plotMsgs.size() > 0)
      {
         bool msgPopped = false;
         m_multiPlotMsgsQueueMutex.lock();
         // If the queue of plot messages is getting too big (i.e. we aren't keeping up)
         // remove the oldest messages from the queue (i.e. drop them on the ground).
         while(m_multiPlotMsgs.size() > MAX_NUM_MSGS_IN_QUEUE)
         {
            delete m_multiPlotMsgs.front();
            m_multiPlotMsgs.pop();
            msgPopped = true;
         }
         m_multiPlotMsgs.push(msgUnpacker);
         m_multiPlotMsgsQueueMutex.unlock();

         // If a message has been popped off the queue, then there is no reason to
         // send the signal. The signal of the popped message should still be waiting
         // to be handled.
         if(msgPopped == false)
         {
            emit readPlotMsgSignal();
         }
      }
      else
      {
         delete msgUnpacker;
      }
   }
}

void plotGuiMain::readPlotMsgSlot()
{
   UnpackMultiPlotMsg* plotMsg = NULL;
   bool finishedReading = m_multiPlotMsgs.size() == 0;

   while(finishedReading == false)
   {
      m_multiPlotMsgsQueueMutex.lock();
      if(m_multiPlotMsgs.size() > 0)
      {
         plotMsg = m_multiPlotMsgs.front();
         m_multiPlotMsgs.pop();
      }
      m_multiPlotMsgsQueueMutex.unlock();

      readPlotMsg(plotMsg);

      m_multiPlotMsgsQueueMutex.lock();
      finishedReading = m_multiPlotMsgs.size() == 0;
      m_multiPlotMsgsQueueMutex.unlock();
   }

}

bool plotGuiMain::processNonSamplePlotMsgs(UnpackMultiPlotMsg* plotMsg)
{
   bool nonSamplePlotMsg = false;
   if( plotMsg->m_plotMsgs.find("") != plotMsg->m_plotMsgs.end() )
   {
      nonSamplePlotMsg = true;

      plotMsgGroup* noPlotName = plotMsg->m_plotMsgs[""];

      // Read out all the individual plot messages in the message group.
      UnpackPlotMsgPtrList::iterator plotMsgIter = noPlotName->m_plotMsgs.begin();
      while(plotMsgIter != noPlotName->m_plotMsgs.end())
      {
         UnpackPlotMsg* unpackedMsg = *plotMsgIter;

         // Process the E_PLOT_TYPE_RESTORE_PLOT_FROM_FILE messages (if they exists).
         if(unpackedMsg->m_restorePlotFromFileList.size() > 0)
         {
            std::list<std::string>::iterator restoreFileIter = unpackedMsg->m_restorePlotFromFileList.begin();
            while(restoreFileIter != unpackedMsg->m_restorePlotFromFileList.end())
            {
               // Call restore function.
               restorePlotFile(*restoreFileIter);
               ++restoreFileIter;
            }
         }

         // Make sure to delete.
         delete unpackedMsg;

         ++plotMsgIter;
      }
   }
   return nonSamplePlotMsg;
}

void plotGuiMain::readPlotMsg(UnpackMultiPlotMsg* plotMsg)
{
   // Check for special plot messages that don't contain actual samples to plot.
   // TODO: I should probably remove the plot message group with the plot name of ""
   // from plotMsg and continue processing any remaining plot groups in the list.
   // It just doesn't seem very likely that there ever would be a mixture of both
   // types of message in one UnpackMultiPlotMsg message.
   if(processNonSamplePlotMsgs(plotMsg) == false)
   {
      for(std::map<std::string, plotMsgGroup*>::iterator allMsgs = plotMsg->m_plotMsgs.begin(); allMsgs != plotMsg->m_plotMsgs.end(); ++allMsgs)
      {
         QString plotName(allMsgs->first.c_str());
         plotMsgGroup* group = allMsgs->second;
         for(UnpackPlotMsgPtrList::iterator plotMsgs = group->m_plotMsgs.begin(); plotMsgs != group->m_plotMsgs.end(); ++plotMsgs)
         {
            // Grab the parameters needed for storePlotMsg.
            const char* msgPtr( (*plotMsgs)->GetMsgPtr() );
            unsigned int msgSize( (*plotMsgs)->GetMsgPtrSize() );
            QString curveName( (*plotMsgs)->m_curveName.c_str() );
            m_curveCommander.storePlotMsg(msgPtr, msgSize, plotName, curveName);
         }
      }

      m_curveCommander.readPlotMsg(plotMsg);
   }

   delete plotMsg;
}

void plotGuiMain::restorePlotMsg(const char *msg, unsigned int size, tPlotCurveName plotCurveName)
{
   UnpackMultiPlotMsg* plotMsg = new UnpackMultiPlotMsg(msg, size);
   if(plotMsg->m_plotMsgs.size() > 0)
   {
      // Change all plot/curve names that were read from the unpacked message to the plot/curve names passed into this function.
      for(std::map<std::string, plotMsgGroup*>::iterator allMsgs = plotMsg->m_plotMsgs.begin(); allMsgs != plotMsg->m_plotMsgs.end(); ++allMsgs)
      {
         plotMsgGroup* group = allMsgs->second;
         for(UnpackPlotMsgPtrList::iterator plotMsgs = group->m_plotMsgs.begin(); plotMsgs != group->m_plotMsgs.end(); ++plotMsgs)
         {
            (*plotMsgs)->m_plotName = plotCurveName.plot.toStdString();
            (*plotMsgs)->m_curveName = plotCurveName.curve.toStdString();
         }
      }

      m_multiPlotMsgsQueueMutex.lock();
      m_multiPlotMsgs.push(plotMsg);
      m_multiPlotMsgsQueueMutex.unlock();
      emit readPlotMsgSignal();
   }
   else
   {
      delete plotMsg;
   }
}

void plotGuiMain::restorePlotFile(std::string plotFilePath)
{
   m_plotFilesToRestoreMutex.lock();
   m_plotFilesToRestoreList.push_back(plotFilePath);
   m_plotFilesToRestoreMutex.unlock();

   emit restorePlotFilesInListSignal();
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
