/* Copyright 2015 - 2017 Dan Williams. All Rights Reserved.
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
   unsigned int startIndex;
   unsigned int numPoints;
   unsigned int maxIndex;
   double maxValue;
   unsigned int minIndex;
   double minValue;
   bool realPoints; // If true at least 1 of the points in the segment is a real, valid value.


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

   void handleShortenedNumPoints();
private:
   smartMaxMin();

   void calcTotalMaxMin();
   void calcMaxMinOfSeg(unsigned int startIndex, unsigned int numPoints, tMaxMinSegment& seg);

   void combineSegments();

   void debug_verifySegmentsAreContiguous();
   void debug_verifyAllPointsAreInList();

   unsigned int m_minSegSize;
   unsigned int m_maxSegSize;

   const dubVect* m_srcVect;
   tSegList m_segList;
   double m_curMax;
   double m_curMin;
   bool m_curMaxMinHasRealPoints;
};

#endif // SMARTMAXMIN_H
