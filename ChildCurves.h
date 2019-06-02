/* Copyright 2014, 2016, 2019 Dan Williams. All Rights Reserved.
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
#ifndef ChildCurves_h
#define ChildCurves_h

#include <QWidget>
#include <QString>
#include <QVector>
#include <PlotHelperTypes.h>
#include "CurveData.h"

class CurveCommander;

class ChildCurve : public QWidget
{
   Q_OBJECT
public:
   ChildCurve( CurveCommander* curveCmdr,
               QString plotName,
               QString curveName,
               ePlotType plotType,
               bool forceContiguousParentPoints,
               bool startChildInScrollMode,
               tParentCurveInfo yAxis);

   ChildCurve( CurveCommander* curveCmdr,
               QString plotName,
               QString curveName,
               ePlotType plotType,
               bool forceContiguousParentPoints,
               bool startChildInScrollMode,
               tParentCurveInfo xAxis,
               tParentCurveInfo yAxis);

   void anotherCurveChanged( QString plotName,
                             QString curveName,
                             unsigned int parentStartIndex,
                             unsigned int parentNumPoints,
                             PlotMsgIdType parentGroupMsgId,
                             PlotMsgIdType parentCurveMsgId );

   void setToParentsSampleRate();
   QVector<tPlotCurveAxis> getParents();

   QString getPlotName(){return m_plotName;}
   QString getCurveName(){return m_curveName;}

private:

   // Types
   typedef struct
   {
      bool xParentChanged;
      bool yParentChanged;
      unsigned int parentStartIndex;
      unsigned int parentStopIndex;
   }tParentUpdateChunk;

   // Eliminate default, copy, assign
   ChildCurve();
   ChildCurve(ChildCurve const&);
   void operator=(ChildCurve const&);

   void getParentUpdateInfo( tParentCurveInfo &parentInfo,
                             unsigned int parentStartIndex,
                             unsigned int parentStopIndex,
                             bool parentChanged,
                             CurveData*& parentCurve,
                             int& origStartIndex,
                             int& startIndex,
                             int& stopIndex,
                             int& scrollModeShift);

   unsigned int getDataFromParent1D( unsigned int parentStartIndex = 0,
                                     unsigned int parentStopIndex = 0);

   unsigned int getDataFromParent2D( bool xParentChanged,
                                     bool yParentChanged,
                                     unsigned int parentStartIndex = 0,
                                     unsigned int parentStopIndex = 0);

   void getDataForFft( ePlotType fftType, 
                       PlotMsgIdType parentGroupMsgId,
                       bool xParentChanged,
                       bool yParentChanged,
                       unsigned int parentStartIndex,
                       unsigned int parentStopIndex );

   bool handleDuplicate2DParentChunks( PlotMsgIdType parentGroupMsgId,
                                       bool xParentChanged,
                                       bool yParentChanged,
                                       unsigned int parentStartIndex,
                                       unsigned int parentStopIndex,
                                       bool childIsInScrollMode = false );

   ePlotType determineChildPlotTypeFor1D(tParentCurveInfo &parentInfo, ePlotType origChildPlotType);

   void update1dChildCurve(QString curveName, ePlotType plotType, unsigned int sampleStartIndex, dubVect& yPoints, PlotMsgIdType parentMsgId);
   void update2dChildCurve(unsigned int sampleStartIndex, dubVect& xPoints, dubVect& yPoints, PlotMsgIdType parentMsgId);

   void updateCurve( bool xParentChanged,
                     bool yParentChanged,
                     PlotMsgIdType parentGroupMsgId = PLOT_MSG_ID_TYPE_NO_PARENT_MSG,
                     PlotMsgIdType parentCurveMsgId = PLOT_MSG_ID_TYPE_NO_PARENT_MSG,
                     unsigned int parentStartIndex = 0,
                     unsigned int parentStopIndex = 0);

   CurveCommander* m_curveCmdr;
   QString m_plotName;
   QString m_curveName;
   ePlotType m_plotType;
   bool m_plotIsFft;
   tParentCurveInfo m_xAxis;
   tParentCurveInfo m_yAxis;
   bool m_forceContiguousParentPoints;
   bool m_startChildInScrollMode;

   dubVect m_xSrcData;
   dubVect m_ySrcData;

   dubVect m_prevInfo; // Used to store previous information needed to create some child curves / window coef's for FFTs.

   PlotMsgIdType m_lastGroupMsgId;
   QVector<tParentUpdateChunk> m_fft_parentChunksProcessedInCurGroupMsg;

   int m_fftMeasSize;
   int m_fftMeasPointIndex;
};

#endif



