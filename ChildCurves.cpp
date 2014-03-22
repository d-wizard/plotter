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

ChildCurve::ChildCurve( CurveCommander* curveCmdr,
                        QString plotName,
                        QString curveName,
                        tParentCurveAxis yAxis):
   m_curveCmdr(curveCmdr),
   m_plotName(plotName),
   m_curveName(curveName),
   m_plotType(E_PLOT_TYPE_1D),
   m_yAxis(yAxis)
{
   updateCurve();
}

ChildCurve::ChildCurve( CurveCommander* curveCmdr,
                        QString plotName,
                        QString curveName,
                        tParentCurveAxis xAxis,
                        tParentCurveAxis yAxis):
   m_curveCmdr(curveCmdr),
   m_plotName(plotName),
   m_curveName(curveName),
   m_plotType(E_PLOT_TYPE_2D),
   m_xAxis(xAxis),
   m_yAxis(yAxis)
{
   updateCurve();
}

void ChildCurve::anotherCurveChanged(QString plotName, QString curveName)
{
   bool curveIsYAxisParent = (m_yAxis.plotName == plotName) &&
                             (m_yAxis.curveName == curveName);
   bool curveIsXAxisParent = (m_plotType == E_PLOT_TYPE_2D) &&
                             (m_xAxis.plotName == plotName) &&
                             (m_xAxis.curveName == curveName);

   bool parentChanged = curveIsYAxisParent || curveIsXAxisParent;
   if(parentChanged)
   {
      updateCurve();
   }

}

void ChildCurve::updateCurve()
{
   if(m_plotType == E_PLOT_TYPE_1D)
   {
      dubVect yPoints;
      if(m_yAxis.axis == E_X_AXIS)
         m_curveCmdr->getCurveData(m_yAxis.plotName, m_yAxis.curveName)->getXPoints(yPoints);
      else
         m_curveCmdr->getCurveData(m_yAxis.plotName, m_yAxis.curveName)->getYPoints(yPoints);

      m_curveCmdr->create1dCurve(m_plotName, m_curveName, yPoints);
   }
   else
   {
      dubVect xPoints;
      if(m_xAxis.axis == E_X_AXIS)
         m_curveCmdr->getCurveData(m_xAxis.plotName, m_xAxis.curveName)->getXPoints(xPoints);
      else
         m_curveCmdr->getCurveData(m_xAxis.plotName, m_xAxis.curveName)->getYPoints(xPoints);

      dubVect yPoints;
      if(m_yAxis.axis == E_X_AXIS)
         m_curveCmdr->getCurveData(m_yAxis.plotName, m_yAxis.curveName)->getXPoints(yPoints);
      else
         m_curveCmdr->getCurveData(m_yAxis.plotName, m_yAxis.curveName)->getYPoints(yPoints);

      m_curveCmdr->create2dCurve(m_plotName, m_curveName, xPoints, yPoints);
   }
}


QVector<tParentCurveAxis> ChildCurve::getParents()
{
   QVector<tParentCurveAxis> retVal;
   retVal.push_back(m_yAxis);
   if(m_plotType == E_PLOT_TYPE_2D)
   {
      retVal.push_back(m_xAxis);
   }
   return retVal;
}

