/* Copyright 2017, 2021 Dan Williams. All Rights Reserved.
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
#include <QFile>
#include <QDir>
#include <QThread>
#include "FileSystemOperations.h"
#include "dString.h"
#include "persistentParameters.h"

#ifdef Q_OS_WIN
#include <windows.h> // Sleep, CreateProcessA
#endif

#define COPY_UPDATER_TO_TEMP_DIR

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#ifdef Q_OS_WIN32
   const char* UPDATE_FILES[] = {"update.exe", "libcrypto-1_1.dll", "msvcr120.dll", "updateSources.txt"};
#else
   const char* UPDATE_FILES[] = {"update", "updateSources.txt"};
#endif
#define UPDATE_FILES_UPDATE_EXE_INDEX (0)

#define PERSISTENT_PARAM_TEMP_PATH_KEY "UpdateTempDirectory"


static bool dirEmptyOrDoesntExist(std::string t_dir)
{
   bool empty = true;
   if(fso::DirExists(t_dir))
   {
      fso::tDirContents t_dirContents;
      fso::GetDirContents(t_dirContents, t_dir, false);
      if(t_dirContents.size() > 0)
      {
         empty = false;
      }
   }
   else if(fso::FileExists(t_dir))
   {
      empty = false;
   }
   return empty;
}

#ifdef COPY_UPDATER_TO_TEMP_DIR // #ifdef out local functions that are not needed
static std::string getUniqueTempFileName(std::string tempPath, const std::string& fileName)
{
   std::string startFileName = tempPath + fso::dirSep() + fileName;

   if(fso::FileExists(startFileName) == false && dirEmptyOrDoesntExist(startFileName) == true)
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
      } while (fso::FileExists(checkFileName) || !dirEmptyOrDoesntExist(checkFileName));

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

bool updatePlotter(std::string pathToThisBinary, std::string& plotterUpdatePath, std::vector<std::string>& plotterUpdateArgs, std::string plotterCmdLine)
{
   std::string updateBinaryFileName = UPDATE_FILES[UPDATE_FILES_UPDATE_EXE_INDEX];

   pathToThisBinary = fso::dirSepToOS(pathToThisBinary);
   std::string pathToUpdateBinary = fso::GetDir(pathToThisBinary) + fso::dirSep() + updateBinaryFileName;

   std::string finalUpdateBinaryPath;
   bool valid = false;
   if(fso::FileExists(pathToUpdateBinary))
   {
#ifdef COPY_UPDATER_TO_TEMP_DIR
      std::string tempDir = "/tmp";
      #ifdef Q_OS_WIN32
      tempDir = getEnvVar("TEMP");
      #endif

      // Get the temp folder and create it if it doesn't exist.
      std::string tempUpdateDir = getUniqueTempFileName(tempDir, "plotterUpdate");
      if(fso::DirExists(tempUpdateDir) == false)
      {
         fso::createDir(tempUpdateDir);
      }

      if(fso::DirExists(tempUpdateDir))
      {
         // Copy all the update files.
         for(size_t i = 0; i < ARRAY_SIZE(UPDATE_FILES); ++i)
         {
            copyFile(fso::GetDir(pathToThisBinary) + fso::dirSep() + UPDATE_FILES[i],
                     tempUpdateDir + fso::dirSep() + UPDATE_FILES[i]);
         }

         finalUpdateBinaryPath = tempUpdateDir + fso::dirSep() + updateBinaryFileName;

         persistentParam_setParam_str(PERSISTENT_PARAM_TEMP_PATH_KEY, tempUpdateDir);
      }
#else
      finalUpdateBinaryPath = pathToUpdateBinary;
#endif

      // Add quotes around finalUpdateBinaryPath and pathToThisBinary
      plotterCmdLine = "\"" + finalUpdateBinaryPath + "\" -p \"" + pathToThisBinary + "\"";

      plotterUpdatePath = finalUpdateBinaryPath;

      plotterUpdateArgs.push_back("-p");
      plotterUpdateArgs.push_back(pathToThisBinary);

      valid = true;
   }
   return valid;
}

void cleanupAfterUpdate()
{
   std::string tempUpdateDir = "";

   // Check if there is a temp update dir to remove. Read the value from the Persistent Parameters file.
   if( persistentParam_getParam_str(PERSISTENT_PARAM_TEMP_PATH_KEY, tempUpdateDir) &&
       tempUpdateDir != "" &&
       fso::DirExists(tempUpdateDir))
   {
      int removeTryCount = 0;
      bool canExitWhile = false;

      do
      {
         bool allFilesRemoved = true; // Init to true, only go false if we fail to delete an existing update file.
         bool updateDirRemoved = false;

         // Attempt to remove any Update Files from the temp update dir.
         for(size_t i = 0; i < ARRAY_SIZE(UPDATE_FILES); ++i)
         {
            std::string fileToDelete = tempUpdateDir + fso::dirSep() + UPDATE_FILES[i];
            if(fso::FileExists(fileToDelete))
            {
               allFilesRemoved = QFile::remove(fileToDelete.c_str()) && allFilesRemoved;
            }
         }

         // Attempt to remove the temp update dir.
         if(allFilesRemoved && dirEmptyOrDoesntExist(tempUpdateDir))
         {
            // Remove temp update dir if it exists.
            updateDirRemoved = !fso::DirExists(tempUpdateDir);
            if(updateDirRemoved == false)
            {
               QDir dir(tempUpdateDir.c_str());
               dir.rmdir(dir.absolutePath());
               updateDirRemoved = !fso::DirExists(tempUpdateDir);
            }

            // If the temp update dir was removed, clear the directory from persistent memory. This will make sure
            // we avoid attempting to delete it again in the future.
            if(updateDirRemoved)
            {
               persistentParam_setParam_str(PERSISTENT_PARAM_TEMP_PATH_KEY, "");
            }
         }

         canExitWhile = (allFilesRemoved && updateDirRemoved) || (++removeTryCount < 5);
         if(!canExitWhile)
         {
#ifdef Q_OS_WIN
            Sleep(1000);
#else
            struct timespec ts = {1,0};
            nanosleep(&ts, NULL);
#endif
         }

      }while(!canExitWhile);
   }

}


bool startExecutable(std::string& exePath, std::vector<std::string> &exeArgs, std::string& cmdLine)
{
#ifdef Q_OS_WIN
   (void)exeArgs; // Unused here.
   STARTUPINFOA si;
   PROCESS_INFORMATION pi;

   ZeroMemory(&si, sizeof(si));
   si.cb = sizeof(si);
   ZeroMemory(&pi, sizeof(pi));

   std::vector<char> cmdLineCopy;
   cmdLineCopy.resize(cmdLine.size()+1);
   cmdLineCopy[cmdLine.size()] = '\0'; // Null Terminator
   memcpy(&cmdLineCopy[0], cmdLine.c_str(), cmdLine.size());


   BOOL createProcessResult = CreateProcessA
   (
      exePath.c_str(),
      cmdLineCopy.data(),
      NULL, NULL, FALSE,
      CREATE_NEW_CONSOLE,
      NULL, NULL,
      &si,
      &pi
   );
   return !!createProcessResult;
#else
   // startDetached doesn't seem to work with QT5 (it did with QT4). Now this app won't quit while the child app is running.
   // At least that is the case in Windows. Maybe it will work in Linux. Otherwise system might do it.
   (void)cmdLine;
   QString cmd(exePath.c_str());
   QStringList args;
   for(auto& arg : exeArgs)
       args.push_back(arg.c_str());

   return QProcess::startDetached(cmd, args);
#endif
}
