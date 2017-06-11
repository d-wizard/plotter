/* Copyright 2017 Dan Williams. All Rights Reserved.
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
#include <stdio.h>
#include <string>
#include <string.h>
#include <QString>
#include <QProcess>
#include "FileSystemOperations.h"
#include "dString.h"

//#define COPY_UPDATER_TO_TEMP_DIR

#ifdef COPY_UPDATER_TO_TEMP_DIR
static std::string getUniqueTempFileName(std::string tempPath, const std::string& fileName)
{
   std::string startFileName = tempPath + fso::dirSep() + fileName;

   if (fso::FileExists(startFileName) == false && fso::DirExists(startFileName) == false)
   {
      return startFileName;
   }
   else
   {
      std::string checkFileName;
      int number = 0;
      char numAscii[20];
      do
      {
         snprintf(numAscii, sizeof(numAscii), "%d", number++);
         checkFileName = startFileName + std::string(numAscii);
      } while (fso::FileExists(checkFileName) || fso::DirExists(checkFileName));

      return checkFileName;
   }

   return "";
}

static std::string getEnvVar(QString envVarNam)
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

static bool copyFile(std::string srcPath, std::string destPath)
{
   bool success = false;
   if(fso::FileExists(srcPath))
   {
      std::vector<char> inFile;
      fso::ReadBinaryFile(srcPath, inFile);
      fso::WriteFile(destPath, &inFile[0], inFile.size());
      success = true;
   }
   return success;
}
#endif


std::string updatePlotter(std::string pathToThisBinary)
{
   std::string updateBinaryFileName = "update";
   #ifdef Q_OS_WIN32
   updateBinaryFileName = "update.exe";
   #endif

   std::string updateBinaryDllFileName = "libcrypto-1_1.dll";

   pathToThisBinary = fso::dirSepToOS(pathToThisBinary);
   std::string pathToUpdateBinary = fso::GetDir(pathToThisBinary) + fso::dirSep() + updateBinaryFileName;
   std::string pathToUpdateDll    = fso::GetDir(pathToThisBinary) + fso::dirSep() + updateBinaryDllFileName;

   std::string finalUpdateBinaryPath;
   std::string updateCmd = "";
   if(fso::FileExists(pathToUpdateBinary))
   {
#ifdef COPY_UPDATER_TO_TEMP_DIR
      std::string tempDir = "/tmp";
      #ifdef Q_OS_WIN32
      tempDir = getEnvVar("TEMP");
      #endif

      std::string tempUpdateDir = getUniqueTempFileName(tempDir, "plotterUpdate");
      fso::createDir(tempUpdateDir);

      if(fso::DirExists(tempUpdateDir))
      {
         std::string tempUpdateBinaryPath = tempUpdateDir + fso::dirSep() + updateBinaryFileName;
         copyFile(pathToUpdateBinary, tempUpdateBinaryPath);
         copyFile(pathToUpdateDll, tempUpdateDir + fso::dirSep() + updateBinaryDllFileName);

         finalUpdateBinaryPath = tempUpdateBinaryPath;
         // Add quotes around tempUpdateBinaryPath and pathToThisBinary
         updateCmd = "\"" + tempUpdateBinaryPath + "\" -p \"" + pathToThisBinary + "\"";
      }
#else
      finalUpdateBinaryPath = pathToUpdateBinary;
#endif

      // Add quotes around finalUpdateBinaryPath and pathToThisBinary
      updateCmd = "\"" + finalUpdateBinaryPath + "\" -p \"" + pathToThisBinary + "\"";
   }
   return updateCmd;
}
