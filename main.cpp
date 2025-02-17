/* Copyright 2013 - 2019, 2021, 2023, 2025 Dan Williams. All Rights Reserved.
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
#include <QApplication>
#include <QProcess>
#include <QString>
#include <assert.h>
#include <vector>
#include "plotguimain.h"
#include "dString.h"
#include "FileSystemOperations.h"
#include "PackUnpackPlotMsg.h"
#include "persistentParameters.h"
#include "sendTCPPacket.h"
#include "plotMsgPack.h"
#include "update.h"
#include "pthread.h"
#include "spectrumAnalyzerModeTypes.h"

#ifdef Q_OS_WIN32 // Q_OS_LINUX // http://stackoverflow.com/a/8556254
#define PLOTTER_WINDOWS_BUILD
#endif

// Local Variables.
static bool g_portsSpecifiedViaCmdLine = false;
static std::vector<unsigned short> g_ports;
static std::vector<std::string> g_cmdLineRestorePlotFilePaths;

// Global Variables (might be extern'd)
bool defaultCursorZoomModeIsZoom = false;
bool default2dPlotStyleIsLines = false; // true = Lines, false = Dots
bool inSpectrumAnalyzerMode = false;
tSpecAnModeParam spectrumAnalyzerParams;

// Local Functions
static QString getEnvVar(QString envVarNam)
{
   std::string searchStr = envVarNam.toStdString() + "=";
   QStringList envList(QProcess::systemEnvironment());
   foreach(QString env, envList)
   {
      if(dString::Left(env.toStdString(), searchStr.size()) == searchStr)
      {
         return dString::SplitRight(env.toStdString(), searchStr).c_str();
      }
   }
   return "";
}

static int connectToExistingAppInstance(unsigned short port)
{
   return sendTCPPacket_init("127.0.0.1", port);
}

static void processCmdLineArgs(int argc, char *argv[])
{
   // First argument is path to executable.

   // If the second argument is a valid port number, use it for the socket server port number.
   if(argc >= 2)
   {
      unsigned int cmdLinePort = atoi(argv[1]);
      if(cmdLinePort > 0 && cmdLinePort <= 0xFFFF)
      {
         g_portsSpecifiedViaCmdLine = true;
         g_ports.push_back(cmdLinePort);
      }
   }

   // If any of the command line arguments are valid file paths, assume they are stored plot files
   // and restore them from the files.
   for(int argIndex = 1; argIndex < argc; ++argIndex)
   {
      std::string plotFilePath(argv[argIndex]);
      if(fso::FileExists(plotFilePath))
      {
         g_cmdLineRestorePlotFilePaths.push_back(plotFilePath);
      }
   }
}

void sendRestorePlotPathsToActiveInstance(int connectionToExistingAppInstance)
{
   ePlotAction plotAction = E_OPEN_PLOT_FILE;
   unsigned long totalMsgSize = 0; // will be set later.

   totalMsgSize = sizeof(plotAction) + sizeof(totalMsgSize);
   for(unsigned int i = 0; i <g_cmdLineRestorePlotFilePaths.size(); ++i)
   {
      totalMsgSize += (g_cmdLineRestorePlotFilePaths[i].size() + 1); // Add 1 to include string null terminator.
   }

   std::vector<char> packedPlotMsg;
   packedPlotMsg.resize(totalMsgSize);
   unsigned int packIndex = 0;

   memcpy(&packedPlotMsg[packIndex], &plotAction, sizeof(plotAction));
   packIndex += sizeof(plotAction);
   memcpy(&packedPlotMsg[packIndex], &totalMsgSize, sizeof(totalMsgSize));
   packIndex += sizeof(totalMsgSize);
   for(unsigned int i = 0; i <g_cmdLineRestorePlotFilePaths.size(); ++i)
   {
      unsigned long strPackSize = g_cmdLineRestorePlotFilePaths[i].size() + 1; // Add 1 to include string null terminator.
      memcpy(&packedPlotMsg[packIndex], g_cmdLineRestorePlotFilePaths[i].c_str(), strPackSize);
      packIndex += strPackSize;
   }

   assert(packIndex == packedPlotMsg.size());

   sendTCPPacket_send(connectionToExistingAppInstance, &packedPlotMsg[0], packIndex);
}

static std::string getIniParam(std::string iniFile, std::string paramName, bool inQuotes = false)
{
   std::string retVal = dString::GetMiddle(&iniFile, "\n" + paramName + "=", "\n");
   if(inQuotes)
   {
      retVal = dString::GetMiddle(&retVal, "\"", "\"");
   }
   return retVal;
}

static void setSpectrumAnalyzerModeFromIni(std::string iniFile)
{
   std::string specAnModeActive = getIniParam(iniFile, "spec_an_mode_active");
   if(dString::Lower(specAnModeActive) == "true")
   {
      inSpectrumAnalyzerMode = true;
   }

   if(inSpectrumAnalyzerMode)
   {
      std::string temp;

      // Set Default Values.
      spectrumAnalyzerParams.srcRealCurveName = "";
      spectrumAnalyzerParams.srcImagCurveName = "";

      spectrumAnalyzerParams.srcFullScale = 1.0;
      spectrumAnalyzerParams.srcNumSamples = 0;

      spectrumAnalyzerParams.specAnPlotName = "Spectrum Analyzer";
      spectrumAnalyzerParams.specAnCurveName = "FFT";

      // Source Curve Names must be specified in the ini, so there is no need to do any validation before setting the values.
      spectrumAnalyzerParams.srcRealCurveName = getIniParam(iniFile, "spec_an_src_real_curve_name", true).c_str();
      spectrumAnalyzerParams.srcImagCurveName = getIniParam(iniFile, "spec_an_src_imag_curve_name", true).c_str();

      // Source Full Scale Value (this value can be a non integer value)
      temp = getIniParam(iniFile, "spec_an_src_full_scale");
      if(temp != "")
      {
         bool success = dString::strTo(temp, spectrumAnalyzerParams.srcFullScale);
         inSpectrumAnalyzerMode = inSpectrumAnalyzerMode && success;
      }

      // Source number of input samples for the FFT
      temp = getIniParam(iniFile, "spec_an_src_num_samples");
      if(temp != "")
      {
         bool success = dString::strTo(temp, spectrumAnalyzerParams.srcNumSamples);
         inSpectrumAnalyzerMode = inSpectrumAnalyzerMode && success;
      }

      // Set the Desination Plot Name for the Spectrum Analyzer plot name.
      temp = getIniParam(iniFile, "spec_an_plot_name", true);
      if(temp != "")
      {
         spectrumAnalyzerParams.specAnPlotName = temp.c_str();
      }

      // Set the Desination Plot Name for the Spectrum Analyzer curve name.
      temp = getIniParam(iniFile, "spec_an_curve_name", true);
      if(temp != "")
      {
         spectrumAnalyzerParams.specAnCurveName = temp.c_str();
      }

      // If some paremeters are invalid, then we can't do Spectrum Analyzer Mode.
      if( spectrumAnalyzerParams.srcRealCurveName == "" || spectrumAnalyzerParams.srcImagCurveName == "" ||
          spectrumAnalyzerParams.srcNumSamples == 0 || spectrumAnalyzerParams.srcFullScale <= 0 )
      {
         inSpectrumAnalyzerMode = false;
      }

   }
}

static bool getByteSizeFromIni(const std::string& iniFile, const std::string& key, unsigned int& retNum)
{
   bool found = false;
   std::string keySearchStr = std::string("\n") + key + "="; 
   std::string strBegin =  dString::SplitRight(iniFile, keySearchStr);

   std::string sizeNumStr = dString::SplitNumFromStr(strBegin);
   if(sizeNumStr.size() > 0)
   {
      retNum = atoi(sizeNumStr.c_str());
      std::string nextChar = strBegin.substr(0, 1);

      // Determine scale
      if(dString::Lower(nextChar) == "k")
         retNum *= 1024;
      else if(dString::Lower(nextChar) == "m")
         retNum *= (1024*1024);
      found = true;
   }
   return found;
}

static void processIniFile(int argc, char *argv[])
{
   (void)argc; // Tell the compiler not to warn that this variable is unused.

   // Try to find .ini file in Current Working Directory.
   std::string iniName(fso::GetFileNameNoExt(argv[0]));
   iniName.append(".ini");

   // Check if .ini file exists in Current Working Directory.
   if(fso::FileExists(iniName) == false)
   {
      // .ini file doesn't exist in Current Working Directory.
      // Try to find it in the same directory as the plotter binary.
      iniName = fso::RemoveExt(argv[0]);
      iniName.append(".ini");
   }


   std::string iniFile = fso::ReadFile(iniName);

   // If the ini file is empty, don't do anything.
   if(iniFile != "")
   {
      // convert windows line ending to linux
      iniFile = dString::Replace(iniFile, "\r\n", "\n");

      // suround with new line for easier searching
      iniFile = std::string("\n") + iniFile + std::string("\n");

      // Search for Spectrum Analyzer Mode setting right after the ini file has been modified for easier searching.
      setSpectrumAnalyzerModeFromIni(iniFile);

      // Grab port number(s) from .ini file (but only if port numbers
      // weren't specified via command line arguments).
      if(g_portsSpecifiedViaCmdLine == false)
      {
         const std::string INI_PORT_SEARCH_STR = "\nport=";
         std::string iniFile_findPorts = iniFile;

         while(dString::InStr(iniFile_findPorts, INI_PORT_SEARCH_STR) >= 0)
         {
            iniFile_findPorts = dString::SplitRight(iniFile_findPorts, INI_PORT_SEARCH_STR);

            std::string portFromIni = dString::GetNumFromStr(iniFile_findPorts);
            if(portFromIni.size() > 0)
            {
               unsigned int iniPort = atoi(portFromIni.c_str());
               if(iniPort > 0 && iniPort <= 0xFFFF)
               {
                  g_ports.push_back(iniPort);
               }
            }
         }
      } // End if(g_portsSpecifiedViaCmdLine == false)

      // Get the size of the max plot packet to support. This is an entire "stitched-together" packet that could be megabytes.
      if(getByteSizeFromIni(iniFile, "max_packet_size", g_maxTcpPlotMsgSize)) // Returns true if g_maxTcpPlotMsgSize was updated
      {
         g_maxTcpPlotMsgSize += MAX_PLOT_MSG_HEADER_SIZE; // Increase to add space for message header.
      }

      // Get the size of packets to support at the socket level.
      extern unsigned int g_tcp_maxPacketSize;
      extern unsigned int g_tcp_maxStoredPackets;
      getByteSizeFromIni(iniFile, "socket_max_packet_size", g_tcp_maxPacketSize);
      getByteSizeFromIni(iniFile, "socket_max_stored_packets", g_tcp_maxStoredPackets);

      std::string defaultMode = dString::GetMiddle(&iniFile, "\ndefault_mode=", "\n");
      if(defaultMode == std::string("zoom"))
      {
         defaultCursorZoomModeIsZoom = true;
      }

      std::string useLinesStyleFor2d = dString::GetMiddle(&iniFile, "\nuse_lines_for_2d_plots=", "\n");
      if(useLinesStyleFor2d == std::string("true"))
      {
         default2dPlotStyleIsLines = true; // true = Lines, false = Dots
      }

   } // End if(iniFile != "")
}

static void setPersistentParamPath()
{
#ifdef PLOTTER_WINDOWS_BUILD
   std::string appDataPath = getEnvVar("APPDATA").toStdString() +
         fso::dirSep() + "plotter";

   persistentParam_setPath(appDataPath);
#else
    std::string homePlotter = getEnvVar("HOME").toStdString() +
          fso::dirSep() + ".plotter";
   persistentParam_setPath(homePlotter);
#endif
}

static int startGuiApp()
{
   int dummyArgc = 0;
   QApplication a(dummyArgc, NULL);

   a.setQuitOnLastWindowClosed(false);

#ifdef PLOTTER_WINDOWS_BUILD
   plotGuiMain pgm(NULL, g_ports, true);
#else
   // Linux doesn't seem to have a tray, so show the GUI with similar functions
   plotGuiMain pgm(NULL, g_ports, false);
   pgm.show();
#endif

   return a.exec();
}

static int stopGuiApp()
{
   QApplication::quit();
   return -1;
}

void* sendRestorePathThread(void*)
{
   if(g_ports.size() > 0 && g_cmdLineRestorePlotFilePaths.size() > 0)
   {
      bool exitLoop = false;
      int tryCount = 0;
      unsigned short portToUse = g_ports[0];

      while(exitLoop == false)
      {
         // Need to wait for the Main GUI object to start and create the TCP server that is needed to process the plot to restore.
#ifdef PLOTTER_WINDOWS_BUILD
         Sleep(100);
#else
         struct timespec ts = {0,100*1000*1000};
         nanosleep(&ts, NULL);
#endif

         // Attempt to connect to the server.
         int connectionToThisAppInstance = connectToExistingAppInstance(portToUse);
         if(connectionToThisAppInstance >= 0)
         {
            sendRestorePlotPathsToActiveInstance(connectionToThisAppInstance);
            sendTCPPacket_close(connectionToThisAppInstance); // Close the connection that was just made.
            exitLoop = true;
         }
         exitLoop = exitLoop || (++tryCount > 100); // If unsuccessful, try 100 times before exiting the loop.
      }
   }
   return NULL;
}

void restorePathsFromCmdLineInThisInstance()
{
   if(g_ports.size() > 0 && g_cmdLineRestorePlotFilePaths.size() > 0)
   {
      static pthread_t restorePathsMsgThread; // Make static just incase... this function should only ever be called once anyway.
      static pthread_attr_t pta;              // Make static just incase... this function should only ever be called once anyway.

      pthread_attr_init(&pta);
      pthread_attr_setdetachstate(&pta, PTHREAD_CREATE_DETACHED); // This thread will never be joined, so make it a detached thread.
      pthread_create(&restorePathsMsgThread, &pta, sendRestorePathThread, NULL);
   }
}


int main(int argc, char *argv[])
{
   QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
   processCmdLineArgs(argc, argv);
   processIniFile(argc, argv);

   if(g_ports.size() == 0)
   {
       g_ports.push_back(2000);
   }
   if(g_ports.size() > 0)
   {
      int connectionToExistingAppInstance = -1;

      for(size_t i = 0; i < g_ports.size(); ++i)
      {
         connectionToExistingAppInstance = connectToExistingAppInstance(g_ports[i]);
         if(connectionToExistingAppInstance >= 0)
         {
            break;
         }
      }

      if(connectionToExistingAppInstance >= 0)
      {
         if(g_cmdLineRestorePlotFilePaths.size() > 0)
         {
            sendRestorePlotPathsToActiveInstance(connectionToExistingAppInstance);
         }

         // There is already an instance of the plot app running. Close this new instance now.
         return stopGuiApp();
      }

      setPersistentParamPath();

      cleanupAfterUpdate();

      restorePathsFromCmdLineInThisInstance(); // If cmd line has plot files to resore, create a thread to restore them once the main gui is created.

      return startGuiApp();
   }
   else
   {
      printf("No TCP Server Ports specified. Is the .ini file missing?\n");
      return stopGuiApp();
   }

}
