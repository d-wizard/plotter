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
#include <QMap>
#include <algorithm>
#include <QFileDialog>
#include <fstream>
#include "overwriterenamedialog.h"
#include "saveRestoreCurve.h"
#include "FileSystemOperations.h"

const QString X_AXIS_APPEND = ".xAxis";
const QString Y_AXIS_APPEND = ".yAxis";

const int TAB_CREATE_CHILD_CURVE = 0;
const int TAB_CREATE_MATH = 1;
const int TAB_RESTORE_MSG = 2;
const int TAB_OPEN_SAVE_CURVE = 3;


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

   for(int i = 0; i < NUM_MATH_OPS; ++i)
   {
      ui->availableOps->addItem(mathOpsStr[i]);
   }
   ui->availableOps->setCurrentRow(0);

   // Initialize the list of all the combo boxes that display PlotName->CurveName
   m_plotCurveCombos.clear();
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbXAxisSrc, E_X_AXIS));
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbYAxisSrc, E_Y_AXIS));
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbSrcCurve_math, E_Y_AXIS));
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbCurveToSave, E_Y_AXIS));

   // Initialize the list of all the combo boxes that display all the plot names
   m_plotNameCombos.clear();
   m_plotNameCombos.append(tCmbBoxAndValue(ui->cmbDestPlotName, E_X_AXIS));
   m_plotNameCombos.append(tCmbBoxAndValue(ui->cmbOpenCurvePlotName, E_X_AXIS));
   m_plotNameCombos.append(tCmbBoxAndValue(m_plotCurveDialog.getPlotNameCombo(), E_X_AXIS));

   // Set current tab index.
   ui->tabWidget->setCurrentIndex(TAB_CREATE_CHILD_CURVE);
   on_cmbPlotType_currentIndexChanged(ui->cmbPlotType->currentIndex());

   // Initialize GUI elements.
   updateGuiPlotCurveInfo(plotName, curveName);
}

curveProperties::~curveProperties()
{
   delete ui;
}

void curveProperties::updateGuiPlotCurveInfo(QString plotName, QString curveName)
{
   tCurveCommanderInfo allCurves = m_curveCmdr->getCurveCommanderInfo();

   // Save current values of the GUI elements. Clear the combo box members.
   for(int i = 0; i < m_plotCurveCombos.size(); ++i)
   {
      m_plotCurveCombos[i].cmbBoxVal = m_plotCurveCombos[i].cmbBoxPtr->currentText();
      m_plotCurveCombos[i].cmbBoxPtr->clear();
   }

   // Save current values of the GUI elements. Clear Plot Name combo boxes.
   for(int i = 0; i < m_plotNameCombos.size(); ++i)
   {
      m_plotNameCombos[i].cmbBoxVal = m_plotNameCombos[i].cmbBoxPtr->currentText();
      m_plotNameCombos[i].cmbBoxPtr->clear();
   }

   foreach( QString curPlotName, allCurves.keys() )
   {
      // Add to dest plot name combo box
      for(int i = 0; i < m_plotNameCombos.size(); ++i)
      {
         m_plotNameCombos[i].cmbBoxPtr->addItem(curPlotName);
      }

      tCurveDataInfo* curves = &(allCurves[curPlotName].curves);
      foreach( QString curveName, curves->keys() )
      {
         QString plotCurveName = curPlotName + PLOT_CURVE_SEP + curveName;
         if( (*curves)[curveName]->getPlotDim() == E_PLOT_DIM_1D)
         {
            for(int i = 0; i < m_plotCurveCombos.size(); ++i)
            {
               m_plotCurveCombos[i].cmbBoxPtr->addItem(plotCurveName);
            }
         }
         else
         {
            for(int i = 0; i < m_plotCurveCombos.size(); ++i)
            {
               m_plotCurveCombos[i].cmbBoxPtr->addItem(plotCurveName + X_AXIS_APPEND);
               m_plotCurveCombos[i].cmbBoxPtr->addItem(plotCurveName + Y_AXIS_APPEND);
            }
         }
      }
   }

   if(plotName != "" && curveName != "")
   {
      // User brought this GUI up from a plot window. Set the selected value of all the plot/curve
      // combo boxes to the plot/curve name of the selected curve in the plot window.
      setCombosToPlotCurve(plotName, curveName);
   }
   else
   {
      // This function is being called because some curve has changed (created/deleted/updated).
      // Attempt to set the selected value of the plot/curve combos back to the value
      // it was before the change.
      setCombosToPrevValues();
   }
}

