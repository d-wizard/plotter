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
#include "plotguimain.h"
#include "dString.h"
#include "FileSystemOperations.h"

int main(int argc, char *argv[])
{
    int dummyArgc = 0;
    bool validPort = false;
    unsigned short port = 0xFFFF;

    if(argc >= 2)
    {
        unsigned int cmdLinePort = atoi(argv[1]);
        if(cmdLinePort > 0 && cmdLinePort <= 0xFFFF)
        {
            validPort = true;
            port = cmdLinePort;
        }
    }

    if(validPort == false)
    {
        std::string iniName(fso::GetFileNameNoExt(argv[0]));
        iniName.append(".ini");

        std::string iniFile = fso::ReadFile(iniName);

        // If the ini file is empty, don't do anything.
        if(dString::Compare(iniFile, "") == false)
        {
            // convert windows line ending to linux
            iniFile = dString::Replace(iniFile, "\r\n", "\n");

            // suround with new line for easier searching
            std::string temp = "\n";
            iniFile = temp.append(iniFile).append("\n");

            std::string portFromIni = dString::GetMiddle(&iniFile, "\nport=", "\n");
            unsigned int iniPort = atoi(portFromIni.c_str());
            if(iniPort > 0 && iniPort <= 0xFFFF)
            {
                validPort = true;
                port = iniPort;
            }
        }

    }

    if(validPort == true)
    {
        QApplication a(dummyArgc, NULL);

        a.setQuitOnLastWindowClosed(false);

        plotGuiMain pgm(NULL, port);
        pgm.hide();

        return a.exec();
    }
    else
    {
        QApplication::quit();
        return -1;
    }

}
