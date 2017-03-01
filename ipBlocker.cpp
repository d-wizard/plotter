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
#include "ipBlocker.h"


ipBlocker::ipBlocker():
   m_mutex(QMutex::Recursive)
{

}

ipBlocker::~ipBlocker()
{

}


void ipBlocker::addIpAddrToList(tPlotterIpAddr ipAddr)
{
   QMutexLocker locker(&m_mutex);

   if(!m_allIps.contains(ipAddr))
   {
      m_allIps.push_back(ipAddr);
   }
}

ipBlocker::tIpAddrs& ipBlocker::getIpAddrList()
{
   QMutexLocker locker(&m_mutex);

   return m_allIps;
}


void ipBlocker::addToBlockList(tPlotterIpAddr ipAddr)
{
   QMutexLocker locker(&m_mutex);

   if(!m_mapOfBlockedIps.contains(ipAddr))
   {
      tIpBlockListOfPlotNames plotNames_empty;
      m_mapOfBlockedIps[ipAddr] = plotNames_empty; // Empty list means block all.
   }
}

void ipBlocker::addToBlockList(tPlotterIpAddr ipAddr, QString plotName)
{
   QMutexLocker locker(&m_mutex);

   if(!isInBlockList(ipAddr, plotName))
   {
      if(!m_mapOfBlockedIps.contains(ipAddr))
      {
         tIpBlockListOfPlotNames plotNames;
         plotNames.push_back(plotName);
         m_mapOfBlockedIps[ipAddr] = plotNames;
      }
      else
      {
         m_mapOfBlockedIps[ipAddr].push_back(plotName);
      }
   }
}

void ipBlocker::removeFromBlockList(tPlotterIpAddr ipAddr)
{
   QMutexLocker locker(&m_mutex);

   // Remove all IP block entries with the matching IP Address.
   tMapOfBlockedIps::iterator ipAddrIter = m_mapOfBlockedIps.find(ipAddr);
   if(ipAddrIter != m_mapOfBlockedIps.end())
   {
      m_mapOfBlockedIps.erase(ipAddrIter);
   }
}

void ipBlocker::removeFromBlockList(tPlotterIpAddr ipAddr, QString plotName)
{
   QMutexLocker locker(&m_mutex);

   tMapOfBlockedIps::iterator ipAddrIter = m_mapOfBlockedIps.find(ipAddr);
   if(ipAddrIter != m_mapOfBlockedIps.end())
   {
      tIpBlockListOfPlotNames::iterator iter = ipAddrIter.value().begin();
      while(iter != ipAddrIter.value().end())
      {
         if(*iter == plotName)
         {
            ipAddrIter.value().erase(iter);
            break;
         }
         else
         {
            ++iter;
         }
      }
      // If we removed the last Plot Name for the given IP Addr, remove the IP Addr from the map.
      if(ipAddrIter.value().size() == 0)
      {
         m_mapOfBlockedIps.erase(ipAddrIter);
      }
   }
}

void ipBlocker::clearBlockList()
{
   QMutexLocker locker(&m_mutex);
   m_mapOfBlockedIps.clear();
}

ipBlocker::tMapOfBlockedIps& ipBlocker::getBlockList()
{
   QMutexLocker locker(&m_mutex);

   return m_mapOfBlockedIps;
}

bool ipBlocker::isInBlockList(tPlotterIpAddr ipAddr, QString plotName)
{
   QMutexLocker locker(&m_mutex);

   return m_mapOfBlockedIps.contains(ipAddr) &&
          ( m_mapOfBlockedIps[ipAddr].size() == 0 || // Empty list means block all.
            m_mapOfBlockedIps[ipAddr].contains(plotName) );
}