void curveProperties::setCombosToPrevValues()
{
   // Set Plot/Curve Name Combos to previous values
   for(int i = 0; i < m_plotCurveCombos.size(); ++i)
   {
      trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, m_plotCurveCombos[i].cmbBoxVal);
   }

   // Set Plot Name Combos to previous values
   for(int i = 0; i < m_plotNameCombos.size(); ++i)
   {
      trySetComboItemIndex(m_plotNameCombos[i].cmbBoxPtr, m_plotNameCombos[i].cmbBoxVal);
   }
}

void curveProperties::setCombosToPlotCurve(QString plotName, QString curveName)
{
   QString plotCurveName1D = plotName + PLOT_CURVE_SEP + curveName;
   QString plotCurveName2Dx = plotCurveName1D + X_AXIS_APPEND;
   QString plotCurveName2Dy = plotCurveName1D + Y_AXIS_APPEND;

   // Set Plot/Curve Name Combos
   for(int i = 0; i < m_plotCurveCombos.size(); ++i)
   {
      // Try to set to 1D Curve Name.
      if(trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, plotCurveName1D) == false)
      {
         // 1D Curve Name doesn't exists, must be 2D. Set to 2D Curve Name.
         if(m_plotCurveCombos[i].defaultAxis == E_X_AXIS)
            trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, plotCurveName2Dx);
         else
            trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, plotCurveName2Dy);
      }
   }

   // Set Plot Name Combos
   for(int i = 0; i < m_plotNameCombos.size(); ++i)
   {
      trySetComboItemIndex(m_plotNameCombos[i].cmbBoxPtr, plotName);
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
   bool xVis = false;
   bool yVis = false;
   bool slice = ui->chkSrcSlice->checkState() == Qt::Checked;
   switch(index)
   {
      case E_PLOT_TYPE_1D:
         ui->lblYAxisSrc->setText("Y Axis Source");
         yVis = true;
      break;
      case E_PLOT_TYPE_2D:
         ui->lblXAxisSrc->setText("X Axis Source");
         ui->lblYAxisSrc->setText("Y Axis Source");
         xVis = true;
         yVis = true;
      break;
      case E_PLOT_TYPE_REAL_FFT:
         ui->lblYAxisSrc->setText("Real Source");
         yVis = true;
      break;
      case E_PLOT_TYPE_COMPLEX_FFT:
      case E_PLOT_TYPE_AM_DEMOD:
      case E_PLOT_TYPE_FM_DEMOD:
      case E_PLOT_TYPE_PM_DEMOD:
         ui->lblXAxisSrc->setText("Real Source");
         ui->lblYAxisSrc->setText("Imag Source");
         xVis = true;
         yVis = true;
      break;

   }
   ui->lblXAxisSrc->setVisible(xVis);
   ui->cmbXAxisSrc->setVisible(xVis);
   ui->spnXSrcStart->setVisible(xVis && slice);
   ui->spnXSrcStop->setVisible(xVis && slice);
   ui->cmdXUseZoomForSlice->setVisible(xVis && slice);

   ui->lblYAxisSrc->setVisible(yVis);
   ui->cmbYAxisSrc->setVisible(yVis);
   ui->spnYSrcStart->setVisible(yVis && slice);
   ui->spnYSrcStop->setVisible(yVis && slice);
   ui->cmdYUseZoomForSlice->setVisible(yVis && slice);
}

