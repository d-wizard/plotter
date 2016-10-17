/* Copyright 2016 Dan Williams. All Rights Reserved.
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
/*
 * persistentParamerts.cpp
 *
 *  Created on: Sep 27, 2016
 *      Author: d
 */
#include <iostream>
#include <iomanip>
#include "persistentParameters.h"
#include "FileSystemOperations.h"
#include "dString.h"
#include "pthread.h"

static const std::string PERSISTENT_APPEND = "_persistentParam.ini";
static const std::string DELIM = "=";


static std::string iniPath = "";

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void persistentParam_setPath(std::string path)
{
   pthread_mutex_lock(&mutex);
   
   if(fso::FileExists(path))
   {
      std::string ext = dString::Lower(fso::GetExt(path));
      if(ext == "ini")
      {
         iniPath = path;
      }
      else
      {
         std::string pathNameNoExt = dString::SplitBackLeft(path, ".");
         iniPath = pathNameNoExt + PERSISTENT_APPEND;
      }
   }
   else
   {
      std::string dir = path;
      if(fso::DirExists(dir))
      {
         iniPath = dir + fso::dirSep() + PERSISTENT_APPEND;
      }
   }
   if(iniPath != "" && fso::FileExists(iniPath) == false)
   {
      std::string iniFileInit = "";
      fso::WriteFile(iniPath, iniFileInit);
   }
   pthread_mutex_unlock(&mutex);
}

bool persistentParam_setParam_str(const std::string& paramName, std::string writeVal)
{
   bool success = false;
   if(fso::FileExists(iniPath))
   {
      pthread_mutex_lock(&mutex);
      {
         std::string searchStr = paramName + DELIM;
         std::string iniFile = dString::ConvertLineEndingToOS(fso::ReadFile(iniPath));
         std::string endl = dString::GetLineEnding();
         
         std::vector<std::string> lines = dString::SplitV(iniFile, endl);
         bool paramNameExistsInLines = false;
         for(size_t i = 0; i < lines.size(); ++i)
         {
            if(dString::Left(lines[i], searchStr.size()) == searchStr)
            {
               paramNameExistsInLines = true;
               lines[i] = searchStr + writeVal;
               break;
            }
         }
         
         iniFile = dString::JoinV(lines, endl);
         if(paramNameExistsInLines == false)
         {
            iniFile = iniFile + endl + searchStr + writeVal;
         }      
         
         // Remove any double line endings.
         std::string twoEndl = endl + endl;
         while(dString::InStr(iniFile, twoEndl) >= 0)
         {
            iniFile = dString::Replace(iniFile, twoEndl, endl);
         }
         // Don't start or end with endl.
         iniFile = dString::DontStartWithThis(iniFile, endl);
         iniFile = dString::DontEndWithThis(iniFile, endl);
   
         fso::WriteFile(iniPath, iniFile);
         
         success = true;
      }
      pthread_mutex_unlock(&mutex); 
   }
   
   return success;
}
bool persistentParam_setParam_f64(const std::string& paramName, double writeVal)
{
   std::stringstream ss;
   ss << writeVal;
   return persistentParam_setParam_str(paramName, ss.str());
}

bool persistentParam_getParam_str(const std::string& paramName, std::string& retVal)
{
   bool success = false;
   if(fso::FileExists(iniPath))
   {
      pthread_mutex_lock(&mutex);
      std::string endl = dString::GetLineEnding();
      std::string searchStr = endl + paramName + DELIM;
      
      std::string iniFile = dString::ConvertLineEndingToOS(fso::ReadFile(iniPath));
      iniFile = dString::DontStartWithThis(iniFile, endl);
      iniFile = dString::DontEndWithThis(iniFile, endl);
      
      // Make sure iniFile starts and ends with line ending for easier searching in the following lines.
      iniFile = endl + iniFile + endl;
      
      if(dString::InStr(iniFile, paramName) >= 0)
      {
         std::string temp = iniFile;
         retVal = dString::GetMiddle(&temp, searchStr, endl);
         success = true;
      }
      
      pthread_mutex_unlock(&mutex);
   }

   return success;
}

bool persistentParam_getParam_f64(const std::string& paramName, double& retVal)
{
   std::string temp = "";
   bool success = persistentParam_getParam_str(paramName, temp);
   if(success)
   {
      success = dString::strTo(temp, retVal);
   }
   return success;
}

