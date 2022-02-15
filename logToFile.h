/* Copyright 2015, 2017, 2019 Dan Williams. All Rights Reserved.
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
#ifndef LOGTOFILE_H
#define LOGTOFILE_H

#include <sstream>
#include <iomanip>
#include <QTime>
#include <QElapsedTimer>
#include "FileSystemOperations.h"

static inline void logLine(std::string logFileName, std::string srcFileName, std::string funcName, int lineNum, std::string extra)
{
   QTime timeObj = QTime::currentTime();
   std::stringstream str;
   str << "[" << std::setw( 2 ) << std::setfill( '0' ) << timeObj.hour() << ":"
       << std::setw( 2 ) << std::setfill( '0' ) << timeObj.minute() << ":"
       << std::setw( 2 ) << std::setfill( '0' ) << timeObj.second() << "."
       << std::setw( 3 ) << std::setfill( '0' ) << timeObj.msec() << "] "
       << srcFileName << ":" << funcName << ":" << lineNum << " " << extra << std::endl;
   fso::AppendFile(logFileName, str.str());
}

static inline void logLineLoad(std::string logFileName, std::string srcFileName, std::string funcName, int lineNum, std::string extra, qint64 load)
{
   QTime timeObj = QTime::currentTime();
   std::stringstream str;
   str << "[" << std::setw( 2 ) << std::setfill( '0' ) << timeObj.hour() << ":"
       << std::setw( 2 ) << std::setfill( '0' ) << timeObj.minute() << ":"
       << std::setw( 2 ) << std::setfill( '0' ) << timeObj.second() << "."
       << std::setw( 3 ) << std::setfill( '0' ) << timeObj.msec() << "] "
       << srcFileName << ":" << funcName << ":" << lineNum << " " << extra << ": " << load << " ns" << std::endl;
   fso::AppendFile(logFileName, str.str());
}

#define LOG_LINE(extraPrint) logLine("LogLine.log", __FILE__, __func__, __LINE__, extraPrint);

#define LOG_LINE_LOAD_INIT static QElapsedTimer llTimer;
#define LOG_LINE_LOAD_START llTimer.start();
#define LOG_LINE_LOAD_STOP(extraPrint) logLineLoad("LogLine.log", __FILE__, __func__, __LINE__, extraPrint, llTimer.nsecsElapsed());

#endif // LOGTOFILE_H
