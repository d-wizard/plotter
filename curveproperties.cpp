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
#include "curveproperties.h"
#include "ui_curveproperties.h"
#include "CurveCommander.h"
#include "dString.h"

const QString X_AXIS_APPEND = ".xAxis";
const QString Y_AXIS_APPEND = ".yAxis";
const QString PLOT_CURVE_SEP = "->";

const int TAB_CREATE_CHILD_CURVE = 0;

const int CREATE_CHILD_CURVE_COMBO_1D = 0;
const int CREATE_CHILD_CURVE_COMBO_2D = 1;

curveProperties::curveProperties(CurveCommander *curveCmdr, QWidget *parent) :
   QWidget(parent),
   ui(new Ui::curveProperties),
   m_curveCmdr(curveCmdr)
{
   ui->setupUi(this);
   on_cmbPlotType_currentIndexChanged(ui->cmbPlotType->currentIndex());
   setCreateChildComboBoxes();
}

curveProperties::~curveProperties()
{
   delete ui;
}

void curveProperties::setCreateChildComboBoxes()
{
   tCurveCommanderInfo allCurves = m_curveCmdr->getCurveCommanderInfo();
   foreach( QString plotName, allCurves.keys() )
   {
      // Add to dest plot name combo box
      ui->cmdDestPlotName->addItem(plotName);

      tCurveDataInfo* curves = &(allCurves[plotName].curves);
      foreach( QString curveName, curves->keys() )
      {
         QString plotCurveName = plotName + PLOT_CURVE_SEP + curveName;
         if( (*curves)[curveName]->getPlotType() == E_PLOT_TYPE_1D)
         {
            ui->cmbXAxisSrc->addItem(plotCurveName);
            ui->cmbYAxisSrc->addItem(plotCurveName);
         }
         else
         {
            ui->cmbXAxisSrc->addItem(plotCurveName + X_AXIS_APPEND);
            ui->cmbYAxisSrc->addItem(plotCurveName + X_AXIS_APPEND);
            ui->cmbXAxisSrc->addItem(plotCurveName + Y_AXIS_APPEND);
            ui->cmbYAxisSrc->addItem(plotCurveName + Y_AXIS_APPEND);
         }
      }

   }
}


void curveProperties::on_cmbPlotType_currentIndexChanged(int index)
{
   if(index == CREATE_CHILD_CURVE_COMBO_1D)
   {
      ui->lblXAxisSrc->setVisible(false);
      ui->cmbXAxisSrc->setVisible(false);
   }
   else
   {
      ui->lblXAxisSrc->setVisible(true);
      ui->cmbXAxisSrc->setVisible(true);
   }

}

void curveProperties::on_cmdApply_clicked()
{
   if(ui->tabWidget->tabPosition() == TAB_CREATE_CHILD_CURVE)
   {
      if(ui->cmbPlotType->currentIndex() == CREATE_CHILD_CURVE_COMBO_1D)
      {
         m_curveCmdr->createChildCurve( ui->cmdDestPlotName->currentText(),
                                        ui->txtDestCurveName->text(),
                                        getCreateChildCurveInfo(E_Y_AXIS));
      }
      else
      {
         m_curveCmdr->createChildCurve( ui->cmdDestPlotName->currentText(),
                                        ui->txtDestCurveName->text(),
                                        getCreateChildCurveInfo(E_X_AXIS),
                                        getCreateChildCurveInfo(E_Y_AXIS));
      }
   }
}



tParentCurveAxis curveProperties::getCreateChildCurveInfo(eAxis childAxis)
{
   std::string cmbText = "";
   if(childAxis == E_X_AXIS)
      cmbText = ui->cmbXAxisSrc->currentText().toStdString();
   else
      cmbText = ui->cmbYAxisSrc->currentText().toStdString();

   tParentCurveAxis retVal;
   int axisAppendLen = (int)X_AXIS_APPEND.size();

   // Handle plot name ending in .xAxis or .yAxis or neither
   if( dString::Right(cmbText, axisAppendLen) == X_AXIS_APPEND.toStdString())
   {
      retVal.axis = E_X_AXIS;
      cmbText = dString::Left(cmbText, cmbText.size() - axisAppendLen);
   }
   else if( dString::Right(cmbText, axisAppendLen) == Y_AXIS_APPEND.toStdString())
   {
      retVal.axis = E_Y_AXIS;
      cmbText = dString::Left(cmbText, cmbText.size() - axisAppendLen);
   }
   else
   {
      retVal.axis = E_Y_AXIS;
   }

   retVal.plotName = QString::fromStdString(dString::SplitLeft(cmbText, PLOT_CURVE_SEP.toStdString()));
   retVal.curveName = QString::fromStdString(dString::SplitRight(cmbText, PLOT_CURVE_SEP.toStdString()));

   return retVal;
}