void curveProperties::on_cmdApply_clicked()
{
   int tab = ui->tabWidget->currentIndex();
   if(tab == TAB_CREATE_CHILD_CURVE)
   {
      QString newChildPlotName = ui->cmbDestPlotName->currentText();
      QString newChildCurveName = ui->txtDestCurveName->text();

      if(newChildPlotName != "" && newChildCurveName != "")
      {
         if(m_curveCmdr->validCurve(newChildPlotName, newChildCurveName) == false)
         {
            // New Child plot->curve does not exist, continue creating the child curve.
            ePlotType plotType = (ePlotType)ui->cmbPlotType->currentIndex();
            if( plotTypeHas2DInput(plotType) == false )
            {
               tParentCurveInfo yAxisParent;
               yAxisParent.dataSrc = getSelectedCurveInfo(ui->cmbYAxisSrc);
               if(ui->chkSrcSlice->checkState() == Qt::Checked)
               {
                  yAxisParent.startIndex = ui->spnYSrcStart->value();
                  yAxisParent.stopIndex = ui->spnYSrcStop->value();
               }
               else
               {
                  yAxisParent.startIndex = 0;
                  yAxisParent.stopIndex = 0;
               }
               m_curveCmdr->createChildCurve( newChildPlotName,
                                              newChildCurveName,
                                              plotType,
                                              yAxisParent);
            }
            else
            {
               tParentCurveInfo xAxisParent;
               xAxisParent.dataSrc = getSelectedCurveInfo(ui->cmbXAxisSrc);

               tParentCurveInfo yAxisParent;
               yAxisParent.dataSrc = getSelectedCurveInfo(ui->cmbYAxisSrc);


               if(ui->chkSrcSlice->checkState() == Qt::Checked)
               {
                  xAxisParent.startIndex = ui->spnXSrcStart->value();
                  xAxisParent.stopIndex = ui->spnXSrcStop->value();
                  yAxisParent.startIndex = ui->spnYSrcStart->value();
                  yAxisParent.stopIndex = ui->spnYSrcStop->value();
               }
               else
               {
                  xAxisParent.startIndex = 0;
                  xAxisParent.stopIndex = 0;
                  yAxisParent.startIndex = 0;
                  yAxisParent.stopIndex = 0;
               }

               m_curveCmdr->createChildCurve( newChildPlotName,
                                              newChildCurveName,
                                              plotType,
                                              xAxisParent,
                                              yAxisParent);
            }
         }
         else
         {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Curve Already Exists");
            msgBox.setText(newChildPlotName + PLOT_CURVE_SEP + newChildCurveName + " already exists. Choose another plot->curve name.");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
         }
      } // End if(newChildPlotName != "" && newChildCurveName != "")
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
            bool curveHidden = ui->chkHideCurve->checkState() == Qt::Checked;
            parentPlotGui->setCurveProperties(curve.curveName, curve.axis, sampleRate, m_mathOps, curveHidden);
         }
      }
   }
   else if(tab == TAB_RESTORE_MSG)
   {
      QModelIndexList indexes = ui->storedMsgs->selectionModel()->selectedIndexes();

      if(indexes.size() > 0)
      {
         // Cycle through list, in reverse order (oldest plot messages will be restored first)

         // We are potentially restoring many different curves. Some or all of these curves could
         // already exist. For each individual curve name that exists, ask the user whether
         // to overwrite the existing curve, rename the curve to restore or cancel the restore.
         // Save the users response for each unique curve name to restore, so we only ask
         // the user how to handle restoring curves that match existing curve names.
         QModelIndexList::Iterator iter = indexes.end();
         QMap<tPlotCurveName, bool> validPlotCurveNames;         // Indicates whether to cancel restore for plot->curve name.
         QMap<tPlotCurveName, tPlotCurveName> renamedPlotCurves; // Stores renamed plot->curve name.

         do
         {
            --iter;
            int storedMsgIndex = iter->row();
            if(storedMsgIndex >= 0 && storedMsgIndex < m_storedMsgs.size())
            {
               tPlotCurveName plotCurve = m_storedMsgs[storedMsgIndex].name;
               bool restoreCurve = false;

               // Check if user has already renamed the plot->curve name of the message to restore.
               QMap<tPlotCurveName, tPlotCurveName>::Iterator renameIter = renamedPlotCurves.find(plotCurve);
               if(renameIter != renamedPlotCurves.end())
               {
                  // Already renaming this restore plot->curve name. Use use new name and restore message.
                  restoreCurve = true;
                  plotCurve = renamedPlotCurves[plotCurve];
               }
               else
               {
                  // Not renaming, check if the user has cancelled the restore of this plot->curve name.
                  QMap<tPlotCurveName, bool>::Iterator validIter = validPlotCurveNames.find(plotCurve);

                  if(validIter != validPlotCurveNames.end())
                  {
                     // Already determined whether the plot->curve is valid, use previously determined value.
                     restoreCurve = validIter.value();
                  }
                  else
                  {
                     // Check whether the plot->curve is valid, if it exists ask the user whether to overwrite/rename/cancel.
                     restoreCurve = validateNewPlotCurveName(plotCurve.plot, plotCurve.curve);

                     if(restoreCurve == true && plotCurve != m_storedMsgs[storedMsgIndex].name)
                     {
                        // User renamed the plot->curve, add to map of renamed plot->curves.
                        renamedPlotCurves[m_storedMsgs[storedMsgIndex].name] = plotCurve;
                     }

                     validPlotCurveNames[plotCurve] = restoreCurve;
                  }
               }
               if(restoreCurve)
                  m_curveCmdr->restorePlotMsg(m_storedMsgs[storedMsgIndex], plotCurve);
            }

         }while(iter != indexes.begin());
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


void curveProperties::on_cmdClose_clicked()
{
   m_curveCmdr->curvePropertiesGuiClose();
}

void curveProperties::closeEvent(QCloseEvent* /*event*/)
{
   m_curveCmdr->curvePropertiesGuiClose();
}

void curveProperties::setMathSampleRate(CurveData* curve)
{
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

   bool showApplyButton = false;

   switch(tab)
   {
      case TAB_CREATE_CHILD_CURVE:
      {
         showApplyButton = true;
      }
      break;

      case TAB_CREATE_MATH:
      {
         tPlotCurveAxis curveInfo = getSelectedCurveInfo(ui->cmbSrcCurve_math);
         CurveData* curve = m_curveCmdr->getCurveData(curveInfo.plotName, curveInfo.curveName);

         setMathSampleRate(curve);
         setUserMathFromSrc(curveInfo, curve);
         setCurveHiddenCheckBox(curve);
         displayUserMathOp();

         showApplyButton = true;
      }
      break;

      case TAB_RESTORE_MSG:
      {
         fillRestoreFilters();
         fillRestoreTabListBox();

         showApplyButton = true;
      }
      break;

      case TAB_OPEN_SAVE_CURVE:
      {
         showApplyButton = false;
      }
      break;

   }

   ui->cmdApply->setVisible(showApplyButton);

}

void curveProperties::on_cmbSrcCurve_math_currentIndexChanged(int index)
{
   tPlotCurveAxis curveInfo = getSelectedCurveInfo(ui->cmbSrcCurve_math);
   CurveData* curve = m_curveCmdr->getCurveData(curveInfo.plotName, curveInfo.curveName);

   setMathSampleRate(curve);
   setUserMathFromSrc(curveInfo, curve);
   setCurveHiddenCheckBox(curve);
   displayUserMathOp();
}

void curveProperties::setUserMathFromSrc(tPlotCurveAxis &curveInfo, CurveData* curve)
{
   if(curve != NULL)
   {
      m_mathOps = curve->getMathOps(curveInfo.axis);
   }
}

void curveProperties::setCurveHiddenCheckBox(CurveData* curve)
{
   if(curve != NULL)
   {
      ui->chkHideCurve->setChecked(curve->getHidden());
   }
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

   if(number != 0.0 || m_selectedMathOpLeft == E_LOG)
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

void curveProperties::on_chkSrcSlice_clicked()
{
   on_cmbPlotType_currentIndexChanged(ui->cmbPlotType->currentIndex());
}

void curveProperties::fillRestoreTabListBox()
{
   ui->storedMsgs->clear();
   m_storedMsgs.clear();

   QVector<tStoredMsg> storedMsgs;
   m_curveCmdr->getStoredPlotMsgs(storedMsgs);
   for(QVector<tStoredMsg>::iterator iter = storedMsgs.begin(); iter != storedMsgs.end(); ++iter)
   {
      bool validPlotName = false;
      bool validCurveName = false;

      int plotIndex = ui->cmbRestorePlotNameFilter->currentIndex() - 1;   // index 0 is all plots,  subtract 1 to make index match member variable index
      int curveIndex = ui->cmbRestoreCurveNameFilter->currentIndex() - 1; // index 0 is all curves, subtract 1 to make index match member variable index

      if( plotIndex < 0 || // When index is less than 0, assume all plot names are valid
          ( plotIndex < m_restoreFilterPlotName.size() &&
            m_restoreFilterPlotName[plotIndex] == (*iter).name.plot ) )
      {
         validPlotName = true;
      }

      if( curveIndex < 0 || // When index is less than 0, assume all curve names are valid
          ( curveIndex < m_restoreFilterCurveName.size() &&
            m_restoreFilterCurveName[curveIndex] == (*iter).name.curve ) )
      {
         validCurveName = true;
      }

      if(validPlotName && validCurveName)
      {
         // I like the msgTime toString except for the year at the end.
         // Remove last 5 characters from toString return to remove the year (and space).
         // This should be replaced with a date/time format.
         QString time((*iter).msgTime.toString());
         time = dString::Slice(time.toStdString(), 0, -5).c_str();
         ui->storedMsgs->addItem("[" + time + "] " + (*iter).name.plot + "->" + (*iter).name.curve);
         m_storedMsgs.push_back(*iter);
      }
   }

}

void curveProperties::fillRestoreFilters()
{
   m_restoreFilterPlotName.clear();
   m_restoreFilterCurveName.clear();

   QVector<tStoredMsg> storedMsgs;
   m_curveCmdr->getStoredPlotMsgs(storedMsgs);
   // Add unique plot->curve names to lists.
   for(QVector<tStoredMsg>::iterator iter = storedMsgs.begin(); iter != storedMsgs.end(); ++iter)
   {
      if(m_restoreFilterPlotName.indexOf(iter->name.plot) < 0)
         m_restoreFilterPlotName.append(iter->name.plot);
      if(m_restoreFilterCurveName.indexOf(iter->name.curve) < 0)
         m_restoreFilterCurveName.append(iter->name.curve);
   }

   ui->cmbRestorePlotNameFilter->clear();
   ui->cmbRestoreCurveNameFilter->clear();
   ui->cmbRestorePlotNameFilter->addItem("*");
   ui->cmbRestoreCurveNameFilter->addItem("*");

   for(int i = 0; i < m_restoreFilterPlotName.size(); ++i)
   {
      ui->cmbRestorePlotNameFilter->addItem(m_restoreFilterPlotName[i]);
   }
   for(int i = 0; i < m_restoreFilterCurveName.size(); ++i)
   {
      ui->cmbRestoreCurveNameFilter->addItem(m_restoreFilterCurveName[i]);
   }
}


void curveProperties::on_cmbRestorePlotNameFilter_currentIndexChanged(int index)
{
   fillRestoreTabListBox();
}

void curveProperties::on_cmbRestoreCurveNameFilter_currentIndexChanged(int index)
{
   fillRestoreTabListBox();
}

// Get source curve start and stop indexes from the current zoom
// This will only work if the source curve is 1D
void curveProperties::on_cmdXUseZoomForSlice_clicked()
{
   tPlotCurveAxis curve = getSelectedCurveInfo(ui->cmbXAxisSrc);
   CurveData* parentCurve = m_curveCmdr->getCurveData(curve.plotName, curve.curveName);
   if(parentCurve != NULL && parentCurve->getPlotDim() == E_PLOT_DIM_1D)
   {
      // Valid 1D parent, continue
      MainWindow* mainPlot = m_curveCmdr->getMainPlot(curve.plotName);
      if(mainPlot != NULL)
      {
         // Get zoom dimensions.
         maxMinXY dim = mainPlot->getZoomDimensions();

         // Use sample rate to convert back to start/stop indexes of source data.
         double sampleRate = parentCurve->getSampleRate();
         if(sampleRate != 0.0)
         {
            dim.minX *= sampleRate;
            dim.maxX *= sampleRate;
         }

         // Bound.
         if(dim.minX < 0)
            dim.minX = 0;
         if(dim.maxX >= parentCurve->getNumPoints())
            dim.maxX = parentCurve->getNumPoints() - 1;

         // Set GUI elements.
         ui->spnXSrcStart->setValue(dim.minX);
         ui->spnXSrcStop->setValue(dim.maxX);
      }
   }
   else
   {
      ui->spnXSrcStart->setValue(0);
      ui->spnXSrcStop->setValue(0);
   }
}

// Get source curve start and stop indexes from the current zoom
// This will only work if the source curve is 1D
void curveProperties::on_cmdYUseZoomForSlice_clicked()
{
   tPlotCurveAxis curve = getSelectedCurveInfo(ui->cmbYAxisSrc);
   CurveData* parentCurve = m_curveCmdr->getCurveData(curve.plotName, curve.curveName);
   if(parentCurve != NULL && parentCurve->getPlotDim() == E_PLOT_DIM_1D)
   {
      // Valid 1D parent, continue
      MainWindow* mainPlot = m_curveCmdr->getMainPlot(curve.plotName);
      if(mainPlot != NULL)
      {
         // Get zoom dimensions.
         maxMinXY dim = mainPlot->getZoomDimensions();

         // Use sample rate to convert back to start/stop indexes of source data.
         double sampleRate = parentCurve->getSampleRate();
         if(sampleRate != 0.0)
         {
            dim.minX *= sampleRate;
            dim.maxX *= sampleRate;
         }

         // Bound.
         if(dim.minX < 0)
            dim.minX = 0;
         if(dim.maxX >= parentCurve->getNumPoints())
            dim.maxX = parentCurve->getNumPoints() - 1;

         // Set GUI elements.
         ui->spnYSrcStart->setValue(dim.minX);
         ui->spnYSrcStop->setValue(dim.maxX);
      }
   }
   else
   {
      ui->spnYSrcStart->setValue(0);
      ui->spnYSrcStop->setValue(0);
   }
}

void curveProperties::on_cmdSaveCurveToFile_clicked()
{
   tPlotCurveAxis toSave = getSelectedCurveInfo(ui->cmbCurveToSave);
   CurveData* toSaveCurveData = m_curveCmdr->getCurveData(toSave.plotName, toSave.curveName);
   if(toSaveCurveData != NULL)
   {
      QString fileName = QFileDialog::getSaveFileName(this, tr("Save Curve To File"),
                                                       "",
                                                       tr("Curves (*.curve)"));
      SaveCurve packedCurve(toSaveCurveData);

      fso::WriteFile(fileName.toStdString(), &packedCurve.packedCurveData[0], packedCurve.packedCurveData.size());

   }

}

void curveProperties::on_cmdOpenCurveFromFile_clicked()
{
   QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                   "",
                                                   tr("Curves (*.curve);;All files (*.*)"));
   std::vector<char> curveFile;
   fso::ReadBinaryFile(fileName.toStdString(), curveFile);

   // Get plot name.
   QString plotName = ui->cmbOpenCurvePlotName->currentText();
   if(plotName == "")
   {
      plotName = "New Plot";
   }

   RestoreCurve restoreCurve(curveFile);

   if(restoreCurve.isValid)
   {
      tSaveRestoreCurveParams* p = &restoreCurve.params;
      QString newCurveName = p->curveName;

      if(validateNewPlotCurveName(plotName, newCurveName))
      {
         if(p->plotDim == E_PLOT_DIM_1D)
         {
            // 1D Plot
            m_curveCmdr->create1dCurve(plotName, newCurveName, p->plotType, p->yOrigPoints);
         }
         else
         {
            // 2D Plot
            m_curveCmdr->create2dCurve(plotName, newCurveName, p->xOrigPoints, p->yOrigPoints);
         }

         MainWindow* plot = m_curveCmdr->getMainPlot(plotName);
         if(plot != NULL)
         {
            plot->setCurveProperties(newCurveName, E_X_AXIS, p->sampleRate, p->mathOpsXAxis, false);
            plot->setCurveProperties(newCurveName, E_Y_AXIS, p->sampleRate, p->mathOpsYAxis, false);
         }
      }

   } // End if(restoreCurve.isValid)
}

bool curveProperties::validateNewPlotCurveName(QString& plotName, QString& curveName)
{
   bool namesAreValid = false;
   bool finished = false;

   if(m_curveCmdr->validCurve(plotName, curveName) == false)
   {
      // plot->curve does not exist, the input names are valid.
      namesAreValid = true;
   }
   else
   {
      // plot->curve does exists, ask user whether to overwrite existing curve, rename
      // the new curve or cancel creation of new curve.
      do
      {
         overwriteRenameDialog overRenameDlg(NULL);
         ePlotExistsReturn whatToDo = overRenameDlg.askUserAboutExistingPlot(plotName, curveName);
         if(whatToDo == RENAME)
         {
            bool renamed = m_plotCurveDialog.getPlotCurveNameFromUser(plotName, curveName);
            if(renamed == true && m_curveCmdr->validCurve(plotName, curveName) == false)
            {
               // Renamed to a unique plot->curve name. Return valid.
               namesAreValid = true;
               finished = true;
            }
            else if(renamed == false)
            {
               // User clicked cancel. Return invalid.
               namesAreValid = false;
               finished = true;
            }
         }
         else if(whatToDo == OVERWRITE)
         {
            // Overwrite. Do not rename. Return valid.
            namesAreValid = true;
            finished = true;
         }
         else
         {
            // Cancel. Return invalid.
            namesAreValid = false;
            finished = true;
         }

      }while(finished == false);
   }

   return namesAreValid;
}


