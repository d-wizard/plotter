/* Copyright 2013, 2015 - 2016 Dan Williams. All Rights Reserved.
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
#include "plotMsgPack.h"
#include "plotguimain.h"

plotGuiMain* pgm = NULL;
volatile bool isRunning = false;

bool defaultCursorZoomModeIsZoom = false;

class AppThread : public QThread {
   volatile bool* m_running;
   void run()
   {
      int argc = 0;
      QApplication a(argc, NULL);
      a.setQuitOnLastWindowClosed(false);
      pgm = new plotGuiMain(NULL, 0, false);
      pgm->hide();
      *m_running = true;
      a.exec();
      *m_running = false;
   }
public:
   AppThread(volatile bool* running):m_running(running){}
};

#ifdef DLL_HELPER_WIN_BUILD
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpvReserved*/ )
{
   return TRUE;
}
#endif

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
         // Plots need to be closed outside this thread and
         // before pgm's destructor is called.
         pgm->closeAllPlotsFromLib();
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
      pgm->startPlotMsgProcess(msg, len);
   }
}

void create1dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   int yAxisType,
                   void* yAxisSamples)
{
   if(pgm != NULL)
   {
      std::vector<char> msg;
      t1dPlot plotParam;

      plotParam.plotName = plotName;
      plotParam.curveName = curveName;
      plotParam.numSamp = numSamp;
      plotParam.yAxisType = (ePlotDataTypes)yAxisType;

      msg.resize(getCreatePlot1dMsgSize(&plotParam));

      packCreate1dPlotMsg(&plotParam, yAxisSamples, &msg[0]);

      pgm->startPlotMsgProcess(&msg[0], msg.size());
   }
}

void create2dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   int xAxisType,
                   int yAxisType,
                   void* xAxisSamples,
                   void* yAxisSamples)
{
   if(pgm != NULL)
   {
      std::vector<char> msg;
      t2dPlot plotParam;

      plotParam.plotName = plotName;
      plotParam.curveName = curveName;
      plotParam.numSamp = numSamp;
      plotParam.xAxisType = (ePlotDataTypes)xAxisType;
      plotParam.yAxisType = (ePlotDataTypes)yAxisType;

      msg.resize(getCreatePlot2dMsgSize(&plotParam));

      packCreate2dPlotMsg(&plotParam, xAxisSamples, yAxisSamples, &msg[0]);

      pgm->startPlotMsgProcess(&msg[0], msg.size());
   }
}


void update1dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   unsigned int sampleStartIndex,
                   int yAxisType,
                   void* yAxisSamples)
{
   if(pgm != NULL)
   {
      std::vector<char> msg;
      t1dPlot plotParam;

      plotParam.plotName = plotName;
      plotParam.curveName = curveName;
      plotParam.numSamp = numSamp;
      plotParam.yAxisType = (ePlotDataTypes)yAxisType;

      msg.resize(getUpdatePlot1dMsgSize(&plotParam));

      packUpdate1dPlotMsg(&plotParam, sampleStartIndex, yAxisSamples, &msg[0]);

      pgm->startPlotMsgProcess(&msg[0], msg.size());
   }
}

void update2dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   unsigned int sampleStartIndex,
                   int xAxisType,
                   int yAxisType,
                   void* xAxisSamples,
                   void* yAxisSamples)
{
   if(pgm != NULL)
   {
      std::vector<char> msg;
      t2dPlot plotParam;

      plotParam.plotName = plotName;
      plotParam.curveName = curveName;
      plotParam.numSamp = numSamp;
      plotParam.xAxisType = (ePlotDataTypes)xAxisType;
      plotParam.yAxisType = (ePlotDataTypes)yAxisType;

      msg.resize(getUpdatePlot2dMsgSize(&plotParam));

      packUpdate2dPlotMsg(&plotParam, sampleStartIndex, xAxisSamples, yAxisSamples, &msg[0]);

      pgm->startPlotMsgProcess(&msg[0], msg.size());
   }
}


void removeCurve(char* plotName, char* curveName)
{
    pgm->getCurveCommander().removeCurve(plotName, curveName);
}

void setLegendState(char* plotName, char showLegend)
{
    MainWindow* plot = pgm->getCurveCommander().getMainPlot(plotName);
    if(plot != NULL)
    {
        plot->setLegendState(showLegend != 0);
    }
}
