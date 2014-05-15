/* Copyright 2014 Dan Williams. All Rights Reserved.
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
   updateCurve();
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
   updateCurve();
}

void ChildCurve::anotherCurveChanged(QString plotName, QString curveName)
{
   bool curveIsYAxisParent = (m_yAxis.dataSrc.plotName == plotName) &&
                             (m_yAxis.dataSrc.curveName == curveName);
   bool curveIsXAxisParent = ( m_plotType == E_PLOT_TYPE_2D ||
                               m_plotType == E_PLOT_TYPE_COMPLEX_FFT ) &&
                             (m_xAxis.dataSrc.plotName == plotName) &&
                             (m_xAxis.dataSrc.curveName == curveName);

   bool parentChanged = curveIsYAxisParent || curveIsXAxisParent;
   if(parentChanged)
   {
      updateCurve();
   }

}

void ChildCurve::getDataFromParent(tParentCurveInfo &parentInfo, dubVect& data)
{
   CurveData* parent = m_curveCmdr->getCurveData(parentInfo.dataSrc.plotName, parentInfo.dataSrc.curveName);
   int startIndex = parentInfo.startIndex;
   int stopIndex = parentInfo.stopIndex;

   // Slice... Handle negative indexes, 0 is beginning for start and 0 is end for stop.
   if(startIndex < 0)
      startIndex += parent->getNumPoints();
   if(stopIndex <= 0)
      stopIndex += parent->getNumPoints();

   if(parent != NULL)
   {
      if(parentInfo.dataSrc.axis == E_X_AXIS)
         parent->getXPoints(data, startIndex, stopIndex);
      else
         parent->getYPoints(data, startIndex, stopIndex);
   }
}

void ChildCurve::updateCurve()
{
   getDataFromParent(m_yAxis, m_ySrcData);

   if(m_plotType == E_PLOT_TYPE_2D || m_plotType == E_PLOT_TYPE_COMPLEX_FFT)
   {
      getDataFromParent(m_xAxis, m_xSrcData);
   }

   switch(m_plotType)
   {
      case E_PLOT_TYPE_1D:
         m_curveCmdr->create1dCurve(m_plotName, m_curveName, m_plotType, m_ySrcData);
      break;
      case E_PLOT_TYPE_2D:
         m_curveCmdr->create2dCurve(m_plotName, m_curveName, m_xSrcData, m_ySrcData);
      break;
      case E_PLOT_TYPE_REAL_FFT:
      {
         dubVect realFFTOut;
         realFFT(m_ySrcData, realFFTOut);
         m_curveCmdr->create1dCurve(m_plotName, m_curveName, m_plotType, realFFTOut);
      }
      break;
      case E_PLOT_TYPE_COMPLEX_FFT:
      {
         dubVect realFFTOut;
         dubVect imagFFTOut;

         complexFFT(m_xSrcData, m_ySrcData, realFFTOut, imagFFTOut);

         m_curveCmdr->create1dCurve(m_plotName, m_curveName + COMPLEX_FFT_REAL_APPEND, m_plotType, realFFTOut);
         m_curveCmdr->create1dCurve(m_plotName, m_curveName + COMPLEX_FFT_IMAG_APPEND, m_plotType, imagFFTOut);
      }
      break;
   }

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
   retVal.push_back(m_yAxis.dataSrc);
   if(m_plotType == E_PLOT_TYPE_2D || m_plotType == E_PLOT_TYPE_COMPLEX_FFT)
   {
      retVal.push_back(m_xAxis.dataSrc);
   }
   return retVal;
}

