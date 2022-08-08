/* Copyright 2015 - 2019, 2021 - 2022 Dan Williams. All Rights Reserved.
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

smartMaxMin::smartMaxMin(const dubVect *srcVect, unsigned int minSegSize, unsigned int maxSegSize):
   m_minSegSize(minSegSize),
   m_maxSegSize(maxSegSize),
   m_srcVect(srcVect),
   m_curMax(0.0),
   m_curMin(0.0),
   m_curMaxMinHasRealPoints(false),
   m_firstRealPointIndex(-1),
   m_lastRealPointIndex(-1)
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
   unsigned int newSegNumPoints = srcVectSize - startIndex;
   bool overlapFound = false;

   unsigned int lastSegEndIndex = 0;

   // Find and remove any segments that overlap with the new segment.
   tSegList::iterator iter = m_segList.begin();
   while(iter != m_segList.end())
   {
      if(overlapFound == false)
      {

         lastSegEndIndex = iter->startIndex + iter->numPoints;

         if( ((int)startIndex >= iter->startIndex) && (startIndex < lastSegEndIndex) )
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
         if(iter->startIndex >= (int)(startIndex + numPoints))
         {
            newSegNumPoints = iter->startIndex - newSegStartIndex;

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
         newSegNumPoints = srcVectSize - newSegStartIndex;
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
      newSeg.realPoints = true;
      newSeg.firstRealPointIndex = newSeg.startIndex;
      newSeg.lastRealPointIndex = newSeg.startIndex + newSeg.numPoints - 1;
      m_segList.push_back(newSeg);
   }

   int nextSegStartIndex = newSegStartIndex;
   int pointsRemaining = newSegNumPoints;
   while(pointsRemaining > 0)
   {
      tMaxMinSegment newSeg;
      unsigned int calcSize = std::min(pointsRemaining, m_maxSegSize);

      calcMaxMinOfSeg(m_srcVect->data(), nextSegStartIndex, calcSize, newSeg);
      m_segList.push_back(newSeg);

      nextSegStartIndex += calcSize;
      pointsRemaining -= calcSize;
   }

   m_segList.sort();

   combineSegments();
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
         // Part of the segment has been shifted out. Re-calc min/max on the remaining points.
         calcMaxMinOfSeg(m_srcVect->data(), 0, numPoints + numPointsErased - shiftAmount, *iter);
         ++iter;
      }
      else
      {
         // All of the samples in this segment will remain. Just need to shift all the indexes
         // into the curve data.
         iter->startIndex -= shiftAmount;
         iter->maxIndex -= shiftAmount;
         iter->minIndex -= shiftAmount;
         iter->firstRealPointIndex -= shiftAmount;
         iter->lastRealPointIndex  -= shiftAmount;

         ++iter;
      }
   }

   // Segments might have changed, update min / max.
   calcTotalMaxMin();

   //debug_verifySegmentsAreContiguous();
}

void smartMaxMin::getMaxMin(double &retMax, double &retMin, bool& retReal)
{
   retMax  = m_curMax;
   retMin  = m_curMin;
   retReal = m_curMaxMinHasRealPoints;
}

void smartMaxMin::getFirstLastReal(int& firstRealPointIndex, int& lastRealPointIndex)
{
   firstRealPointIndex = m_firstRealPointIndex;
   lastRealPointIndex  = m_lastRealPointIndex;
}

void smartMaxMin::handleShortenedNumPoints()
{
   int srcVectSize = m_srcVect->size();
   tSegList::iterator iter = m_segList.begin();
   while(iter != m_segList.end())
   {
      if(iter->startIndex >= srcVectSize)
      {
         m_segList.erase(iter++);
      }
      else
      {
         if( (iter->startIndex + iter->numPoints) >= srcVectSize )
         {
            calcMaxMinOfSeg(m_srcVect->data(), iter->startIndex, srcVectSize - iter->startIndex, *iter);
         }
         iter++;
      }
   }

   // Segments might have changed, update min / max.
   calcTotalMaxMin();
}

tMaxMinSegment smartMaxMin::getMinMaxOfSubrange(unsigned int start, unsigned int numPoints)
{
   tSegList::iterator iter = m_segList.begin();
   return getMinMaxOfSubrange(start, numPoints, iter);
}

tMaxMinSegment smartMaxMin::getMinMaxOfSubrange(unsigned int start, unsigned int numPoints, tSegList::iterator& iterInOut)
{
   tMaxMinSegment retVal;
   retVal.startIndex = 0;
   retVal.numPoints = 0;

   bool segFound = false;

   int stop = start + numPoints;
   tSegList::iterator iter = iterInOut;
   if(iter != m_segList.end())
   {
      while(iter != m_segList.end())
      {
         int segStop = iter->startIndex + iter->numPoints;
         if(iter->startIndex >= (int)start)
         {
            if(segStop <= stop)
            {
               // Segment is fully contained in the input range. Add it to the retVal.
               if(!segFound)
               {
                  segFound = true;
                  retVal = *iter;
               }
               else
               {
                  retVal.numPoints += iter->numPoints;

                  if(iter->maxValue > retVal.maxValue)
                  {
                     retVal.maxValue = iter->maxValue;
                     retVal.maxIndex = iter->maxIndex;
                  }
                  if(iter->minValue < retVal.minValue)
                  {
                     retVal.minValue = iter->minValue;
                     retVal.minIndex = iter->minIndex;
                  }

                  if(!retVal.realPoints && iter->realPoints)
                  {
                     // New segment has first real points.
                     retVal.realPoints = true;
                     retVal.firstRealPointIndex = iter->firstRealPointIndex;
                     retVal.lastRealPointIndex = iter->lastRealPointIndex;
                  }
                  else if(iter->realPoints)
                  {
                     retVal.lastRealPointIndex = iter->lastRealPointIndex;
                  }
               }
            }
            else
            {
               break;
            }
         }
         iterInOut = iter;
         ++iter;
      }
   }
   return retVal;
}

void smartMaxMin::calcTotalMaxMin()
{
   tSegList::iterator iter = m_segList.begin();
   if(iter != m_segList.end())
   {
      double newMax = iter->maxValue;
      double newMin = iter->minValue;
      bool maxMinIsRealValue = iter->realPoints;
      int firstRealPointIndex = iter->firstRealPointIndex;
      int lastRealPointIndex  = iter->lastRealPointIndex;

      ++iter;

      while(iter != m_segList.end())
      {
         // Only update Max/Min if the new segment contains real numbers.
         if(iter->realPoints)
         {
            if(!maxMinIsRealValue)
            {
               // Old segment(s) did not contain real numbers. New segment has real numbers. So, use the new segment values.
               newMax = iter->maxValue;
               newMin = iter->minValue;
            }
            else
            {
               if(iter->maxValue > newMax)
               {
                  newMax = iter->maxValue;
               }
               if(iter->minValue < newMin)
               {
                  newMin = iter->minValue;
               }
            }
            maxMinIsRealValue = true;

            // Determine first / last real point index.
            if(firstRealPointIndex < 0)
            {
               firstRealPointIndex = iter->firstRealPointIndex;
            }
            lastRealPointIndex = iter->lastRealPointIndex;
         }

         ++iter;
      }

      m_curMax = newMax;
      m_curMin = newMin;
      m_curMaxMinHasRealPoints = maxMinIsRealValue;
      m_firstRealPointIndex = firstRealPointIndex;
      m_lastRealPointIndex = lastRealPointIndex;
   }
}


// Find the max and min in the vector, but only allow real numbers to be used
// to determine the max and min (ingore values that are not real, i.e 'Not a Number'
// and Positive Infinity or Negative Inifinty)
void smartMaxMin::calcMaxMinOfSeg(const double* srcPoints, unsigned int startIndex, unsigned int numPoints, tMaxMinSegment& seg)
{
   // Initialize max and min to +/- 1 just in case all the input points are not real numbers.
   seg.maxValue = 1;
   seg.minValue = -1;
   seg.maxIndex = -1;
   seg.minIndex = -1;
   seg.startIndex = startIndex;
   seg.numPoints = numPoints;
   seg.realPoints = false;
   seg.firstRealPointIndex = -1;
   seg.lastRealPointIndex  = -1;

   unsigned int stopIndex = startIndex + numPoints;

   // Find first point that is a real number.
   for(unsigned int i = startIndex; i < stopIndex; ++i)
   {
      if(isDoubleValid(srcPoints[i])) // Only allow real numbers.
      {
         seg.maxValue = srcPoints[i];
         seg.minValue = srcPoints[i];
         seg.maxIndex = i;
         seg.minIndex = i;
         seg.realPoints = true;
         seg.firstRealPointIndex = i;
         seg.lastRealPointIndex  = i;  // This will get updated if a subsequent point is valid.
         startIndex = i+1;
         break;
      }
   }

   // Loop through the input value to find the max and min.
   if(seg.realPoints)
   {
      for(unsigned int i = startIndex; i < stopIndex; ++i)
      {
         if(isDoubleValid(srcPoints[i])) // Only allow real numbers.
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
            seg.lastRealPointIndex = i;
         }
      }
   }
}

// Combine two segments. Result will be filled into seg1.
// seg1 should be before seg2 in the source data (i.e. seg1.startIndex+seg1.numPoints <= seg2.startIndex)
void smartMaxMin::combineSegments(tMaxMinSegment& seg1, const tMaxMinSegment& seg2)
{
   // First, update the Max/Min values and their indexes.
   if(seg1.realPoints && seg2.realPoints)
   {
      // Both segments contain real values, simply find the min and max between the two segments.
      if(seg1.maxValue < seg2.maxValue)
      {
         seg1.maxValue = seg2.maxValue;
         seg1.maxIndex = seg2.maxIndex;
      }
      if(seg1.minValue > seg2.minValue)
      {
         seg1.minValue = seg2.minValue;
         seg1.minIndex = seg2.minIndex;
      }
      seg1.lastRealPointIndex = seg2.lastRealPointIndex; // seg2 is getting combined into seg1.
   }
   else if(seg2.realPoints)
   {
      // seg1 is not real but seg2 is, so just use seg2 values
      seg1.maxValue = seg2.maxValue;
      seg1.maxIndex = seg2.maxIndex;
      seg1.minValue = seg2.minValue;
      seg1.minIndex = seg2.minIndex;
      seg1.firstRealPointIndex = seg2.firstRealPointIndex;
      seg1.lastRealPointIndex  = seg2.lastRealPointIndex;
   }
   // else seg1 is real and seg2 isn't (thus keep seg1 the same) or both are not real (also keep seg1 the same)

   // Next, update the variable that indicates if any of the points in the segment contain real values.
   if(seg1.realPoints || seg2.realPoints)
   {
      seg1.realPoints = true;
   }

   // Finally, update the number of points in the new, combined segment.
   seg1.numPoints += seg2.numPoints;
}

// Combine small segments.
void smartMaxMin::combineSegments()
{
   // All the segments must be contiguous after this point.
   //debug_verifySegmentsAreContiguous();
   //debug_verifyAllPointsAreInList();

   tSegList::iterator cur = m_segList.begin();
   tSegList::iterator next;
   while(cur != m_segList.end())
   {
      next = cur;
      ++next;

      if(next != m_segList.end())
      {
         // Check if the two segments together are small enough to combine.
         if( (cur->numPoints + next->numPoints) < m_minSegSize )
         {
            // Combine cur and next then erase next.
            combineSegments(*cur, *next);
            m_segList.erase(next);
         }
         else
         {
            // Segments aren't small enough to combine. Move on to the next pair of segments to check.
            ++cur;
         }
      }
      else
      {
         // Next segment is the end of the list. Increment cur to exit the loop.
         ++cur;
      }
   }


   //debug_verifySegmentsAreContiguous();
   //debug_verifyAllPointsAreInList();
}

void smartMaxMin::debug_verifySegmentsAreContiguous()
{
   int nextSegExpectedStart = 0;
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

void smartMaxMin::getSegmentsInRange(double start, double stop, tSegList& full, std::vector<unsigned int>& partial)
{
   full.clear();
   partial.clear();
   tSegList::iterator iter = m_segList.begin();
   while(iter != m_segList.end())
   {
      if(iter->realPoints)
      {
         bool minInRange = (iter->minValue >= start && iter->minValue <= stop);
         bool maxInRange = (iter->maxValue >= start && iter->maxValue <= stop);
         bool rangeContainedInThisSeg = (iter->maxValue >= stop && iter->minValue <= start);
         if(minInRange && maxInRange)
         {
            full.push_back(*iter); // The whole segment is in range.
         }
         else if(minInRange || maxInRange || rangeContainedInThisSeg)
         {
            // Only some points are in range. Store off the points that are in range.
            for(int i = iter->firstRealPointIndex; i <= iter->lastRealPointIndex; ++i)
            {
               double point = (*m_srcVect)[i];
               if(point >= start && point <= stop)
               {
                  partial.push_back(i);
               }
            }
         }
      }
      ++iter;
   }
}

void smartMaxMin::getMaxMinFromSegments(tSegList& fullIn, std::vector<unsigned int>& partial, double &retMax, double &retMin, bool& retReal)
{
   retReal = false;
   retMax = 0;
   retMin = 0;
   
   // First deal with the full matching ranges.
   tSegList::iterator fullIter = fullIn.begin();
   while(fullIter != fullIn.end())
   {
      tSegList::iterator thisIter = m_segList.begin();
      int matchingMinIndex = -1;
      int matchingMaxIndex = 0;
      while(thisIter != m_segList.end())
      {
         bool startInRange = thisIter->startIndex >= fullIter->startIndex;
         bool stopInRange  = (thisIter->startIndex+thisIter->numPoints) <= (fullIter->startIndex+fullIter->numPoints);

         if(startInRange && stopInRange)
         {
            // The entire input segment is contained within this segment.
            if(thisIter->realPoints)
            {
               if(!retReal)
               {
                  retReal = true;
                  retMax = thisIter->maxValue;
                  retMin = thisIter->minValue;
               }
               else
               {
                  retMax = std::max(retMax, thisIter->maxValue);
                  retMin = std::min(retMin, thisIter->minValue);
               }
            }

            if(matchingMinIndex < 0)
               matchingMinIndex = thisIter->minIndex;
            matchingMaxIndex = thisIter->maxIndex;
         }
         ++thisIter;
      }

      // Grab points that weren't whole segments.
      for(int i = fullIter->minIndex; i < matchingMinIndex; ++i)
      {
         double point = (*m_srcVect)[i];
         if(isDoubleValid(point))
         {
            if(!retReal)
            {
               retReal = true;
               retMax = point;
               retMin = point;
            }
            else
            {
               retMax = std::max(retMax, point);
               retMin = std::min(retMin, point);
            }
         }
      }
      for(int i = matchingMaxIndex+1; i < fullIter->maxIndex; ++i)
      {
         double point = (*m_srcVect)[i];
         if(isDoubleValid(point))
         {
            if(!retReal)
            {
               retReal = true;
               retMax = point;
               retMin = point;
            }
            else
            {
               retMax = std::max(retMax, point);
               retMin = std::min(retMin, point);
            }
         }
      }

      ++fullIter;
   } // End while(fullIter != fullIn.end())

   // Account for the parial points.
   size_t numPartial = partial.size();
   unsigned int* partialIndex = partial.data();
   for(size_t i = 0; i < numPartial; ++i)
   {
      double point = (*m_srcVect)[partialIndex[i]];
      if(isDoubleValid(point))
      {
         if(!retReal)
         {
            retReal = true;
            retMax = point;
            retMin = point;
         }
         else
         {
            retMax = std::max(retMax, point);
            retMin = std::min(retMin, point);
         }
      }
   }
}



fastMonotonicMaxMin::fastMonotonicMaxMin(smartMaxMin& parent):
   m_parent(parent),
   m_parentIter(m_parent.getBeginIter())
{

}

tMaxMinSegment fastMonotonicMaxMin::getMinMaxInRange(unsigned int start, unsigned int len)
{
   tMaxMinSegment seg = m_parent.getMinMaxOfSubrange(start, len, m_parentIter);
   const double* srcData = m_parent.getSrcVect()->data();
   tMaxMinSegment retValSeg;

   if(seg.numPoints > 0)
   {
      // Got some points from smartMaxMin. Include the samples before and after the points from smartMaxMin.
      tMaxMinSegment beforeSeg, afterSeg;
      int beforeSegLen = seg.startIndex - start;
      int afterSegLen = len - beforeSegLen - seg.numPoints;

      // Get the segments before and after.
      smartMaxMin::calcMaxMinOfSeg(srcData, start, beforeSegLen, beforeSeg);
      smartMaxMin::calcMaxMinOfSeg(srcData, seg.startIndex+seg.numPoints, afterSegLen, afterSeg);

      // Combine the segments (Be careful of the order. Needs to be beforeSeg, then seg, then afterSeg).
      retValSeg = beforeSeg;
      smartMaxMin::combineSegments(retValSeg, seg);
      smartMaxMin::combineSegments(retValSeg, afterSeg);
   }
   else
   {
      // No segments in smartMaxMin were fully contained in the desire range. Manually do the min/max measurement.
      smartMaxMin::calcMaxMinOfSeg(m_parent.getSrcVect()->data(), start, len, retValSeg);
   }

   return retValSeg;
}

