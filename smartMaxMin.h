/* Copyright 2015 - 2017, 2019, 2021 Dan Williams. All Rights Reserved.
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
#ifndef SMARTMAXMIN_H
#define SMARTMAXMIN_H

#include <list>
#include "PlotHelperTypes.h"

typedef struct MaxMinSegment
{
   int startIndex;
   int numPoints;
   int maxIndex;
   double maxValue;
   int minIndex;
   double minValue;
   bool realPoints; // If true at least 1 of the points in the segment is a real, valid value.
   int firstRealPointIndex;
   int lastRealPointIndex;


   bool operator>(const struct MaxMinSegment& rhs)
   {
       return (startIndex > rhs.startIndex);
   }
   bool operator<(const struct MaxMinSegment& rhs)
   {
       return (startIndex < rhs.startIndex);
   }
   bool operator>=(const struct MaxMinSegment& rhs)
   {
       return (startIndex >= rhs.startIndex);
   }
   bool operator<=(const struct MaxMinSegment& rhs)
   {
       return (startIndex <= rhs.startIndex);
   }
}tMaxMinSegment;

typedef std::list<tMaxMinSegment> tSegList;

class smartMaxMin
{
public:
   smartMaxMin(const dubVect* srcVect, unsigned int minSegSize, unsigned int maxSegSize);
   ~smartMaxMin();

   void updateMaxMin(unsigned int startIndex, unsigned int numPoints);
   void scrollModeShift(unsigned int shiftAmount);

   void getMaxMin(double& retMax, double& retMin, bool& retReal);
   void getFirstLastReal(int& firstRealPointIndex, int& lastRealPointIndex);

   void handleShortenedNumPoints();

   tMaxMinSegment getMinMaxOfSubrange(unsigned int start, unsigned int numPoints);
   tMaxMinSegment getMinMaxOfSubrange(unsigned int start, unsigned int numPoints, tSegList::iterator& iterInOut);
   tSegList::iterator getBeginIter(){return m_segList.begin();}
   const dubVect* getSrcVect(){return m_srcVect;}

   static void calcMaxMinOfSeg(const double* srcPoints, unsigned int startIndex, unsigned int numPoints, tMaxMinSegment& seg);
   static void combineSegments(tMaxMinSegment& seg1, const tMaxMinSegment& seg2); // seg1 is input and the return value (i.e. the combined version)

private:
   smartMaxMin();
   smartMaxMin(smartMaxMin const&);
   void operator=(smartMaxMin const&);

   void calcTotalMaxMin();

   void combineSegments();

   void debug_verifySegmentsAreContiguous();
   void debug_verifyAllPointsAreInList();

   int m_minSegSize;
   int m_maxSegSize;

   const dubVect* m_srcVect;
   tSegList m_segList;
   double m_curMax;
   double m_curMin;
   bool m_curMaxMinHasRealPoints;
   int m_firstRealPointIndex;
   int m_lastRealPointIndex;
};


// This can be used to more quickly determine the min/max of subranges of plot data. This can only be used if
// the subranges monotonically increasing up through the plot data.
class fastMonotonicMaxMin
{
public:
   fastMonotonicMaxMin(smartMaxMin& parent);

   tMaxMinSegment getMinMaxInRange(unsigned int start, unsigned int len);

private:
   fastMonotonicMaxMin();
   fastMonotonicMaxMin(fastMonotonicMaxMin const&);
   void operator=(fastMonotonicMaxMin const&);

   smartMaxMin& m_parent;
   tSegList::iterator m_parentIter;

};

#endif // SMARTMAXMIN_H
