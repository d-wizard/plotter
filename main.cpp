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

// Local Variables.
static bool g_portsSpecifiedViaCmdLine = false;
static std::vector<unsigned short> g_ports;
static std::vector<std::string> g_cmdLineRestorePlotFilePaths;

// Global Variables (might be extern'd)
bool defaultCursorZoomModeIsZoom = false;



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

      std::string maxPacketSizeStrBegin =
            dString::SplitRight(iniFile, "\nmax_packet_size=");

      std::string maxPacketSizeNumStr = dString::SplitNumFromStr(maxPacketSizeStrBegin);
      if(maxPacketSizeNumStr.size() > 0)
      {
         g_maxTcpPlotMsgSize = atoi(maxPacketSizeNumStr.c_str());
         std::string nextChar = maxPacketSizeStrBegin.substr(0, 1);

         // Determine scale of Max Packet Size.
         if(dString::Lower(nextChar) == "k")
            g_maxTcpPlotMsgSize *= 1024;
         else if(dString::Lower(nextChar) == "m")
            g_maxTcpPlotMsgSize *= (1024*1024);

         // Increase to add remove for message header.
         g_maxTcpPlotMsgSize += MAX_PLOT_MSG_HEADER_SIZE;
      }

      std::string defaultMode = dString::GetMiddle(&iniFile, "\ndefault_mode=", "\n");
      if(defaultMode == std::string("zoom"))
      {
         defaultCursorZoomModeIsZoom = true;
      }

   } // End if(iniFile != "")
}

static void setPersistentParamPath()
{
#ifdef Q_OS_WIN32 // Q_OS_LINUX // http://stackoverflow.com/a/8556254
   std::string appDataPath = getEnvVar("APPDATA").toStdString() +
         fso::dirSep() + "plotter";

   persistentParam_setPath(appDataPath);
#else
   persistentParam_setPath(argv[0]);
#endif
}

static int startGuiApp()
{
   int dummyArgc = 0;
   QApplication a(dummyArgc, NULL);

   a.setQuitOnLastWindowClosed(false);

#if (defined(_WIN32) || defined(__WIN32__))
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


int main(int argc, char *argv[])
{
   processCmdLineArgs(argc, argv);
   processIniFile(argc, argv);

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

      return startGuiApp();
   }
   else
   {
      return stopGuiApp();
   }

}
