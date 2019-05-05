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
#ifndef FFTMEASCHILDPARAM_H
#define FFTMEASCHILDPARAM_H

#include <QString>
#include "PlotHelperTypes.h"

typedef struct fftMeasChildKey
{
   QString parentPlotName;
   QString childPlotName;

   // Constructors
   fftMeasChildKey(void) :
      parentPlotName(""),
      childPlotName("")
   {
   }

   fftMeasChildKey(QString parentPlotName_in, QString childPlotName_in) :
      parentPlotName(parentPlotName_in),
      childPlotName(childPlotName_in)
   {
   }

   bool operator==(const struct fftMeasChildKey& rhs)
   {
       return (parentPlotName == rhs.parentPlotName) &&
              (childPlotName  == rhs.childPlotName );
   }
   bool operator!=(const struct fftMeasChildKey& rhs){ return !operator==(rhs); }

   bool operator<(const struct fftMeasChildKey& rhs) const
   {
      if(parentPlotName != rhs.parentPlotName)
      {
         return parentPlotName < rhs.parentPlotName;
      }
      return childPlotName < rhs.childPlotName;
   }

}tFftMeasChildKey;

typedef struct fftMeasChildParam
{
   QString childCurveName;
   PlotMsgIdType groupMsgId;
   int childCurveSize;
   int childPointIndex;

   // Constructors
   fftMeasChildParam(void) :
      childCurveName(""),
      groupMsgId(0),
      childCurveSize(0),
      childPointIndex(0)
   {
   }

   fftMeasChildParam( QString childCurveName_in,
                      PlotMsgIdType groupMsgId_in,
                      int childCurveSize_in,
                      int childPointIndex_in) :
      childCurveName(childCurveName_in),
      groupMsgId(groupMsgId_in),
      childCurveSize(childCurveSize_in),
      childPointIndex(childPointIndex_in)
   {
   }

}tFftMeasChildParam;

bool fftMeasChildParam_getIndex( const QString& parentPlotName, 
                                 const QString& childPlotName, 
                                 const QString& childCurveName, 
                                 PlotMsgIdType groupMsgId, 
                                 int childCurveSize, 
                                 int& childPointIndex ); // Index is read / write

void fftMeasChildParam_plotRemoved(const QString& plotName);



#endif // FFTMEASCHILDPARAM_H
