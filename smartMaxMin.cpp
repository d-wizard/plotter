/* Copyright 2015 Dan Williams. All Rights Reserved.
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
#include <math.h>
#include <algorithm>
#include <assert.h>
#include "smartMaxMin.h"

smartMaxMin::smartMaxMin(const dubVect *srcVect, unsigned int maxSegSize):
   m_maxSegSize(maxSegSize),
   m_srcVect(srcVect),
   m_curMax(0.0),
   m_curMin(0.0)
{
}

smartMaxMin::~smartMaxMin()
{

}


void smartMaxMin::updateMaxMin(unsigned int startIndex, unsigned int numPoints)
{
   if(m_srcVect->size() < (startIndex + numPoints))
   {
      assert(0);
      return;
   }

   unsigned int srcVectSize = m_srcVect->size();

   unsigned int newSegStartIndex = startIndex;
   unsigned int newSetNumPoints = srcVectSize - startIndex;
   bool overlapFound = false;

   // Find and remove any segments that overlap with the new segment.
   tSegList::iterator iter = m_segList.begin();
   while(iter != m_segList.end())
   {
      if(overlapFound == false)
      {
         if( (startIndex >= iter->startIndex) && (startIndex < (iter->startIndex + iter->numPoints)) )
         {
            overlapFound = true;
            newSegStartIndex = iter->startIndex;
            m_segList.erase(iter++);
         }
         else
         {
            ++iter;
         }
      }
      else
      {
         if(iter->startIndex >= (startIndex + numPoints))
         {
            newSetNumPoints = iter->startIndex - newSegStartIndex;

            // Overlap is over. Break out of while early.
            break;
         }
         else
         {
            // This segment is fully encapsulated by the new points.
            m_segList.erase(iter++);
         }
      }

      // If this is the last segment and we are still overlapping,
      // set the update size to go to the end of the vector.
      if(iter == m_segList.end() && overlapFound == true)
      {
         newSetNumPoints = srcVectSize - newSegStartIndex;
      }
   }

   unsigned int nextSegStartIndex = newSegStartIndex;
   unsigned int pointsRemaining = newSetNumPoints;
   while(pointsRemaining > 0)
   {
      tMaxMinSegment newSeg;
      unsigned int calcSize = std::min(pointsRemaining, m_maxSegSize);

      calcMaxMinOfSeg(nextSegStartIndex, calcSize, newSeg);
      m_segList.push_back(newSeg);

      nextSegStartIndex += calcSize;
      pointsRemaining -= calcSize;
   }

   m_segList.sort();
   calcTotalMaxMin();
}

void smartMaxMin::scrollModeShift(unsigned int shiftAmount)
{
}

void smartMaxMin::getMaxMin(double &retMax, double &retMin)
{
   retMax = m_curMax;
   retMin = m_curMin;
}

void smartMaxMin::calcTotalMaxMin()
{
   tSegList::iterator iter = m_segList.begin();
   if(iter != m_segList.end())
   {
      double newMax = iter->maxValue;
      double newMin = iter->minValue;
      ++iter;

      while(iter != m_segList.end())
      {
         if(iter->maxValue > newMax)
         {
            newMax = iter->maxValue;
         }
         if(iter->minValue < newMin)
         {
            newMin = iter->minValue;
         }
         ++iter;
      }

      m_curMax = newMax;
      m_curMin = newMin;
   }
}


// Find the max and min in the vector, but only allow real numbers to be used
// to determine the max and min (ingore values that are not real, i.e 'Not a Number'
// and Positive Infinity or Negative Inifinty)
void smartMaxMin::calcMaxMinOfSeg(unsigned int startIndex, unsigned int numPoints, tMaxMinSegment& seg)
{
   // Initialize max and min to +/- 1 just in case all the input points are not real numbers.
   seg.maxValue = 1;
   seg.minValue = -1;
   seg.maxIndex = -1;
   seg.minIndex = -1;
   seg.startIndex = startIndex;
   seg.numPoints = numPoints;

   const double* srcPoints = &((*m_srcVect)[0]);
   unsigned int stopIndex = startIndex + numPoints;

   // Find first point that is a real number.
   for(unsigned int i = startIndex; i < stopIndex; ++i)
   {
      if(isfinite(srcPoints[i])) // Only allow real numbers.
      {
         seg.maxValue = srcPoints[i];
         seg.minValue = srcPoints[i];
         seg.maxIndex = i;
         seg.minIndex = i;
         startIndex = i+1;
         break;
      }
   }

   // Loop through the input value to find the max and min.
   for(unsigned int i = startIndex; i < stopIndex; ++i)
   {
      if(isfinite(srcPoints[i])) // Only allow real numbers.
      {
         if(seg.minValue > srcPoints[i])
         {
            seg.minValue = srcPoints[i];
            seg.minIndex = i;
         }
         if(seg.maxValue < srcPoints[i])
         {
            seg.maxValue = srcPoints[i];
            seg.maxIndex = i;
         }
      }
   }
}
