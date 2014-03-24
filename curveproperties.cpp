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

const int CREATE_CHILD_CURVE_COMBO_1D    = 0;
const int CREATE_CHILD_CURVE_COMBO_2D    = 1;
const int CREATE_CHILD_CURVE_FFT_REAL    = 2;
const int CREATE_CHILD_CURVE_FFT_COMPLEX = 3;

curveProperties::curveProperties(CurveCommander *curveCmdr, QString plotName, QString curveName, QWidget *parent) :
   QWidget(parent),
   ui(new Ui::curveProperties),
   m_curveCmdr(curveCmdr),
   m_xAxisSrcCmbText(""),
   m_yAxisSrcCmbText(""),
   m_plotNameDestCmbText("")
{
   ui->setupUi(this);
   on_cmbPlotType_currentIndexChanged(ui->cmbPlotType->currentIndex());
   setCreateChildComboBoxes(plotName, curveName);
}

curveProperties::~curveProperties()
{
   delete ui;
}

void curveProperties::setCreateChildComboBoxes(QString plotName, QString curveName)
{
   tCurveCommanderInfo allCurves = m_curveCmdr->getCurveCommanderInfo();

   m_xAxisSrcCmbText = ui->cmbXAxisSrc->currentText();
   m_yAxisSrcCmbText = ui->cmbYAxisSrc->currentText();
   m_plotNameDestCmbText = ui->cmbDestPlotName->currentText();

   ui->cmbDestPlotName->clear();
   ui->cmbXAxisSrc->clear();
   ui->cmbYAxisSrc->clear();

   foreach( QString plotName, allCurves.keys() )
   {
      // Add to dest plot name combo box
      ui->cmbDestPlotName->addItem(plotName);

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

   if(plotName != "" && curveName != "")
   {
      setCombosToPlotCurve(plotName, curveName);
   }
   else
   {
      setCombosToPrevValues();
   }
}

void curveProperties::setCombosToPrevValues()
{
   trySetComboItemIndex(ui->cmbXAxisSrc, m_xAxisSrcCmbText);
   trySetComboItemIndex(ui->cmbYAxisSrc, m_yAxisSrcCmbText);

   trySetComboItemIndex(ui->cmbDestPlotName, m_plotNameDestCmbText);
}

void curveProperties::setCombosToPlotCurve(QString plotName, QString curveName)
{
   QString plotCurveName1D = plotName + PLOT_CURVE_SEP + curveName;
   QString plotCurveName2D = plotCurveName1D + Y_AXIS_APPEND;

   QComboBox* cmbBox = ui->cmbXAxisSrc;
   if(trySetComboItemIndex(cmbBox, plotCurveName1D) == false)
   {
      trySetComboItemIndex(cmbBox, plotCurveName2D);
   }

   cmbBox = ui->cmbYAxisSrc;
   if(trySetComboItemIndex(cmbBox, plotCurveName1D) == false)
   {
      trySetComboItemIndex(cmbBox, plotCurveName2D);
   }

   cmbBox = ui->cmbDestPlotName;
   trySetComboItemIndex(cmbBox, plotName);
}

bool curveProperties::trySetComboItemIndex(QComboBox* cmbBox, QString text)
{
   int cmbIndex = getMatchingComboItemIndex(cmbBox, text);
   if(cmbIndex >= 0)
   {
      cmbBox->setCurrentIndex(cmbIndex);
      return true;
   }
   else
   {
      return false;
   }
}

int curveProperties::getMatchingComboItemIndex(QComboBox* cmbBox, QString text)
{
   int retVal = -1;
   for(int i = 0; i < cmbBox->count(); ++i)
   {
      if(cmbBox->itemText(i) == text)
      {
         retVal = i;
         break;
      }
   }
   return retVal;
}

void curveProperties::on_cmbPlotType_currentIndexChanged(int index)
{
   switch(index)
   {
      case CREATE_CHILD_CURVE_COMBO_1D:
         ui->lblYAxisSrc->setText("Y Axis Source");
         ui->lblXAxisSrc->setVisible(false);
         ui->cmbXAxisSrc->setVisible(false);
         ui->lblYAxisSrc->setVisible(true);
         ui->cmbYAxisSrc->setVisible(true);
      break;
      case CREATE_CHILD_CURVE_COMBO_2D:
         ui->lblXAxisSrc->setText("X Axis Source");
         ui->lblYAxisSrc->setText("Y Axis Source");
         ui->lblXAxisSrc->setVisible(true);
         ui->cmbXAxisSrc->setVisible(true);
         ui->lblYAxisSrc->setVisible(true);
         ui->cmbYAxisSrc->setVisible(true);
      break;
      case CREATE_CHILD_CURVE_FFT_REAL:
         ui->lblXAxisSrc->setText("Real Source");
         ui->lblXAxisSrc->setVisible(true);
         ui->cmbXAxisSrc->setVisible(true);
         ui->lblYAxisSrc->setVisible(false);
         ui->cmbYAxisSrc->setVisible(false);
      break;
      case CREATE_CHILD_CURVE_FFT_COMPLEX:
         ui->lblXAxisSrc->setText("Real Source");
         ui->lblYAxisSrc->setText("Imag Source");
         ui->lblXAxisSrc->setVisible(true);
         ui->cmbXAxisSrc->setVisible(true);
         ui->lblYAxisSrc->setVisible(true);
         ui->cmbYAxisSrc->setVisible(true);
      break;

   }
}

void curveProperties::on_cmdApply_clicked()
{
   if(ui->tabWidget->currentIndex() == TAB_CREATE_CHILD_CURVE)
   {
      if( ui->cmbPlotType->currentIndex() == CREATE_CHILD_CURVE_COMBO_1D ||
          ui->cmbPlotType->currentIndex() == CREATE_CHILD_CURVE_FFT_REAL )
      {
         m_curveCmdr->createChildCurve( ui->cmbDestPlotName->currentText(),
                                        ui->txtDestCurveName->text(),
                                        ui->cmbPlotType->currentIndex() == CREATE_CHILD_CURVE_FFT_REAL,
                                        getCreateChildCurveInfo(E_Y_AXIS));
      }
      else
      {
         m_curveCmdr->createChildCurve( ui->cmbDestPlotName->currentText(),
                                        ui->txtDestCurveName->text(),
                                        ui->cmbPlotType->currentIndex() == CREATE_CHILD_CURVE_FFT_COMPLEX,
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



void curveProperties::on_cmdDone_clicked()
{
   m_curveCmdr->curvePropertiesGuiClose();
}

void curveProperties::on_cmdCancel_clicked()
{
   m_curveCmdr->curvePropertiesGuiClose();
}

void curveProperties::closeEvent(QCloseEvent* /*event*/)
{
   m_curveCmdr->curvePropertiesGuiClose();
}

