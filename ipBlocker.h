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
#ifndef IPBLOCKER_H
#define IPBLOCKER_H

#include <QMap>
#include <QList>
#include <QString>
#include <QMutexLocker>
#include "PlotHelperTypes.h"

class ipBlocker
{
public:
   // Types
   typedef QList<tPlotterIpAddr> tIpAddrs;
   typedef QList<QString> tIpBlockListOfPlotNames;
   typedef QMap<tPlotterIpAddr, tIpBlockListOfPlotNames> tMapOfBlockedIps;

   ipBlocker();
   ~ipBlocker();

   void addIpAddrToList(tPlotterIpAddr ipAddr);
   tIpAddrs& getIpAddrList();

   void addToBlockList(tPlotterIpAddr ipAddr);
   void addToBlockList(tPlotterIpAddr ipAddr, QString plotName);

   void removeFromBlockList(tPlotterIpAddr ipAddr);
   void removeFromBlockList(tPlotterIpAddr ipAddr, QString plotName);
   void clearBlockList();

   tMapOfBlockedIps& getBlockList();
   bool isInBlockList(tPlotterIpAddr ipAddr, QString plotName);

private:
   // Eliminate copy, assign
   ipBlocker(ipBlocker const&);
   void operator=(ipBlocker const&);

   tIpAddrs m_allIps;
   tMapOfBlockedIps m_mapOfBlockedIps;

   QMutex m_mutex;
};



#endif // IPBLOCKER_H
