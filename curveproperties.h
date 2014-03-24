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
#ifndef CURVEPROPERTIES_H
#define CURVEPROPERTIES_H

#include <QWidget>
#include <QComboBox>
#include "PlotHelperTypes.h"

namespace Ui {
class curveProperties;
}

class CurveCommander;

class curveProperties : public QWidget
{
   Q_OBJECT
   
public:
   explicit curveProperties(CurveCommander* curveCmdr, QString plotName = "", QString curveName = "", QWidget *parent = 0);
   ~curveProperties();

   void setCreateChildComboBoxes(QString plotName = "", QString curveName = "");
   
private slots:
   void on_cmbPlotType_currentIndexChanged(int index);
   void on_cmdApply_clicked();

   void on_cmdDone_clicked();

   void on_cmdCancel_clicked();

private:
   void closeEvent(QCloseEvent* event);

   void setCombosToPrevValues();
   void setCombosToPlotCurve(QString plotName, QString curveName);
   bool trySetComboItemIndex(QComboBox* cmbBox, QString text);
   int getMatchingComboItemIndex(QComboBox* cmbBox, QString text);

   tParentCurveAxis getCreateChildCurveInfo(QComboBox* cmbBox);


   Ui::curveProperties *ui;

   CurveCommander* m_curveCmdr;

   QString m_xAxisSrcCmbText;
   QString m_yAxisSrcCmbText;
   QString m_plotNameDestCmbText;
   QString m_mathSrcCmbText;
};

#endif // CURVEPROPERTIES_H
