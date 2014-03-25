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
#ifndef ChildCurves_h
#define ChildCurves_h

#include <QWidget>
#include <QString>
#include <QVector>
#include <PlotHelperTypes.h>

class CurveCommander;

class ChildCurve : public QWidget
{
   Q_OBJECT
public:
   ChildCurve(CurveCommander* curveCmdr, QString plotName, QString curveName, ePlotType plotType, tParentCurveAxis yAxis);
   ChildCurve(CurveCommander* curveCmdr,  QString plotName, QString curveName, ePlotType plotType, tParentCurveAxis xAxis, tParentCurveAxis yAxis);

   void anotherCurveChanged(QString plotName, QString curveName);
   QVector<tParentCurveAxis> getParents();

private:
   // Eliminate default, copy, assign
   ChildCurve();
   ChildCurve(ChildCurve const&);
   void operator=(ChildCurve const&);

   void getDataFromParent(tParentCurveAxis& parentInfo, dubVect &data);
   void updateCurve();

   CurveCommander* m_curveCmdr;
   QString m_plotName;
   QString m_curveName;
   ePlotType m_plotType;
   tParentCurveAxis m_xAxis;
   tParentCurveAxis m_yAxis;

   dubVect m_xSrcData;
   dubVect m_ySrcData;
};

#endif



