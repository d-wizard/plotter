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
#include <QtGui/QApplication>
#include "plotguimain.h"
#include "dString.h"
#include "FileSystemOperations.h"
#include "PackUnpackPlotMsg.h"
#include "persistentParameters.h"

bool defaultCursorZoomModeIsZoom = false;

int main(int argc, char *argv[])
{
   bool validPort = false;
   unsigned short port = 0xFFFF;

   // First argument is path to executable,
   // argc must be 2 or more for command line parameters.
   if(argc >= 2)
   {
      unsigned int cmdLinePort = atoi(argv[1]);
      if(cmdLinePort > 0 && cmdLinePort <= 0xFFFF)
      {
         validPort = true;
         port = cmdLinePort;
      }
   }

   persistentParam_setPath(argv[0]);

   std::string iniName(fso::GetFileNameNoExt(argv[0]));
   iniName.append(".ini");

   std::string iniFile = fso::ReadFile(iniName);

   // If the ini file is empty, don't do anything.
   if(iniFile != "")
   {
      // convert windows line ending to linux
      iniFile = dString::Replace(iniFile, "\r\n", "\n");

      // suround with new line for easier searching
      iniFile = std::string("\n") + iniFile + std::string("\n");

      if(validPort == false)
      {
         std::string portFromIni = dString::SplitRight(iniFile, "\nport=");
         portFromIni = dString::GetNumFromStr(portFromIni);
         if(portFromIni.size() > 0)
         {
            unsigned int iniPort = atoi(portFromIni.c_str());
            if(iniPort > 0 && iniPort <= 0xFFFF)
            {
               validPort = true;
               port = iniPort;
            }
         }
      } // End if(validPort == false)

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

   if(validPort == true)
   {
      int dummyArgc = 0;
      QApplication a(dummyArgc, NULL);

      a.setQuitOnLastWindowClosed(false);

#if (defined(_WIN32) || defined(__WIN32__))
      plotGuiMain pgm(NULL, port, true);
#else
      // Linux doesn't seem to have a tray, so show the GUI with similar functions
      plotGuiMain pgm(NULL, port, false);
      pgm.show();
#endif

      return a.exec();
   }
   else
   {
      QApplication::quit();
      return -1;
   }

}
