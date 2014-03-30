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
#include <QMessageBox>

const QString X_AXIS_APPEND = ".xAxis";
const QString Y_AXIS_APPEND = ".yAxis";
const QString PLOT_CURVE_SEP = "->";

const int TAB_CREATE_CHILD_CURVE = 0;
const int TAB_CREATE_MATH = 1;

const int CREATE_CHILD_CURVE_COMBO_1D    = E_PLOT_TYPE_1D;
const int CREATE_CHILD_CURVE_COMBO_2D    = E_PLOT_TYPE_2D;
const int CREATE_CHILD_CURVE_FFT_REAL    = E_PLOT_TYPE_REAL_FFT;
const int CREATE_CHILD_CURVE_FFT_COMPLEX = E_PLOT_TYPE_COMPLEX_FFT;

#define NUM_MATH_OPS (7)
const QString mathOpsStr[NUM_MATH_OPS] = {
"ADD",
"SUBTRACT",
"MUTIPLY",
"DIVIDE",
"SHIFT UP",
"SHIFT DOWN",
"LOG()"};

const QString mathOpsSymbol[NUM_MATH_OPS] = {
"+",
"-",
"*",
"/",
"<<",
">>",
"log"};


curveProperties::curveProperties(CurveCommander *curveCmdr, QString plotName, QString curveName, QWidget *parent) :
   QWidget(parent),
   ui(new Ui::curveProperties),
   m_curveCmdr(curveCmdr),
   m_xAxisSrcCmbText(""),
   m_yAxisSrcCmbText(""),
   m_plotNameDestCmbText(""),
   m_mathSrcCmbText(""),
   m_selectedMathOpLeft(0),
   m_selectedMathOpRight(0)
{
   ui->setupUi(this);
   ui->tabWidget->setCurrentIndex(TAB_CREATE_CHILD_CURVE);
   on_cmbPlotType_currentIndexChanged(ui->cmbPlotType->currentIndex());
   updateGuiPlotCurveInfo(plotName, curveName);

   for(int i = 0; i < NUM_MATH_OPS; ++i)
   {
      ui->availableOps->addItem(mathOpsStr[i]);
   }
   ui->availableOps->setCurrentRow(0);

}

curveProperties::~curveProperties()
{
   delete ui;
}

