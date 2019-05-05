/* Copyright 2019 Dan Williams. All Rights Reserved.
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
#include <QMap>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include "fftMeasChildParam.h"

static QMap<tFftMeasChildKey, tFftMeasChildParam> g_map;
static QMutex g_mutex;

bool fftMeasChildParam_getIndex( const QString& parentPlotName, 
                                 const QString& childPlotName, 
                                 const QString& childCurveName, // Is this needed? 
                                 PlotMsgIdType groupMsgId, 
                                 int childCurveSize, 
                                 int& childPointIndex ) // Index is read / write
{
   QMutexLocker lock(&g_mutex); // Keep changes to the Map thread safe.
   bool matchFound = false;

   tFftMeasChildKey key(parentPlotName, childPlotName);

   bool keyExists = g_map.find(key) != g_map.end();
   if(keyExists)
   {
      // A matching Parent Plot / Child Plot Name exists.
      tFftMeasChildParam matchValue = g_map[key];
      bool newChildPlot = groupMsgId < 0; // Negative Group Message ID indicates that this is a new child plot.

      // Check for situation where we want to reuse the previous index.
      if( (matchValue.groupMsgId == groupMsgId || newChildPlot) && matchValue.childCurveSize == childCurveSize &&
          matchValue.childPointIndex < childCurveSize && matchValue.childCurveSize >= 0 )
      {
         childPointIndex = matchValue.childPointIndex;
         matchFound = true;
      }
   }

   if(matchFound == false)
   {
      tFftMeasChildParam newValue(childCurveName, groupMsgId, childCurveSize, childPointIndex);
      g_map[key] = newValue;
   }

   return matchFound;
}

void fftMeasChildParam_plotRemoved(const QString& plotName)
{
   QMutexLocker lock(&g_mutex); // Keep changes to the Map thread safe.

   // Determine which map keys to remove (i.e. the ones that contain the plot that is being removed).
   QVector<tFftMeasChildKey> keysToRemove;
   foreach(tFftMeasChildKey key, g_map.keys())
   {
      if(key.parentPlotName == plotName || key.childPlotName == plotName)
      {
         keysToRemove.push_back(key);
      }
   }

   // Remove the (now) invalid keys from the map.
   for(int i = 0; i < keysToRemove.size(); ++i)
   {
      g_map.remove(keysToRemove[i]);
   }
}
