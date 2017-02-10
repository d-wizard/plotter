/* Copyright 2014 - 2017 Dan Williams. All Rights Reserved.
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

const QString COMPLEX_FFT_REAL_APPEND = ".real";
const QString COMPLEX_FFT_IMAG_APPEND = ".imag";

ChildCurve::ChildCurve( CurveCommander* curveCmdr,
                        QString plotName,
                        QString curveName,
                        ePlotType plotType,
                        tParentCurveInfo yAxis):
   m_curveCmdr(curveCmdr),
   m_plotName(plotName),
   m_curveName(curveName),
   m_plotType(plotType),
   m_yAxis(yAxis)
{
   updateCurve(false, true);
}

ChildCurve::ChildCurve( CurveCommander* curveCmdr,
                        QString plotName,
                        QString curveName,
                        ePlotType plotType,
                        tParentCurveInfo xAxis,
                        tParentCurveInfo yAxis):
   m_curveCmdr(curveCmdr),
   m_plotName(plotName),
   m_curveName(curveName),
   m_plotType(plotType),
   m_xAxis(xAxis),
   m_yAxis(yAxis)
{
   updateCurve(true, true);
}

void ChildCurve::anotherCurveChanged(QString plotName, QString curveName, unsigned int parentStartIndex, unsigned int parentNumPoints, PlotMsgIdType parentMsgId)
{
   bool curveIsYAxisParent = (m_yAxis.dataSrc.plotName == plotName) &&
                             (m_yAxis.dataSrc.curveName == curveName);
   bool curveIsXAxisParent = plotTypeHas2DInput(m_plotType) &&
                             (m_xAxis.dataSrc.plotName == plotName) &&
                             (m_xAxis.dataSrc.curveName == curveName);

   bool parentChanged = curveIsYAxisParent || curveIsXAxisParent;
   if(parentChanged)
   {
      updateCurve( curveIsXAxisParent,
                   curveIsYAxisParent,
                   parentMsgId,
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
                                      int& stopIndex)
{
   parentCurve = m_curveCmdr->getCurveData(parentInfo.dataSrc.plotName, parentInfo.dataSrc.curveName);
   startIndex = parentInfo.startIndex;
   stopIndex = parentInfo.stopIndex;

   // Slice... Handle negative indexes, 0 is beginning for start and 0 is end for stop.
   if(startIndex < 0)
      startIndex += parentCurve->getNumPoints();
   if(stopIndex <= 0)
      stopIndex += parentCurve->getNumPoints();

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

   getParentUpdateInfo( m_yAxis,
                        parentStartIndex,
                        parentStopIndex,
                        true,
                        yParent,
                        origYStart,
                        yStartIndex,
                        yStopIndex );

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

   CurveData* yParent;
   int yStartIndex;
   int yStopIndex;
   int origYStart;

   getParentUpdateInfo( m_xAxis,
                        parentStartIndex,
                        parentStopIndex,
                        xParentChanged,
                        xParent,
                        origXStart,
                        xStartIndex,
                        xStopIndex );

   getParentUpdateInfo( m_yAxis,
                        parentStartIndex,
                        parentStopIndex,
                        yParentChanged,
                        yParent,
                        origYStart,
                        yStartIndex,
                        yStopIndex );

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

   return startOffset;
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
                              PlotMsgIdType parentMsgId,
                              unsigned int parentStartIndex,
                              unsigned int parentStopIndex )
{

   switch(m_plotType)
   {
      case E_PLOT_TYPE_1D:
      {
         ePlotType childCurvePlotType = determineChildPlotTypeFor1D(m_yAxis, m_plotType);
         unsigned int offset = getDataFromParent1D(parentStartIndex, parentStopIndex);

         m_curveCmdr->update1dChildCurve( m_plotName, m_curveName, childCurvePlotType, offset, m_ySrcData, parentMsgId);

      }
      break;
      case E_PLOT_TYPE_2D:
      {
         unsigned int offset = getDataFromParent2D( xParentChanged,
                                                    yParentChanged,
                                                    parentStartIndex,
                                                    parentStopIndex );

         m_curveCmdr->update2dChildCurve(m_plotName, m_curveName, offset, m_xSrcData, m_ySrcData, parentMsgId);
      }
      break;
      case E_PLOT_TYPE_REAL_FFT:
      {
         dubVect realFFTOut;

         getDataFromParent1D(0, 0); // 0, 0 means get all the samples, not a subset of samples.

         if(m_yAxis.windowFFT == true)
         {
            unsigned int dataSize = m_ySrcData.size();
            if(m_prevInfo.size() != dataSize)
            {
               m_prevInfo.resize(dataSize);
               genBlackmanWindowCoef(&m_prevInfo[0], dataSize);
            }
            realFFT(m_ySrcData, realFFTOut, &m_prevInfo[0]);
         }
         else
         {
            realFFT(m_ySrcData, realFFTOut);
         }

         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, 0, realFFTOut, parentMsgId);
      }
      break;
      case E_PLOT_TYPE_COMPLEX_FFT:
      {
         dubVect realFFTOut;
         dubVect imagFFTOut;

         getDataFromParent2D(xParentChanged, yParentChanged);

         if(m_yAxis.windowFFT == true)
         {
            unsigned int dataSize = std::min(m_ySrcData.size(), m_xSrcData.size());
            if(m_prevInfo.size() != dataSize)
            {
               m_prevInfo.resize(dataSize);
               genBlackmanWindowCoef(&m_prevInfo[0], dataSize);
            }
            complexFFT(m_xSrcData, m_ySrcData, realFFTOut, imagFFTOut, &m_prevInfo[0]);
         }
         else
         {
            complexFFT(m_xSrcData, m_ySrcData, realFFTOut, imagFFTOut);
         }


         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName + COMPLEX_FFT_REAL_APPEND, m_plotType, 0, realFFTOut, parentMsgId);
         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName + COMPLEX_FFT_IMAG_APPEND, m_plotType, 0, imagFFTOut, parentMsgId);
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
         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, offset, demodOut, parentMsgId);
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
            m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, offset, fmDemod, parentMsgId);
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
            m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, offset, pmDemod, parentMsgId);
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


            m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, childCurvePlotType, offset, m_ySrcData, parentMsgId);
         }
      }
      break;
      case E_PLOT_TYPE_DB_POWER_FFT_REAL:
      {
         dubVect realFFTOut;
         getDataFromParent1D(0, 0); // 0, 0 means get all the samples, not a subset of samples.

         if(m_yAxis.windowFFT == true)
         {
            unsigned int dataSize = m_ySrcData.size();
            if(m_prevInfo.size() != dataSize)
            {
               m_prevInfo.resize(dataSize);
               genBlackmanWindowCoef(&m_prevInfo[0], dataSize);
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

         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, 0, realFFTOut, parentMsgId);
      }
      break;
      case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
      {
         dubVect realFFTOut;
         dubVect imagFFTOut;

         getDataFromParent2D(xParentChanged, yParentChanged);
         if(m_yAxis.windowFFT == true)
         {
            unsigned int dataSize = std::min(m_ySrcData.size(), m_xSrcData.size());
            if(m_prevInfo.size() != dataSize)
            {
               m_prevInfo.resize(dataSize);
               genBlackmanWindowCoef(&m_prevInfo[0], dataSize);
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

         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, 0, realFFTOut, parentMsgId);
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

            m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, childCurvePlotType, offset, m_ySrcData, parentMsgId);
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

         m_curveCmdr->update1dChildCurve(m_plotName, m_curveName, m_plotType, offset, mathOut, parentMsgId);
      }
      break;
      default:
         // TODO should I do something here???
      break;
   }

   setToParentsSampleRate();
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