void curveProperties::updateGuiPlotCurveInfo(QString plotName, QString curveName)
{
   tCurveCommanderInfo allCurves = m_curveCmdr->getCurveCommanderInfo();

   // Save current values of the GUI elements.
   m_xAxisSrcCmbText = ui->cmbXAxisSrc->currentText();
   m_yAxisSrcCmbText = ui->cmbYAxisSrc->currentText();
   m_plotNameDestCmbText = ui->cmbDestPlotName->currentText();
   m_mathSrcCmbText = ui->cmbSrcCurve_math->currentText();

   ui->cmbDestPlotName->clear();
   ui->cmbXAxisSrc->clear();
   ui->cmbYAxisSrc->clear();
   ui->cmbSrcCurve_math->clear();

   foreach( QString plotName, allCurves.keys() )
   {
      // Add to dest plot name combo box
      ui->cmbDestPlotName->addItem(plotName);

      tCurveDataInfo* curves = &(allCurves[plotName].curves);
      foreach( QString curveName, curves->keys() )
      {
         QString plotCurveName = plotName + PLOT_CURVE_SEP + curveName;
         if( (*curves)[curveName]->getPlotDim() == E_PLOT_DIM_1D)
         {
            ui->cmbXAxisSrc->addItem(plotCurveName);
            ui->cmbYAxisSrc->addItem(plotCurveName);
            ui->cmbSrcCurve_math->addItem(plotCurveName);
         }
         else
         {
            ui->cmbXAxisSrc->addItem(plotCurveName + X_AXIS_APPEND);
            ui->cmbYAxisSrc->addItem(plotCurveName + X_AXIS_APPEND);
            ui->cmbSrcCurve_math->addItem(plotCurveName + X_AXIS_APPEND);

            ui->cmbXAxisSrc->addItem(plotCurveName + Y_AXIS_APPEND);
            ui->cmbYAxisSrc->addItem(plotCurveName + Y_AXIS_APPEND);
            ui->cmbSrcCurve_math->addItem(plotCurveName + Y_AXIS_APPEND);
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
   trySetComboItemIndex(ui->cmbSrcCurve_math, m_mathSrcCmbText);
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

   trySetComboItemIndex(ui->cmbDestPlotName, plotName);

   cmbBox = ui->cmbSrcCurve_math;
   if(trySetComboItemIndex(cmbBox, plotCurveName1D) == false)
   {
      trySetComboItemIndex(cmbBox, plotCurveName2D);
   }
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
         ui->lblYAxisSrc->setText("Real Source");
         ui->lblXAxisSrc->setVisible(false);
         ui->cmbXAxisSrc->setVisible(false);
         ui->lblYAxisSrc->setVisible(true);
         ui->cmbYAxisSrc->setVisible(true);
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
   int tab = ui->tabWidget->currentIndex();
   if(tab == TAB_CREATE_CHILD_CURVE)
   {
      QString newChildPlotName = ui->cmbDestPlotName->currentText();
      QString newChildCurveName = ui->txtDestCurveName->text();

      if(m_curveCmdr->validCurve(newChildPlotName, newChildCurveName) == false)
      {
         // New Child Plot/Curve does not exist, continue creating the child curve.
         ePlotType plotType = (ePlotType)ui->cmbPlotType->currentIndex();
         if( plotType == E_PLOT_TYPE_1D || plotType == E_PLOT_TYPE_REAL_FFT )
         {
            m_curveCmdr->createChildCurve( newChildPlotName,
                                           newChildCurveName,
                                           plotType,
                                           getSelectedCurveInfo(ui->cmbYAxisSrc));
         }
         else
         {
            m_curveCmdr->createChildCurve( newChildPlotName,
                                           newChildCurveName,
                                           plotType,
                                           getSelectedCurveInfo(ui->cmbXAxisSrc),
                                           getSelectedCurveInfo(ui->cmbYAxisSrc));
         }
      }
      else
      {
         QMessageBox msgBox;
         msgBox.setWindowTitle("Curve Already Exists");
         msgBox.setText(newChildPlotName + PLOT_CURVE_SEP + newChildCurveName + " already exists. Choose another Plot/Curve name.");
         msgBox.setStandardButtons(QMessageBox::Ok);
         msgBox.setDefaultButton(QMessageBox::Ok);
         msgBox.exec();
      }
   }
   else if(tab == TAB_CREATE_MATH)
   {
      tPlotCurveAxis curve = getSelectedCurveInfo(ui->cmbSrcCurve_math);
      CurveData* parentCurve = m_curveCmdr->getCurveData(curve.plotName, curve.curveName);
      if(parentCurve != NULL)
      {
         double sampleRate = atof(ui->txtSampleRate->text().toStdString().c_str());
         MainWindow* parentPlotGui = m_curveCmdr->getMainPlot(curve.plotName);
         if(parentPlotGui != NULL)
         {
            parentPlotGui->setCurveSampleRate(curve.curveName, sampleRate, true);
            parentPlotGui->setCurveMath(curve.curveName, curve.axis, m_mathOps);
         }
      }
   }
}



tPlotCurveAxis curveProperties::getSelectedCurveInfo(QComboBox *cmbBox)
{
   std::string cmbText = cmbBox->currentText().toStdString();

   tPlotCurveAxis retVal;
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

void curveProperties::setMathSampleRate()
{
   tPlotCurveAxis curveInfo = getSelectedCurveInfo(ui->cmbSrcCurve_math);
   CurveData* curve = m_curveCmdr->getCurveData(curveInfo.plotName, curveInfo.curveName);
   if(curve != NULL)
   {
      char sampleRate[50];
      snprintf(sampleRate, sizeof(sampleRate)-1, "%f", (float)curve->getSampleRate());
      ui->txtSampleRate->setText(sampleRate);
   }
}

void curveProperties::on_tabWidget_currentChanged(int index)
{
   updateGuiPlotCurveInfo();
   int tab = ui->tabWidget->currentIndex();
   if(tab == TAB_CREATE_MATH)
   {
      setMathSampleRate();
   }

}

void curveProperties::on_cmbSrcCurve_math_currentIndexChanged(int index)
{
   setMathSampleRate();
}

void curveProperties::displayUserMathOp()
{
   ui->opsOnCurve->clear();

   tMathOpList::iterator iter;

   for(iter = m_mathOps.begin(); iter != m_mathOps.end(); ++iter)
   {
      char number[50];
      snprintf(number, sizeof(number), "%f", iter->num);
      number[sizeof(number)-1] = 0;

      QString displayStr = mathOpsSymbol[iter->op] + " " + QString(number);

      if(iter->op == E_LOG)
      {
         displayStr = mathOpsSymbol[iter->op];
      }

      ui->opsOnCurve->addItem(displayStr);
   }

}

void curveProperties::on_availableOps_currentRowChanged(int currentRow)
{
   if(currentRow >= 0)
      m_selectedMathOpLeft = currentRow;
}

void curveProperties::on_opsOnCurve_currentRowChanged(int currentRow)
{
   if(currentRow >= 0)
      m_selectedMathOpRight = currentRow;
}

void curveProperties::on_cmdOpRight_clicked()
{
   double number = atof(ui->txtNumber->text().toStdString().c_str());

   if(number != 0.0)
   {
      tOperation newOp;

      newOp.op = (eMathOp)m_selectedMathOpLeft;
      newOp.num = number;

      m_mathOps.push_back(newOp);

      displayUserMathOp();
   }
}

void curveProperties::on_cmdOpUp_clicked()
{
   if(m_selectedMathOpRight > 0)
   {
      tMathOpList::iterator first;
      tMathOpList::iterator second = m_mathOps.begin();

      for(int i = 0; i < (m_selectedMathOpRight - 1); ++i)
         second++;
      first = second;
      second++;

      std::iter_swap(first, second);
      displayUserMathOp();

      m_selectedMathOpRight--;
      ui->opsOnCurve->setCurrentRow(m_selectedMathOpRight);
   }
}

void curveProperties::on_cmdOpDown_clicked()
{
   if(m_selectedMathOpRight < (int)(m_mathOps.size()-1))
   {
      tMathOpList::iterator first;
      tMathOpList::iterator second = m_mathOps.begin();

      for(int i = 0; i < m_selectedMathOpRight; ++i)
         second++;
      first = second;
      second++;

      std::iter_swap(first, second);
      displayUserMathOp();

      m_selectedMathOpRight++;
      ui->opsOnCurve->setCurrentRow(m_selectedMathOpRight);
   }
}

void curveProperties::on_cmdOpDelete_clicked()
{
   int count = 0;
   tMathOpList::iterator iter;

   for(iter = m_mathOps.begin(); iter != m_mathOps.end(); )
   {
      if(count == m_selectedMathOpRight)
      {
         m_mathOps.erase(iter++);
         break;
      }
      else
      {
         ++iter;
         ++count;
      }
   }

   displayUserMathOp();
   if(m_selectedMathOpRight >= (int)m_mathOps.size())
   {
      m_selectedMathOpRight = m_mathOps.size() - 1;
      ui->opsOnCurve->setCurrentRow(m_selectedMathOpRight);
   }
}
