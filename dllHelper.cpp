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
#include <QtGui/QApplication>
#include <QThread>
#include "dllHelper.h"
#include "PackUnpackPlotMsg.h"
#include "plotguimain.h"

plotGuiMain* pgm = NULL;
volatile bool isRunning = false;

class AppThread : public QThread {
   volatile bool* m_running;
   void run()
   {
      int argc = 0;
      QApplication a(argc, NULL);
      a.setQuitOnLastWindowClosed(false);
      pgm = new plotGuiMain(NULL, 0);
      pgm->hide();
      *m_running = true;
      a.exec();
      *m_running = false;
   }
public:
   AppThread(volatile bool* running):m_running(running){}
};

void startPlotGui(void)
{
   if(isRunning == false)
   {
      AppThread * thread = new AppThread(&isRunning);
      thread->start();
      while(isRunning == false);
   }
}

void stopPlotGui(void)
{
   if(isRunning == true)
   {
      if(pgm != NULL)
      {
         delete pgm;
      }
      qApp->thread()->quit();
      qApp->thread()->wait();
   }
}

void createPlot(char* msg)
{
   if(pgm != NULL)
   {
      unsigned int len = 0;
      memcpy(&len, msg+4, sizeof(len));
      pgm->readPlotMsg(msg, len);
   }
}

void create1dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   int yAxisType,
                   int yShiftValue,
                   void* yAxisSamples)
{
   if(pgm != NULL)
   {
      std::vector<char> msg;
      pack1dPlotMsg( msg,
                     plotName,
                     curveName,
                     numSamp,
                     (ePlotDataTypes)yAxisType,
                     yShiftValue,
                     yAxisSamples);

      pgm->readPlotMsg(&msg[0], msg.size());
   }
}

void create2dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   int xAxisType,
                   int xShiftValue,
                   int yAxisType,
                   int yShiftValue,
                   void* xAxisSamples,
                   void* yAxisSamples)
{
   if(pgm != NULL)
   {
      std::vector<char> msg;
      pack2dPlotMsg( msg,
                     plotName,
                     curveName,
                     numSamp,
                     (ePlotDataTypes)xAxisType,
                     xShiftValue,
                     (ePlotDataTypes)yAxisType,
                     yShiftValue,
                     xAxisSamples,
                     yAxisSamples);
      pgm->readPlotMsg(&msg[0], msg.size());
   }
}
