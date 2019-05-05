/* Copyright 2014 - 2017, 2019 Dan Williams. All Rights Reserved.
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
#include "ChildCurves.h"
#include "CurveCommander.h"
#include "fftHelper.h"
#include "AmFmPmDemod.h"
#include "handleLogData.h"
#include "fftMeasChildParam.h"

ChildCurve::ChildCurve( CurveCommander* curveCmdr,
                        QString plotName,
                        QString curveName,
                        ePlotType plotType,
                        bool forceContiguousParentPoints,
                        tParentCurveInfo yAxis):
   m_curveCmdr(curveCmdr),
   m_plotName(plotName),
   m_curveName(curveName),
   m_plotType(plotType),
   m_plotIsFft(plotTypeIsFft(plotType)),
   m_yAxis(yAxis),
   m_forceContiguousParentPoints(forceContiguousParentPoints),
   m_fftMeasSize(100),
   m_fftMeasPointIndex(0)
{
   m_fft_parentChunksProcessedInCurGroupMsg.reserve(2); // Typically the max number of duplicate parent chunks will be 2 (when the parent fills in the end and starts over at the beginning).
   updateCurve(false, true);
}

ChildCurve::ChildCurve( CurveCommander* curveCmdr,
                        QString plotName,
                        QString curveName,
                        ePlotType plotType,
                        bool forceContiguousParentPoints,
                        tParentCurveInfo xAxis,
                        tParentCurveInfo yAxis):
   m_curveCmdr(curveCmdr),
   m_plotName(plotName),
   m_curveName(curveName),
   m_plotType(plotType),
   m_plotIsFft(plotTypeIsFft(plotType)),
   m_xAxis(xAxis),
   m_yAxis(yAxis),
   m_forceContiguousParentPoints(forceContiguousParentPoints),
   m_fftMeasSize(0), // Don't care, this is not valid for 2D
   m_fftMeasPointIndex(0)
{
   m_fft_parentChunksProcessedInCurGroupMsg.reserve(2); // Typically the max number of duplicate parent chunks will be 2 (when the parent fills in the end and starts over at the beginning).
   updateCurve(true, true);
}

void ChildCurve::anotherCurveChanged( QString plotName,
                                      QString curveName,
                                      unsigned int parentStartIndex,
                                      unsigned int parentNumPoints,
                                      PlotMsgIdType parentGroupMsgId,
                                      PlotMsgIdType parentCurveMsgId )

{
   bool curveIsYAxisParent = (m_yAxis.dataSrc.plotName == plotName) &&
                             (m_yAxis.dataSrc.curveName == curveName);
   bool curveIsXAxisParent = plotTypeHas2DInput(m_plotType) &&
                             (m_xAxis.dataSrc.plotName == plotName) &&
                             (m_xAxis.dataSrc.curveName == curveName);
   bool parentOfFftMeasurementChild = (m_yAxis.dataSrc.plotName == plotName) &&
                                      (m_plotType == E_PLOT_TYPE_FFT_MEASUREMENT);

   bool parentChanged = curveIsYAxisParent || curveIsXAxisParent || parentOfFftMeasurementChild;
   if(parentChanged)
   {
      updateCurve( curveIsXAxisParent,
                   curveIsYAxisParent,
                   parentGroupMsgId,
                   parentCurveMsgId,
                   parentStartIndex,
                   parentStartIndex + parentNumPoints );
   }

}

void ChildCurve::getParentUpdateInfo( tParentCurveInfo &parentInfo,
                                      unsigned int parentStartIndex,
                                      unsigned int parentStopIndex,
                                      bool parentChanged,
                                      CurveData*& parentCurve,
                                      int& origStartIndex,
                                      int& startIndex,
                                      int& stopIndex,
                                      int& scrollModeShift )
{
   // Grab parameters from the Parent Curve.
   parentCurve = m_curveCmdr->getCurveData(parentInfo.dataSrc.plotName, parentInfo.dataSrc.curveName);
   int parentNumPoints = parentCurve->getNumPoints(); // Number of points actually being displayed in the parent curve.
   int origMsgNumPoints = parentCurve->getPlotSize_nonScrollModeVersion(); // The size that the parent curve would be if it weren't in Scroll Mode.
   bool parentIsInScrollMode = parentCurve->getScrollMode();

   bool sliceParentSamples = parentInfo.startIndex != 0 || parentInfo.stopIndex != 0;
   bool grabAllParentPoints = parentStartIndex >= parentStopIndex;

   startIndex = parentInfo.startIndex; // User specified slice of parent.
   stopIndex = parentInfo.stopIndex;   // User specified slice of parent.

   // Slice... Handle negative indexes, 0 is beginning for start and 0 is end for stop.
   if(startIndex < 0)
      startIndex += parentNumPoints;
   if(stopIndex <= 0)
      stopIndex += parentNumPoints;

   // When in Scroll Mode, check for situation where we need to limit the parent samples to
   // use the most recent samples in the curve. This way the child plot can have the same plot
   // dimensions as the parent (if it was not in Scroll Mode).
   if(parentIsInScrollMode && grabAllParentPoints && !sliceParentSamples && !m_plotIsFft)
   {
      startIndex += (parentNumPoints - origMsgNumPoints);
   }

   // Scroll Mode Logic
   scrollModeShift = 0;
   if(parentIsInScrollMode && !grabAllParentPoints && !sliceParentSamples)
   {
      // Parent Curve is in scroll mode. This means the actual new samples are at the end.
      int oldestPoint = parentCurve->getOldestPoint_nonScrollModeVersion(); // This value can be used to determine where the new samples got moved to via scroll mode.

      int numNewParentPoints = parentStopIndex - parentStartIndex;

      int numSampAfter = oldestPoint - parentStopIndex;
      if(numSampAfter < 0)
      {
         numSampAfter += origMsgNumPoints;
      }

      int newParentStopIndex = parentNumPoints - numSampAfter;
      int newParentStartIndex = newParentStopIndex - numNewParentPoints;

      if(newParentStartIndex >= 0 && newParentStopIndex > newParentStartIndex && newParentStopIndex <= parentNumPoints)
      {
         // The Parent Point Indexes were modified, need to keep track of this to make sure the Child puts the new points in the correct location.
         scrollModeShift = newParentStopIndex - parentStopIndex;

         // Update Parent Point Indexes to match where the new points actually are.
         parentStopIndex = newParentStopIndex;
         parentStartIndex = newParentStartIndex;
      }
      //else something weird happened don't update.
   }

   origStartIndex = startIndex;

   if(parentChanged)
   {
      // Make sure indexes are within the parents change window
      if(startIndex < (int)parentStartIndex)
         startIndex = parentStartIndex;

      // If parentChangeStopIndex is 0, the change goes to the end of the curve.
      // In that case there is no need to bound stop indexes.
      if(parentStopIndex > 0)
      {
         if(stopIndex >= (int)parentStopIndex)
            stopIndex = parentStopIndex;
      }
   }
   else
   {
      // Parent did not change. Set copy length to 0;
      stopIndex = startIndex;
   }
}


unsigned int ChildCurve::getDataFromParent1D( unsigned int parentStartIndex,
                                              unsigned int parentStopIndex)
{
   CurveData* yParent;
   int yStartIndex;
   int yStopIndex;
   int origYStart;
   int scrollModeShiftY;

   getParentUpdateInfo( m_yAxis,
                        parentStartIndex,
                        parentStopIndex,
                        true,
                        yParent,
                        origYStart,
                        yStartIndex,
                        yStopIndex,
                        scrollModeShiftY );

   int startOffset = 0;
   int numSampToGet = yStopIndex - yStartIndex;
   if(numSampToGet > 0)
   {
      startOffset = yStartIndex - origYStart;

      if(m_yAxis.dataSrc.axis == E_X_AXIS)
         yParent->getXPoints(m_ySrcData, yStartIndex, yStopIndex);
      else
         yParent->getYPoints(m_ySrcData, yStartIndex, yStopIndex);
   }
   else
   {
      m_ySrcData.clear();
   }

   // If the parent data is being updated via scroll mode, the startOffset will be shifted.
   startOffset -= scrollModeShiftY;

   return startOffset;
}


unsigned int ChildCurve::getDataFromParent2D( bool xParentChanged,
                                              bool yParentChanged,
                                              unsigned int parentStartIndex,
                                              unsigned int parentStopIndex)
{
   CurveData* xParent;
   int xStartIndex;
   int xStopIndex;
   int origXStart;
   int scrollModeShiftX;

   CurveData* yParent;
   int yStartIndex;
   int yStopIndex;
   int origYStart;
   int scrollModeShiftY;

   getParentUpdateInfo( m_xAxis,
                        parentStartIndex,
                        parentStopIndex,
                        xParentChanged,
                        xParent,
                        origXStart,
                        xStartIndex,
                        xStopIndex,
                        scrollModeShiftX );

   getParentUpdateInfo( m_yAxis,
                        parentStartIndex,
                        parentStopIndex,
                        yParentChanged,
                        yParent,
                        origYStart,
                        yStartIndex,
                        yStopIndex,
                        scrollModeShiftY );

   int numXSamp = xStopIndex - xStartIndex;
   int numYSamp = yStopIndex - yStartIndex;

   int numSampToGet = std::max(numXSamp, numYSamp);
   int startOffset = 0;
   if(numSampToGet > 0)
   {
      if(numXSamp > numYSamp)
      {
         startOffset = xStartIndex - origXStart;
         yStartIndex = origYStart + startOffset;
         yStopIndex = yStartIndex + numSampToGet;
      }
      else
      {
         startOffset = yStartIndex - origYStart;
         xStartIndex = origXStart + startOffset;
         xStopIndex = xStartIndex + numSampToGet;
      }

      if(m_xAxis.dataSrc.axis == E_X_AXIS)
         xParent->getXPoints(m_xSrcData, xStartIndex, xStopIndex);
      else
         xParent->getYPoints(m_xSrcData, xStartIndex, xStopIndex);

      if(m_yAxis.dataSrc.axis == E_X_AXIS)
         yParent->getXPoints(m_ySrcData, yStartIndex, yStopIndex);
      else
         yParent->getYPoints(m_ySrcData, yStartIndex, yStopIndex);
   }
   else
   {
      m_xSrcData.clear();
      m_ySrcData.clear();
   }

   // If the parent data is being updated via scroll mode, the startOffset will be shifted.
   if(scrollModeShiftX == scrollModeShiftY)
   {
      // Shift the start offset back to its orignal position.
      startOffset -= scrollModeShiftY;
   }
   return startOffset;
}


bool ChildCurve::handleDuplicateFftParentChunks( PlotMsgIdType parentGroupMsgId,
                                                 bool xParentChanged,
                                                 bool yParentChanged,
                                                 unsigned int parentStartIndex,
                                                 unsigned int parentStopIndex)
{
   // Check for situation where we already pulled in all the new samples for the FFT.
   if( m_forceContiguousParentPoints && xParentChanged != yParentChanged &&
       parentGroupMsgId == m_lastGroupMsgId && parentGroupMsgId != PLOT_MSG_ID_TYPE_NO_PARENT_MSG )
   {
      // Check if we already pulled in the samples when processing the other axis's parent.
      for(QVector<tParentUpdateChunk>::iterator iter = m_fft_parentChunksProcessedInCurGroupMsg.begin(); iter != m_fft_parentChunksProcessedInCurGroupMsg.end(); ++iter)
      {
         if( iter->parentStartIndex == parentStartIndex && iter->parentStopIndex == parentStopIndex &&
             iter->xParentChanged != xParentChanged && iter->yParentChanged != yParentChanged )
         {
            return true; // Already pulled in all these new samples, nothing to do.
         }
      }
   }
   else if(m_forceContiguousParentPoints) // Only need to update m_fft_parentChunksForCurGroupMsg in Force Contiguous mode.
   {
      // New plot message group (or other reason). Clear out the stored info from the old group.
      m_fft_parentChunksProcessedInCurGroupMsg.clear();
   }

   // No early return, so this must be a new chunk.
   if(m_forceContiguousParentPoints) // Only need to update m_fft_parentChunksForCurGroupMsg in Force Contiguous mode.
   {
      // New Chunk. Add it to the list.
      tParentUpdateChunk thisParentChunk;
      thisParentChunk.xParentChanged   = xParentChanged;
      thisParentChunk.yParentChanged   = yParentChanged;
      thisParentChunk.parentStartIndex = parentStartIndex;
      thisParentChunk.parentStopIndex  = parentStopIndex;

      m_fft_parentChunksProcessedInCurGroupMsg.push_back(thisParentChunk);
   }

   return false; // No early return, no duplicate chunks found.
}

void ChildCurve::getDataForFft( ePlotType fftType, 
                                PlotMsgIdType parentGroupMsgId,
                                bool xParentChanged,
                                bool yParentChanged,
                                unsigned int parentStartIndex,
                                unsigned int parentStopIndex )
{
   bool duplicateChunk = handleDuplicateFftParentChunks( parentGroupMsgId,
                                                         xParentChanged,
                                                         yParentChanged,
                                                         parentStartIndex,
                                                         parentStopIndex );

   if(duplicateChunk)
   {
      return; // Already pulled in all these new samples, nothing to do.
   }

   bool complexFft = plotTypeHas2DInput(fftType);

   int numNewPoints = parentStopIndex - parentStartIndex;
   int numPrevFftPoints = (int)m_ySrcData.size();
   int numPrevPointsToKeep = numPrevFftPoints - numNewPoints;

   if( complexFft && m_forceContiguousParentPoints && (xParentChanged || yParentChanged) )
   {
      xParentChanged = yParentChanged = true; // Set both to true.
   }

   if(complexFft)
   {
      CurveData* parentCurveX = m_curveCmdr->getCurveData(m_xAxis.dataSrc.plotName, m_xAxis.dataSrc.curveName);
      CurveData* parentCurveY = m_curveCmdr->getCurveData(m_yAxis.dataSrc.plotName, m_yAxis.dataSrc.curveName);

      bool parentInScrollMode = false;
      unsigned int parentNumPoints = 0;
      if(parentCurveX != NULL && parentCurveY != NULL)
      {
         parentInScrollMode = parentCurveX->getScrollMode() && parentCurveY->getScrollMode();
         parentNumPoints = std::min(parentCurveX->getNumPoints(), parentCurveY->getNumPoints());
      }

      if(parentInScrollMode || !m_forceContiguousParentPoints || m_ySrcData.size() < parentNumPoints)
      {
         getDataFromParent2D(xParentChanged, yParentChanged, 0, 0); // 0, 0 means get all the samples, not a subset of samples.
      }
      else
      {
         // Store off the original points.
         dubVect origXSrcData = m_xSrcData;
         dubVect origYSrcData = m_ySrcData;

         // Grab the new, updated points (this will set m_xSrcData & m_ySrcData to the new points, which may just be a subset of all the curve points).
         getDataFromParent2D(xParentChanged, yParentChanged, parentStartIndex, parentStopIndex);

         // Move the previous point to before the new points
         if(numPrevPointsToKeep > 0)
         {
            m_xSrcData.insert(m_xSrcData.begin(), origXSrcData.begin()+numNewPoints, origXSrcData.end());
            m_ySrcData.insert(m_ySrcData.begin(), origYSrcData.begin()+numNewPoints, origYSrcData.end());
         }
      }
   }
   else
   {
      CurveData* parentCurveY = m_curveCmdr->getCurveData(m_yAxis.dataSrc.plotName, m_yAxis.dataSrc.curveName);

      bool parentInScrollMode = false;
      unsigned int parentNumPoints = 0;
      if(parentCurveY != NULL)
      {
         parentInScrollMode = parentCurveY->getScrollMode();
         parentNumPoints = parentCurveY->getNumPoints();
      }

      if(parentInScrollMode || !m_forceContiguousParentPoints || m_ySrcData.size() < parentNumPoints)
      {
         getDataFromParent1D(0, 0); // 0, 0 means get all the samples, not a subset of samples.
      }
      else
      {
         // Store off the original points.
         dubVect origYSrcData = m_ySrcData;

         // Grab the new, updated points (this will set m_ySrcData to the new points, which may just be a subset of all the curve points).
         getDataFromParent1D(parentStartIndex, parentStopIndex);

         // Move the previous point to before the new points
         if(numPrevPointsToKeep > 0)
         {
            m_ySrcData.insert(m_ySrcData.begin(), origYSrcData.begin()+numNewPoints, origYSrcData.end());
         }
      }
   }
}

// For some combinations of parent plot type and child plot type, it is better to have the child
// plot type set to the parent plot type.
ePlotType ChildCurve::determineChildPlotTypeFor1D(tParentCurveInfo &parentInfo, ePlotType origChildPlotType)
{
   ePlotType retVal = origChildPlotType;
   ePlotType parentPlotType = m_curveCmdr->getCurveData(parentInfo.dataSrc.plotName, parentInfo.dataSrc.curveName)->getPlotType();

   switch(parentPlotType)
   {
      // FFT plots have special processing for their x-axis. If the parent plot is an FFT plot,
      // make the child plot type equal to the parent plot type.
      case E_PLOT_TYPE_REAL_FFT:
      case E_PLOT_TYPE_COMPLEX_FFT:
      case E_PLOT_TYPE_DB_POWER_FFT_REAL:
      case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
         retVal = parentPlotType;
      break;
      default:
         // retVal is initialized to the correct default value. Do nothing.
      break;
   }
   
   return retVal;
}
void ChildCurve::updateCurve( bool xParentChanged,
                              bool yParentChanged,
                              PlotMsgIdType parentGroupMsgId,
                              PlotMsgIdType parentCurveMsgId,
                              unsigned int parentStartIndex,
                              unsigned int parentStopIndex )
{

   switch(m_plotType)
   {
      case E_PLOT_TYPE_1D:
      {
         ePlotType childCurvePlotType = determineChildPlotTypeFor1D(m_yAxis, m_plotType);
         unsigned int offset = getDataFromParent1D(parentStartIndex, parentStopIndex);

         m_curveCmdr->update1dChildCurve( m_plotName, m_curveName, childCurvePlotType, offset, m_ySrcData, parentCurveMsgId);

      }
      break;
      case E_PLOT_TYPE_2D:
      {
         unsigned int offset = getDataFromParent2D( xParentChanged,
                                                    yParentChanged,
                                                    parentStartIndex,
                                                    parentStopIndex );

         m_curveCmdr->update2dChildCurve(m_plotName, m_curveName, offset, m_xSrcData, m_ySrcData, parentCurveMsgId);
      }
      break;
      case E_PLOT_TYPE_REAL_FFT:
      {
         dubVect realFFTOut;

         getDataForFft(m_plotType, parentGroupMsgId, xParentChanged, yParentChanged, parentStartIndex, parentStopIndex);

         if(m_yAxis.windowFFT == true)
         {
            unsigned int dataSize = m_ySrcData.size();
            if(m_prevInfo.size() != dataSize)
            {
               m_prevInfo.resize(dataSize);
               genWindowCoef(&m_prevInfo[0], dataSize, m_yAxis.scaleFftWindow);
            }
            realFFT(m_ySrcData, realFFTOut, &m_prevInfo[0]);
         }
         else
         {
            realFFT(m_ySrcData, realFFTOut);
         }

         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, 0, realFFTOut, parentCurveMsgId);
      }
      break;
      case E_PLOT_TYPE_COMPLEX_FFT:
      {
         dubVect realFFTOut;
         dubVect imagFFTOut;

         getDataForFft(m_plotType, parentGroupMsgId, xParentChanged, yParentChanged, parentStartIndex, parentStopIndex);

         if(m_yAxis.windowFFT == true)
         {
            unsigned int dataSize = std::min(m_ySrcData.size(), m_xSrcData.size());
            if(m_prevInfo.size() != dataSize)
            {
               m_prevInfo.resize(dataSize);
               genWindowCoef(&m_prevInfo[0], dataSize, m_yAxis.scaleFftWindow);
            }
            complexFFT(m_xSrcData, m_ySrcData, realFFTOut, imagFFTOut, &m_prevInfo[0]);
         }
         else
         {
            complexFFT(m_xSrcData, m_ySrcData, realFFTOut, imagFFTOut);
         }


         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName + COMPLEX_FFT_REAL_APPEND, m_plotType, 0, realFFTOut, parentCurveMsgId);
         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName + COMPLEX_FFT_IMAG_APPEND, m_plotType, 0, imagFFTOut, parentCurveMsgId);
      }
      break;
      case E_PLOT_TYPE_AM_DEMOD:
      {
         dubVect demodOut;
         unsigned int offset = getDataFromParent2D( xParentChanged,
                                                    yParentChanged,
                                                    parentStartIndex,
                                                    parentStopIndex );
         AmDemod(m_xSrcData, m_ySrcData, demodOut);
         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, offset, demodOut, parentCurveMsgId);
      }
      break;
      case E_PLOT_TYPE_FM_DEMOD:
      {
         dubVect fmDemod;
         unsigned int offset = getDataFromParent2D( xParentChanged,
                                                    yParentChanged,
                                                    parentStartIndex,
                                                    parentStopIndex );

         int dataSize = std::min(m_xSrcData.size(), m_ySrcData.size());
         if(dataSize > 0)
         {
            int pmDemodSize = m_prevInfo.size();

            // Get index, make sure it is valid.
            int prevPhaseIndex = (int)offset - 1;
            if(prevPhaseIndex < 0)
            {
               prevPhaseIndex = pmDemodSize - 1;
            }

            // If the prev info array size is 0, can't read from it.
            double prevPhase = pmDemodSize <= 0 ? 0.0 : m_prevInfo[prevPhaseIndex];

            // Resize if needed to allow room for new samples.
            if(pmDemodSize < ((int)offset + dataSize))
            {
               m_prevInfo.resize(offset + dataSize);
            }

            FmPmDemod(m_xSrcData, m_ySrcData, fmDemod, &m_prevInfo[offset], prevPhase);
            m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, offset, fmDemod, parentCurveMsgId);
         }
      }
      break;
      case E_PLOT_TYPE_PM_DEMOD:
      {
         dubVect pmDemod;
         unsigned int offset = getDataFromParent2D( xParentChanged,
                                                    yParentChanged,
                                                    parentStartIndex,
                                                    parentStopIndex );

         int dataSize = std::min(m_xSrcData.size(), m_ySrcData.size());
         if(dataSize > 0)
         {
            int pmDemodSize = m_prevInfo.size();

            // Get index, make sure it is valid.
            int prevPhaseIndex = (int)offset - 1;
            if(prevPhaseIndex < 0)
            {
               prevPhaseIndex = pmDemodSize - 1;
            }

            // If the prev info array size is 0, can't read from it.
            double prevPhase = pmDemodSize <= 0 ? 0.0 : m_prevInfo[prevPhaseIndex];

            // Resize if needed to allow room for new samples.
            if(pmDemodSize < ((int)offset + dataSize))
            {
               m_prevInfo.resize(offset + dataSize);
            }

            PmDemod(m_xSrcData, m_ySrcData, &m_prevInfo[offset], prevPhase);
            pmDemod.assign(&m_prevInfo[offset], (&m_prevInfo[offset])+dataSize);
            m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, offset, pmDemod, parentCurveMsgId);
         }
      }
      break;
      case E_PLOT_TYPE_AVERAGE:
      {
         ePlotType childCurvePlotType = determineChildPlotTypeFor1D(m_yAxis, m_plotType);
         unsigned int offset = getDataFromParent1D(parentStartIndex, parentStopIndex);

         int dataSize = m_ySrcData.size();
         if(dataSize > 0)
         {
            int prevAvgSize = m_prevInfo.size();

            // Get index, make sure it is valid.
            int prevAvgIndex = (int)offset - 1;
            if(prevAvgIndex < 0)
            {
               prevAvgIndex = prevAvgSize - 1;
            }

            // If the prev info array size is 0, can't read from it.
            double prevAvg = prevAvgSize <= 0 ? 0.0 : m_prevInfo[prevAvgIndex];

            // Resize if needed to allow room for new samples.
            if(prevAvgSize < ((int)offset + dataSize))
            {
               m_prevInfo.resize(offset + dataSize);
            }

            double avgKeepAmount = 1.0 - m_yAxis.avgAmount;
            for(int i = 0; i < dataSize; ++i)
            {
               if(isDoubleValid(m_ySrcData[i]))
               {
                  prevAvg = m_ySrcData[i] = m_prevInfo[offset + i] =
                        (m_yAxis.avgAmount * prevAvg) + (avgKeepAmount * m_ySrcData[i]);
               }
               else
               {
                  m_ySrcData[i] = m_prevInfo[offset + i] = prevAvg;
               }
            }


            m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, childCurvePlotType, offset, m_ySrcData, parentCurveMsgId);
         }
      }
      break;
      case E_PLOT_TYPE_DB_POWER_FFT_REAL:
      {
         dubVect realFFTOut;
         getDataForFft(m_plotType, parentGroupMsgId, xParentChanged, yParentChanged, parentStartIndex, parentStopIndex);

         if(m_yAxis.windowFFT == true)
         {
            unsigned int dataSize = m_ySrcData.size();
            if(m_prevInfo.size() != dataSize)
            {
               m_prevInfo.resize(dataSize);
               genWindowCoef(&m_prevInfo[0], dataSize, m_yAxis.scaleFftWindow);
            }
            realFFT(m_ySrcData, realFFTOut, &m_prevInfo[0]);
         }
         else
         {
            realFFT(m_ySrcData, realFFTOut);
         }

         unsigned int fftSize = realFFTOut.size();
         for(unsigned int i = 0; i < fftSize; ++i)
         {
            realFFTOut[i] = 10.0 * log10(realFFTOut[i] * realFFTOut[i]);
         }

         handleLogData(&realFFTOut[0], fftSize);

         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, 0, realFFTOut, parentCurveMsgId);
      }
      break;
      case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
      {
         dubVect realFFTOut;
         dubVect imagFFTOut;

         getDataForFft(m_plotType, parentGroupMsgId, xParentChanged, yParentChanged, parentStartIndex, parentStopIndex);
         if(m_yAxis.windowFFT == true)
         {
            unsigned int dataSize = std::min(m_ySrcData.size(), m_xSrcData.size());
            if(m_prevInfo.size() != dataSize)
            {
               m_prevInfo.resize(dataSize);
               genWindowCoef(&m_prevInfo[0], dataSize, m_yAxis.scaleFftWindow);
            }
            complexFFT(m_xSrcData, m_ySrcData, realFFTOut, imagFFTOut, &m_prevInfo[0]);
         }
         else
         {
            complexFFT(m_xSrcData, m_ySrcData, realFFTOut, imagFFTOut);
         }

         unsigned int fftSize = realFFTOut.size();
         for(unsigned int i = 0; i < fftSize; ++i)
         {
            realFFTOut[i] = 10.0 * log10( (realFFTOut[i] * realFFTOut[i]) + (imagFFTOut[i] * imagFFTOut[i]) );
         }

         handleLogData(&realFFTOut[0], fftSize);

         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, 0, realFFTOut, parentCurveMsgId);
      }
      break;
      case E_PLOT_TYPE_DELTA:
      case E_PLOT_TYPE_SUM:
      {
         ePlotType childCurvePlotType = determineChildPlotTypeFor1D(m_yAxis, m_plotType);
         unsigned int offset = getDataFromParent1D(parentStartIndex, parentStopIndex);

         int dataSize = m_ySrcData.size();
         if(dataSize > 0)
         {
            int prevSize = m_prevInfo.size();

            // Get index, make sure it is valid.
            int prevIndex = (int)offset - 1;
            if(prevIndex < 0)
            {
               prevIndex = prevSize - 1;
            }

            // If the prev info array size is 0, can't read from it.
            double prevVal = prevSize <= 0 ? 0.0 : m_prevInfo[prevIndex];

            // Resize if needed to allow room for new samples.
            if(prevSize < ((int)offset + dataSize))
            {
               m_prevInfo.resize(offset + dataSize);
            }

            if(m_plotType == E_PLOT_TYPE_DELTA)
            {
               for(int i = 0; i < dataSize; ++i)
               {
                  m_prevInfo[offset + i] = m_ySrcData[i];
                  m_ySrcData[i] -= prevVal;
                  prevVal = m_prevInfo[offset + i];
               }

               // The very first point has no previous point to take a delta against.
               // So, set the very first delta to 'Not a Number'.
               if(prevSize == 0)
               {
                  m_ySrcData[0] = NAN; // Set very first delta to 'Not a Number'
               }
            }
            else // Must be E_PLOT_TYPE_SUM
            {
               for(int i = 0; i < dataSize; ++i)
               {
                  if(isDoubleValid(m_ySrcData[i]))
                  {
                     m_ySrcData[i] = m_prevInfo[offset + i] = prevVal + m_ySrcData[i];
                     prevVal = m_prevInfo[offset + i];
                  }
                  else
                  {
                     m_ySrcData[i] = m_prevInfo[offset + i] = prevVal;
                  }
               }
            }

            m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, childCurvePlotType, offset, m_ySrcData, parentCurveMsgId);
         }
      }
      break;
      case E_PLOT_TYPE_MATH_BETWEEN_CURVES:
      {
         dubVect mathOut;
         unsigned int offset = getDataFromParent2D( xParentChanged,
                                                    yParentChanged,
                                                    parentStartIndex,
                                                    parentStopIndex );

         unsigned int outSize = std::min(m_xSrcData.size(), m_ySrcData.size());
         mathOut.resize(outSize);
         switch(m_yAxis.mathBetweenCurvesOperator)
         {
            default:
            case E_MATH_BETWEEN_CURVES_ADD:
               for(unsigned int i = 0; i < outSize; ++i)
                  mathOut[i] = m_xSrcData[i] + m_ySrcData[i];
            break;
            case E_MATH_BETWEEN_CURVES_SUBTRACT:
               for(unsigned int i = 0; i < outSize; ++i)
                  mathOut[i] = m_xSrcData[i] - m_ySrcData[i];
            break;
            case E_MATH_BETWEEN_CURVES_MULTILPY:
               for(unsigned int i = 0; i < outSize; ++i)
                  mathOut[i] = m_xSrcData[i] * m_ySrcData[i];
            break;
            case E_MATH_BETWEEN_CURVES_DIVIDE:
               for(unsigned int i = 0; i < outSize; ++i)
                  mathOut[i] = m_xSrcData[i] / m_ySrcData[i];
            break;
         }

         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, offset, mathOut, parentCurveMsgId);
      }
      break;
      case E_PLOT_TYPE_FFT_MEASUREMENT:
      {
         MainWindow* parentPlot = m_curveCmdr->getMainPlot(m_yAxis.dataSrc.plotName);

         if(parentPlot != NULL && parentPlot->areFftMeasurementsVisible()) // Only plot FFT Measurement if the measurement are visible on the parent.
         {
            m_ySrcData.clear(); // Clear out previous values, only sending 1 point.
            m_ySrcData.push_back(parentPlot->getFftMeasurement(m_yAxis.fftMeasurementType)); // Grab the FFT Meaurement value to be plotted.

            // Set the FFT Measurement Child Plot Size.
            if(m_fftMeasSize != m_yAxis.fftMeasurementPlotSize && m_yAxis.fftMeasurementPlotSize > 0)
            {
               m_fftMeasSize = m_yAxis.fftMeasurementPlotSize;
            }

            // Check if we need to use a previous sibling curve point index value.
            bool useSiblingPointIndex = fftMeasChildParam_getIndex( m_yAxis.dataSrc.plotName,
                                                                    m_plotName,
                                                                    m_curveName,
                                                                    parentGroupMsgId,
                                                                    m_fftMeasSize,
                                                                    m_fftMeasPointIndex );

            // Check if this is a new FFT measurement in a Child Plot that is already plotting other FFT Measurements.
            if(useSiblingPointIndex && parentGroupMsgId < 0 && (m_fftMeasPointIndex + 1) < m_fftMeasSize)
            {
               // Fill the end with 'Not A Number' values (this helps with scan mode).
               m_ySrcData.insert(m_ySrcData.end(), m_fftMeasSize - m_fftMeasPointIndex - 1, NAN);
            }

            // Update the Child Plot with the new FFT Measurement value.
            m_curveCmdr->update1dChildCurve( m_plotName, m_curveName, m_plotType, m_fftMeasPointIndex, m_ySrcData, parentCurveMsgId);

            if(++m_fftMeasPointIndex >= m_fftMeasSize)
            {
               m_fftMeasPointIndex = 0;
            }
         }
      }
      break;
      default:
         // TODO should I do something here???
      break;
   }

   setToParentsSampleRate();

   m_lastGroupMsgId = parentGroupMsgId;
}

void ChildCurve::setToParentsSampleRate()
{
   // Try to set the child curve sample rate to the parent curves sample rate.
   MainWindow* childPlot = m_curveCmdr->getMainPlot(m_plotName);
   CurveData* parentCurve = m_curveCmdr->getCurveData(m_yAxis.dataSrc.plotName, m_yAxis.dataSrc.curveName);
   if(childPlot != NULL && parentCurve != NULL)
   {
      if(m_plotType != E_PLOT_TYPE_COMPLEX_FFT)
      {
         // Only has one child curve.
         childPlot->setCurveSampleRate(m_curveName, parentCurve->getSampleRate(), false);
      }
      else
      {
         // Has real and imag child curves.
         childPlot->setCurveSampleRate(m_curveName + COMPLEX_FFT_REAL_APPEND, parentCurve->getSampleRate(), false);
         childPlot->setCurveSampleRate(m_curveName + COMPLEX_FFT_IMAG_APPEND, parentCurve->getSampleRate(), false);
      }
   }
}


QVector<tPlotCurveAxis> ChildCurve::getParents()
{
   QVector<tPlotCurveAxis> retVal;
   if(plotTypeHas2DInput(m_plotType))
   {
      retVal.push_back(m_xAxis.dataSrc);
   }
   retVal.push_back(m_yAxis.dataSrc);
   return retVal;
}

