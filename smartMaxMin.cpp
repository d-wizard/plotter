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
#include <stdio.h>
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
   unsigned int srcVectSize = m_srcVect->size();
   if(srcVectSize < (startIndex + numPoints))
   {
      assert(0);
      return;
   }

   unsigned int newSegStartIndex = startIndex;
   unsigned int newSetNumPoints = srcVectSize - startIndex;
   bool overlapFound = false;

   unsigned int lastSegEndIndex = 0;

   // Find and remove any segments that overlap with the new segment.
   tSegList::iterator iter = m_segList.begin();
   while(iter != m_segList.end())
   {
      if(overlapFound == false)
      {

         lastSegEndIndex = iter->startIndex + iter->numPoints;

         if( (startIndex >= iter->startIndex) && (startIndex < lastSegEndIndex) )
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

   // If there is no overlap between the new samples and the old samples, then
   // the new samples are ahead of the old samples. Check if there is a gap between
   // the new samples and the old samples.
   if(overlapFound == false && lastSegEndIndex < startIndex)
   {
      // There is a gap between the new samples and the old samples.
      // It is safe to assume all the samples in the gap are zero.
      // Create a new Segment with the max and min set to 0.
      tMaxMinSegment newSeg;
      newSeg.maxValue = 0;
      newSeg.minValue = 0;
      newSeg.startIndex = lastSegEndIndex;
      newSeg.numPoints = startIndex - lastSegEndIndex;
      newSeg.maxIndex = newSeg.startIndex;
      newSeg.minIndex = newSeg.startIndex;
      m_segList.push_back(newSeg);
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

   //debug_verifyAllPointsAreInList();
}

void smartMaxMin::scrollModeShift(unsigned int shiftAmount)
{
   unsigned int numPointsErased = 0;
   tSegList::iterator iter = m_segList.begin();
   while(iter != m_segList.end())
   {
      unsigned int startIndex = iter->startIndex;
      unsigned int numPoints = iter->numPoints;

      if( (startIndex + numPoints) <= shiftAmount)
      {
         // All samples have been shifted out.
         m_segList.erase(iter++);
         numPointsErased += numPoints;
      }
      else if(startIndex < shiftAmount)
      {
         calcMaxMinOfSeg(0, numPoints + numPointsErased - shiftAmount, *iter);
         ++iter;
      }
      else
      {
         iter->startIndex -= shiftAmount;
         ++iter;
      }
   }

   //debug_verifySegmentsAreContiguous();
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

void smartMaxMin::debug_verifySegmentsAreContiguous()
{
   unsigned int nextSegExpectedStart = 0;
   tSegList::iterator iter = m_segList.begin();
   while(iter != m_segList.end())
   {
      if(nextSegExpectedStart != iter->startIndex)
      {
         printf("Gap Between Segments\n");
      }

      nextSegExpectedStart = iter->startIndex + iter->numPoints;

      ++iter;
   }
}

void smartMaxMin::debug_verifyAllPointsAreInList()
{
   unsigned int srcSize = m_srcVect->size();
   unsigned int nextSegExpectedStart = 0;
   tSegList::iterator iter = m_segList.begin();
   while(iter != m_segList.end())
   {
      unsigned int startIndex = iter->startIndex;
      unsigned int numPoints = iter->numPoints;
      unsigned int endIndex = startIndex + numPoints;

      if(nextSegExpectedStart != startIndex)
      {
         printf("Gap Between Segments\n");
      }

      nextSegExpectedStart = endIndex;

      ++iter;
      if(iter == m_segList.end())
      {
         if(endIndex != srcSize)
         {
            printf("Gap Between Last Segement and End of Samples\n");
         }
      }
   }
}
