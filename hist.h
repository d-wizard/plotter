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
#ifndef hist_h
#define hist_h

#include <list>

template <class HistClassType>
class DataHistogram
{
   typedef struct
   {
      HistClassType value;
      int count;
   } tHistValueCountPair;

public:
   void addNewEntry(const HistClassType& newEntry)
   {
      if(m_histValues.size() == 0)
      {
         tHistValueCountPair t_histPairt = {newEntry, 1};
         m_histValues.push_back(t_histPairt);
         m_maxCount = 1;
         m_maxValue = newEntry;
         m_totalEntries = 1;
      }
      else
      {
         bool b_matchFound = false;
         typename std::list<tHistValueCountPair>::iterator t_histIter;

         for(t_histIter = m_histValues.begin(); t_histIter != m_histValues.end(); ++t_histIter)
         {
            if(t_histIter->value == newEntry)
            {
               if(++t_histIter->count > m_maxCount)
               {
                  m_maxCount = t_histIter->count;
                  m_maxValue = newEntry;
               }
               b_matchFound = true;
               break;
            }
         }

         if(b_matchFound == false)
         {
            tHistValueCountPair t_histPairt = {newEntry, 1};
            m_histValues.push_back(t_histPairt);
         }
         ++m_totalEntries;

         m_histValues.sort(compareEntries);
      }
   }
   
   static bool compareEntries(tHistValueCountPair entry1, tHistValueCountPair entry2)
   {
      if(entry1.count > entry2.count)
         return true;
      else
         return false;
   }

   HistClassType getMaxEntryValue(void)
   {
      return m_maxValue;
   }

private:
   std::list<tHistValueCountPair> m_histValues;
   int m_totalEntries;
   int m_maxCount;
   HistClassType m_maxValue;

};

#endif
