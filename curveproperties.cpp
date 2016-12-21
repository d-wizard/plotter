/* Copyright 2014 - 2016 Dan Williams. All Rights Reserved.
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
#include <QMessageBox>
#include <fstream>
#include "overwriterenamedialog.h"
#include "saveRestoreCurve.h"
#include "FileSystemOperations.h"
#include "persistentParameters.h"
#include "localPlotCreate.h"

const QString X_AXIS_APPEND = ".xAxis";
const QString Y_AXIS_APPEND = ".yAxis";

const int TAB_CREATE_CHILD_CURVE = 0;
const int TAB_CREATE_MATH = 1;
const int TAB_RESTORE_MSG = 2;
const int TAB_OPEN_SAVE_CURVE = 3;
const int TAB_IP_BLOCK = 4;
const int TAB_PROPERTIES = 5;

const double CURVE_PROP_PI = 3.1415926535897932384626433832795028841971693993751058;
const double CURVE_PROP_2PI = 6.2831853071795864769252867665590057683943387987502116;
const double CURVE_PROP_E = 2.7182818284590452353602874713526624977572470936999595;

const QString mathOpsStr[] = {
"ADD",
"MUTIPLY",
"DIVIDE",
"SHIFT UP",
"SHIFT DOWN",
"POWER",
"LOG()",
"MOD()",
"ABS()"};

const QString mathOpsSymbol[] = {
"+",
"*",
"/",
"<<",
">>",
"^",
"log",
"mod",
"abs"};


const QString mathOpsValueLabel[] = {
"Value to Add",
"Value to Multiply by",
"Value to Divide by",
"Bits to Shift Up by",
"Bits to Shift Down by",
"Raise to Power Value",
"Base of log",
"Modulus Denominator"
""};

const QString plotTypeNames[] = {
   "1D",
   "2D",
   "FFT",
   "FFT",
   "AM",
   "FM",
   "PM",
   "Avg",
   "FFT",
   "FFT",
   "Delta",
   "Sum",
   "Math"
};



curveProperties::curveProperties(CurveCommander *curveCmdr, QString plotName, QString curveName, QWidget *parent) :
   QWidget(parent),
   m_ipBlocker(curveCmdr->getIpBlocker()),
   ui(new Ui::curveProperties),
   m_curveCmdr(curveCmdr),
   m_selectedMathOpLeft(0),
   m_selectedMathOpRight(0),
   m_childCurveNewPlotNameUser(""),
   m_childCurveNewCurveNameUser(""),
   m_prevChildCurvePlotTypeIndex(-1)
{
   ui->setupUi(this);

   for(unsigned int i = 0; i < ARRAY_SIZE(mathOpsStr); ++i)
   {
      ui->availableOps->addItem(mathOpsStr[i]);
   }

   unsigned int initialAvailableMathOpSelectionIndex = 0;
   ui->availableOps->setCurrentRow(initialAvailableMathOpSelectionIndex);
   ui->lblMapOpValueLabel->setText(mathOpsValueLabel[initialAvailableMathOpSelectionIndex]);

   // Initialize the list of all the combo boxes that display PlotName->CurveName
   m_plotCurveCombos.clear();
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbXAxisSrc, tCmbBoxAndValue::E_PREFERRED_AXIS_X));
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbYAxisSrc, tCmbBoxAndValue::E_PREFERRED_AXIS_Y));
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbSrcCurve_math));
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbCurveToSave));
   m_plotCurveCombos.append(tCmbBoxAndValue(ui->cmbPropPlotCurveName, false));

   // Initialize the list of all the combo boxes that display all the plot names
   m_plotNameCombos.clear();
   m_plotNameCombos.append(tCmbBoxAndValue(ui->cmbDestPlotName));
   m_plotNameCombos.append(tCmbBoxAndValue(ui->cmbOpenCurvePlotName));
   m_plotNameCombos.append(tCmbBoxAndValue(ui->cmbPlotToSave));
   m_plotNameCombos.append(tCmbBoxAndValue(ui->cmbIpBlockPlotNames));

   // Set current tab index.
   ui->tabWidget->setCurrentIndex(TAB_CREATE_CHILD_CURVE);
   on_cmbPlotType_currentIndexChanged(ui->cmbPlotType->currentIndex());

   // Initialize GUI elements.
   updateGuiPlotCurveInfo(plotName, curveName);

   // Set Suggested Plot / Curve Names.
   setUserChildPlotNames();

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

   QList<QString> curveNames;

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
         CurveData* curveData = (*curves)[curveName];
         if(curPlotName == plotName && curveData->isDisplayed())
         {
            curveNames.push_back(curveName); // Fill in list of all curve names for the given plot.
         }

         QString plotCurveName = curPlotName + PLOT_CURVE_SEP + curveName;
         if( curveData->getPlotDim() == E_PLOT_DIM_1D)
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
               if(m_plotCurveCombos[i].displayAxesSeparately)
               {
                  m_plotCurveCombos[i].cmbBoxPtr->addItem(plotCurveName + X_AXIS_APPEND);
                  m_plotCurveCombos[i].cmbBoxPtr->addItem(plotCurveName + Y_AXIS_APPEND);
               }
               else
               {
                  m_plotCurveCombos[i].cmbBoxPtr->addItem(plotCurveName);
               }
            }
         }
      }
   }

   if(plotName != "" && curveName != "")
   {
      // User brought this GUI up from a plot window. Set the selected value of all the plot/curve
      // combo boxes to the plot/curve name of the selected curve in the plot window.

      QString defaultRealCurveName;
      QString defaultImagCurveName;
      findRealImagCurveNames(curveNames, curveName, defaultRealCurveName, defaultImagCurveName);

      setCombosToPlotCurve(plotName, curveName, defaultRealCurveName, defaultImagCurveName);
   }
   else
   {
      // This function is being called because some curve has changed (created/deleted/updated).
      // Attempt to set the selected value of the plot/curve combos back to the value
      // it was before the change.
      setCombosToPrevValues();
   }

   if(ui->tabWidget->currentIndex() == TAB_PROPERTIES)
   {
      fillInPropTab();
   }

   setUserChildPlotNames();
}

void curveProperties::findRealImagCurveNames(QList<QString>& curveNameList, const QString& defaultCurveName, QString& realCurveName, QString& imagCurveName)
{
   QString commonRealImagCurveNamePairs[] = {
      "I", "Q",
      "real", "imag",
      "re", "im"
   };

   QString alternateName = ""; // If no match is found, set the imag curve name to something other than the default.
   size_t numPairs = sizeof(commonRealImagCurveNamePairs) / sizeof(*commonRealImagCurveNamePairs) / 2;

   bool matchFound = false;
   for(size_t i = 0; i < numPairs; ++i)
   {
      QString re = commonRealImagCurveNamePairs[2*i  ].toLower();
      QString im = commonRealImagCurveNamePairs[2*i+1].toLower();

      bool reMatchFound = false;
      bool imMatchFound = false;
      QString reMatchCurveName;
      QString imMatchCurveName;
      foreach( QString curveName, curveNameList )
      {
         QString curveNameLower = curveName.toLower();
         if(alternateName == "" && curveName != defaultCurveName)
         {
            alternateName = curveName; // Set alternate curve name to a curve that isn't the default curve.
         }

         if(curveNameLower == re)
         {
            reMatchCurveName = curveName;
            reMatchFound = true;
         }
         if(curveNameLower == im)
         {
            imMatchCurveName = curveName;
            imMatchFound = true;
         }
         if(reMatchFound && imMatchFound)
         {
            realCurveName = reMatchCurveName;
            imagCurveName = imMatchCurveName;
            matchFound = true;
            return;
         }
      } // End foreach( QString curveName, curveNameList )

      if(matchFound)
      {
         break;
      }
   } // End for(size_t i = 0; i < numPairs; ++i)

   if(!matchFound)
   {
      realCurveName = defaultCurveName;
      imagCurveName = alternateName != "" ? alternateName : defaultCurveName;
   }
}

// This function is called when an existing plot has changed (i.e. no new plot has been created).
void curveProperties::existingPlotsChanged()
{
   if(ui->tabWidget->currentIndex() == TAB_PROPERTIES)
   {
      fillInPropTab();
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

void curveProperties::setCombosToPlotCurve(const QString& plotName, const QString& curveName, const QString& realCurveName, const QString& imagCurveName)
{
   QString plotCurveName1D_real = plotName + PLOT_CURVE_SEP + realCurveName;
   QString plotCurveName1D_imag = plotName + PLOT_CURVE_SEP + imagCurveName;
   QString plotCurveName1D = plotName + PLOT_CURVE_SEP + curveName;
   QString plotCurveName2Dx = plotCurveName1D + X_AXIS_APPEND;
   QString plotCurveName2Dy = plotCurveName1D + Y_AXIS_APPEND;

   // Set Plot/Curve Name Combos
   for(int i = 0; i < m_plotCurveCombos.size(); ++i)
   {
      bool updateComboSuccess = false;

      if(m_plotCurveCombos[i].preferredAxis != tCmbBoxAndValue::E_PREFERRED_AXIS_DONT_CARE)
      {
         if( m_plotCurveCombos[i].preferredAxis == tCmbBoxAndValue::E_PREFERRED_AXIS_X &&
             trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, plotCurveName1D_real) )
         {
            updateComboSuccess = true;
         }
         else if( m_plotCurveCombos[i].preferredAxis == tCmbBoxAndValue::E_PREFERRED_AXIS_Y &&
                  trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, plotCurveName1D_imag) )
         {
            updateComboSuccess = true;
         }
      }

      // Try to set to 1D Curve Name.
      if(updateComboSuccess == false)
      {
         if(trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, plotCurveName1D) == false)
         {
            // 1D Curve Name doesn't exists, must be 2D. Set to 2D Curve Name.
            if(m_plotCurveCombos[i].preferredAxis == tCmbBoxAndValue::E_PREFERRED_AXIS_Y)
               trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, plotCurveName2Dy);
            else
               trySetComboItemIndex(m_plotCurveCombos[i].cmbBoxPtr, plotCurveName2Dx);
         }
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
   bool xVis = true;
   bool yVis = plotTypeHas2DInput((ePlotType)index);
   bool windowChkVis = false;
   bool slice = ui->chkSrcSlice->checkState() == Qt::Checked;
   bool mathCmbVis = false;

   storeUserChildPlotNames((ePlotType)m_prevChildCurvePlotTypeIndex);
   m_prevChildCurvePlotTypeIndex = index;

   switch(index)
   {
      case E_PLOT_TYPE_1D:
      case E_PLOT_TYPE_AVERAGE:
      case E_PLOT_TYPE_DELTA:
      case E_PLOT_TYPE_SUM:
         ui->lblXAxisSrc->setText("Source");
      break;
      case E_PLOT_TYPE_2D:
         ui->lblXAxisSrc->setText("X Axis Source");
         ui->lblYAxisSrc->setText("Y Axis Source");
      break;
      case E_PLOT_TYPE_REAL_FFT:
      case E_PLOT_TYPE_DB_POWER_FFT_REAL:
         ui->lblXAxisSrc->setText("Real Source");
         windowChkVis = true;
      break;
      case E_PLOT_TYPE_COMPLEX_FFT:
      case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
         windowChkVis = true;
      case E_PLOT_TYPE_AM_DEMOD:
      case E_PLOT_TYPE_FM_DEMOD:
      case E_PLOT_TYPE_PM_DEMOD:
         ui->lblXAxisSrc->setText("Real Source");
         ui->lblYAxisSrc->setText("Imag Source");
      break;
      case E_PLOT_TYPE_MATH_BETWEEN_CURVES:
         ui->lblXAxisSrc->setText("Math LHS");
         ui->lblYAxisSrc->setText("Math RHS");
         mathCmbVis = true;
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

   ui->lblAvgAmount->setVisible(index == E_PLOT_TYPE_AVERAGE);
   ui->txtAvgAmount->setVisible(index == E_PLOT_TYPE_AVERAGE);

   ui->chkWindow->setVisible(windowChkVis);
   ui->cmbChildMathOperators->setVisible(mathCmbVis);

   setUserChildPlotNames();
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
               tParentCurveInfo axisParent;
               axisParent.dataSrc = getSelectedCurveInfo(ui->cmbXAxisSrc);
               if(ui->chkSrcSlice->checkState() == Qt::Checked)
               {
                  axisParent.startIndex = ui->spnXSrcStart->value();
                  axisParent.stopIndex = ui->spnXSrcStop->value();
               }
               else
               {
                  axisParent.startIndex = 0;
                  axisParent.stopIndex = 0;
               }

               axisParent.windowFFT = ui->chkWindow->isChecked();
               axisParent.avgAmount = atof(ui->txtAvgAmount->text().toStdString().c_str());

               m_curveCmdr->createChildCurve( newChildPlotName,
                                              newChildCurveName,
                                              plotType,
                                              axisParent);
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

               // Read values from GUI.
               xAxisParent.windowFFT = ui->chkWindow->isChecked();
               xAxisParent.mathBetweenCurvesOperator =
                  (eMathBetweenCurves_operators)ui->cmbChildMathOperators->currentIndex();

               // Y Axis values need to match X Axis value.
               yAxisParent.windowFFT = xAxisParent.windowFFT;
               yAxisParent.mathBetweenCurvesOperator = xAxisParent.mathBetweenCurvesOperator;

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

      // Child curve has been created. Clear user input of plot / curve name.
      m_childCurveNewPlotNameUser = "";
      m_childCurveNewCurveNameUser = "";
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
            parentPlotGui->setCurveProperties(curve.curveName, curve.axis, sampleRate, m_mathOps);
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
   else if(tab == TAB_PROPERTIES)
   {
      propTabApply();
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
   int tab = ui->tabWidget->currentIndex();

   storeUserChildPlotNames((ePlotType)tab);

   updateGuiPlotCurveInfo();

   bool showApplyButton = false;

   switch(tab)
   {
      case TAB_CREATE_CHILD_CURVE:
      {
         showApplyButton = true;
         setUserChildPlotNames();
      }
      break;

      case TAB_CREATE_MATH:
      {
         tPlotCurveAxis curveInfo = getSelectedCurveInfo(ui->cmbSrcCurve_math);
         CurveData* curve = m_curveCmdr->getCurveData(curveInfo.plotName, curveInfo.curveName);

         setMathSampleRate(curve);
         setUserMathFromSrc(curveInfo, curve);
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

      case TAB_IP_BLOCK:
      {
         fillInIpBlockTab();
      }
      break;

      case TAB_PROPERTIES:
      {
         showApplyButton = true;
         fillInPropTab();
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
   displayUserMathOp();
}

void curveProperties::setUserMathFromSrc(tPlotCurveAxis &curveInfo, CurveData* curve)
{
   if(curve != NULL)
   {
      m_mathOps = curve->getMathOps(curveInfo.axis);
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

      if(needsValue_eMathOp(iter->op) == false)
      {
         displayStr = mathOpsSymbol[iter->op];
      }

      ui->opsOnCurve->addItem(displayStr);
   }

}

void curveProperties::on_availableOps_currentRowChanged(int currentRow)
{
   if(currentRow >= 0)
   {
      m_selectedMathOpLeft = currentRow;
      ui->lblMapOpValueLabel->setText(mathOpsValueLabel[currentRow]);
      ui->txtNumber->setVisible(needsValue_eMathOp((eMathOp)currentRow));
   }
}

void curveProperties::on_opsOnCurve_currentRowChanged(int currentRow)
{
   if(currentRow >= 0)
      m_selectedMathOpRight = currentRow;
}

void curveProperties::on_cmdOpRight_clicked()
{
   double number = 0.0;
   if(ui->txtNumber->text().toLower() == "pi")
   {
      number = CURVE_PROP_PI;
   }
   else if(ui->txtNumber->text().toLower() == "2pi")
   {
      number = CURVE_PROP_2PI;
   }
   else if(ui->txtNumber->text().toLower() == "e")
   {
      number = CURVE_PROP_E;
   }
   else
   {
      if(dString::strTo(ui->txtNumber->text().toStdString(), number) == false)
      {
         number = 0.0;
      }
   }

   if(number != 0.0 || needsValue_eMathOp((eMathOp)m_selectedMathOpLeft) == false)
   {
      tOperation newOp;

      newOp.op = (eMathOp)m_selectedMathOpLeft;
      newOp.num = number;

      if(m_selectedMathOpLeft == E_LOG)
      {
         // The number for log is actually the divisor to convert from ln to logx (x is the base).
         // Where logx(n) = ln(n) / ln(x)
         if(number != CURVE_PROP_E)
         {
            newOp.helperNum = log(number); // Remember log in c++ is actually natural log.
         }
         else
         {
            // Base is e, so no divide is needed.
            newOp.helperNum = 1.0;
         }
      }
      else
      {
         newOp.helperNum = 0.0;
      }

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
         ui->storedMsgs->addItem("[" + time + "] " + (*iter).name.plot + PLOT_CURVE_SEP + (*iter).name.curve);
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
   MainWindow* plotGui = m_curveCmdr->getMainPlot(toSave.plotName);

   if(toSaveCurveData != NULL)
   {
      // Use the last saved location to determine the folder to save the curve to.
      QString suggestedSavePath = getOpenSavePath(toSave.curveName);

      // Open the save file dialog.
      QString fileName = QFileDialog::getSaveFileName(this, tr("Save Curve To File"),
                                                       suggestedSavePath,
                                                       tr("Curves (*.curve);;Comma Separted Values (*.csv)"));

      setOpenSavePath(fileName);

      QString ext(fso::GetExt(dString::Lower(fileName.toStdString())).c_str());
      if(ext == "curve")
      {
         SaveCurve packedCurve(plotGui, toSaveCurveData, E_SAVE_RESTORE_RAW);
         fso::WriteFile(fileName.toStdString(), &packedCurve.packedCurveData[0], packedCurve.packedCurveData.size());
      }
      else if(ext == "csv")
      {
         SaveCurve packedCurve(plotGui, toSaveCurveData, E_SAVE_RESTORE_CSV);
         fso::WriteFile(fileName.toStdString(), &packedCurve.packedCurveData[0], packedCurve.packedCurveData.size());
      }

   }

}


void curveProperties::on_cmdSavePlotToFile_clicked()
{
   tCurveCommanderInfo allPlots = m_curveCmdr->getCurveCommanderInfo();
   QString plotName = ui->cmbPlotToSave->currentText();

   if(allPlots.find(plotName) != allPlots.end())
   {

      // Use the last saved location to determine the folder to save the curve to.
      QString suggestedSavePath = getOpenSavePath(plotName);

      // Open the save file dialog.
      QString fileName = QFileDialog::getSaveFileName(this, tr("Save Curve To File"),
                                                       suggestedSavePath,
                                                       tr("Plots (*.plot);;Comma Separted Values (*.csv)"));

      setOpenSavePath(fileName);

      // Fill in vector of curve data in the correct order.
      QVector<CurveData*> curves;
      curves.resize(allPlots[plotName].curves.size());
      foreach( QString key, allPlots[plotName].curves.keys() )
      {
         int index = allPlots[plotName].plotGui->getCurveIndex(key);
         curves[index] = allPlots[plotName].curves[key];
      }

      QString ext(fso::GetExt(dString::Lower(fileName.toStdString())).c_str());
      if(ext == "plot")
      {
         SavePlot savePlot(allPlots[plotName].plotGui, plotName, curves, E_SAVE_RESTORE_RAW);
         fso::WriteFile(fileName.toStdString(), &savePlot.packedCurveData[0], savePlot.packedCurveData.size());
      }
      else if(ext == "csv")
      {
         SavePlot savePlot(allPlots[plotName].plotGui, plotName, curves, E_SAVE_RESTORE_CSV);
         fso::WriteFile(fileName.toStdString(), &savePlot.packedCurveData[0], savePlot.packedCurveData.size());
      }

   }
}

void curveProperties::on_cmdOpenCurveFromFile_clicked()
{
   QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                   getOpenSaveDir(),
                                                   tr("Plot/Curve Files (*.plot *.curve);;CSV File (*.csv)"));

   setOpenSavePath(fileName);

   localPlotCreate::restorePlotFromFile(m_curveCmdr, fileName, ui->cmbOpenCurvePlotName->currentText());

}

bool curveProperties::validateNewPlotCurveName(QString& plotName, QString& curveName)
{
   return localPlotCreate::validateNewPlotCurveName(m_curveCmdr, plotName, curveName);
}

void curveProperties::on_cmbPropPlotCurveName_currentIndexChanged(int index)
{
   fillInPropTab();
}

void curveProperties::fillInPropTab()
{
   tPlotCurveAxis plotCurveInfo = getSelectedCurveInfo(ui->cmbPropPlotCurveName);
   CurveData* parentCurve = m_curveCmdr->getCurveData(plotCurveInfo.plotName, plotCurveInfo.curveName);
   MainWindow* parentPlot = m_curveCmdr->getMainPlot(plotCurveInfo.plotName);
   if(parentCurve != NULL && parentPlot != NULL)
   {
      ui->txtPropPlotName->setText(plotCurveInfo.plotName);
      ui->txtPropCurveName->setText(plotCurveInfo.curveName);
      ui->txtPropNumSamp->setText(QString::number(parentCurve->getNumPoints()));
      ui->txtPropDim->setText(parentCurve->getPlotDim() == E_PLOT_DIM_1D ? "1D" : "2D");
      ui->spnPropCurvePos->setMaximum(parentPlot->getNumCurves()-1);
      ui->spnPropCurvePos->setValue(parentPlot->getCurveIndex(plotCurveInfo.curveName));
      ui->chkPropHide->setChecked(parentCurve->getHidden());
      ui->chkPropVisable->setChecked(parentCurve->isDisplayed());

      maxMinXY curveDataMaxMin = parentCurve->getMaxMinXYOfData();
      ui->txtPropXMin->setText(QString::number(curveDataMaxMin.minX));
      ui->txtPropXMax->setText(QString::number(curveDataMaxMin.maxX));
      ui->txtPropYMin->setText(QString::number(curveDataMaxMin.minY));
      ui->txtPropYMax->setText(QString::number(curveDataMaxMin.maxY));

      ui->propParentCurves->clear();
      QVector<tPlotCurveAxis> parent = m_curveCmdr->getCurveParents(plotCurveInfo.plotName, plotCurveInfo.curveName);
      for(int i = 0; i < parent.size(); ++i)
      {
         ui->propParentCurves->addItem(parent[i].plotName + PLOT_CURVE_SEP + parent[i].curveName);
      }

      // Fill in Last Msg Ip Addr field.
      ui->txtLastIp->setText(tPlotterIpAddr::convert(parentCurve->lastMsgIpAddr.m_ipV4Addr));
   }
   else
   {
       ui->txtPropPlotName->setText("");
       ui->txtPropCurveName->setText("");
       ui->txtPropNumSamp->setText("");
       ui->txtPropDim->setText("");
       ui->spnPropCurvePos->setMaximum(0);
       ui->spnPropCurvePos->setValue(0);
       ui->chkPropHide->setChecked(false);
       ui->txtPropXMin->setText("");
       ui->txtPropXMax->setText("");
       ui->txtPropYMin->setText("");
       ui->txtPropYMax->setText("");
       ui->propParentCurves->clear();
       ui->txtLastIp->setText("");
   }
}

void curveProperties::propTabApply()
{
   tPlotCurveAxis plotCurveInfo = getSelectedCurveInfo(ui->cmbPropPlotCurveName);
   CurveData* parentCurve = m_curveCmdr->getCurveData(plotCurveInfo.plotName, plotCurveInfo.curveName);
   MainWindow* parentPlot = m_curveCmdr->getMainPlot(plotCurveInfo.plotName);
   if(parentCurve != NULL && parentPlot != NULL)
   {
      // If curve visability has changed, update.
      if(parentCurve->isDisplayed() != ui->chkPropVisable->isChecked())
      {
         parentPlot->toggleCurveVisability(plotCurveInfo.curveName);
      }

      // Set Curve Hidden Status.
      parentPlot->setCurveHidden(plotCurveInfo.curveName, ui->chkPropHide->isChecked());

      // Check for change of Curve Display Index
      if(parentPlot->getCurveIndex(plotCurveInfo.curveName) != ui->spnPropCurvePos->value())
      {
         parentPlot->setCurveIndex(plotCurveInfo.curveName, ui->spnPropCurvePos->value());
      }
   }
}

void curveProperties::on_cmdUnlinkParent_clicked()
{
   tPlotCurveAxis plotCurveInfo = getSelectedCurveInfo(ui->cmbPropPlotCurveName);
   m_curveCmdr->unlinkChildFromParents(plotCurveInfo.plotName, plotCurveInfo.curveName);
   fillInPropTab(); // Update Propties Tab with new information.
}

void curveProperties::on_cmdRemoveCurve_clicked()
{
   tPlotCurveAxis plotCurveInfo = getSelectedCurveInfo(ui->cmbPropPlotCurveName);
   // Ask the user if they are sure they want to remove the plot.
   QMessageBox::StandardButton reply;
   reply = QMessageBox::question( this,
                                  "Remove Plot",
                                  "Are you sure you want to permanently remove this curve: " + ui->cmbPropPlotCurveName->currentText(),
                                  QMessageBox::Yes|QMessageBox::No );
   if (reply == QMessageBox::Yes)
   {
      m_curveCmdr->removeCurve(plotCurveInfo.plotName, plotCurveInfo.curveName);
      return; // removing Curve, nothing more to do.
   }
}

void curveProperties::getSuggestedChildPlotCurveName(ePlotType plotType, QString& plotName, QString& curveName)
{
   plotName = "";
   curveName = "";

   tPlotCurveAxis xSrc = getSelectedCurveInfo(ui->cmbXAxisSrc);
   tPlotCurveAxis ySrc = getSelectedCurveInfo(ui->cmbYAxisSrc);

   // Check for no plots.
   if(ySrc.plotName == "")
   {
      // Properies window is open, but there are not plots. Early return.
      return;
   }

   bool twoDInput = plotTypeHas2DInput(plotType);
   bool plotNameMustBeUnique = false;

   twoDInput = plotTypeHas2DInput(plotType);

   if(plotType != E_PLOT_TYPE_AVERAGE && plotType != E_PLOT_TYPE_DELTA && plotType != E_PLOT_TYPE_SUM)
   {
      QString plotPrefix = plotTypeNames[plotType] + " of ";
      QString plotSuffix = "";
      QString plotMid = "";

      if(twoDInput)
      {
         if(xSrc.plotName == ySrc.plotName)
         {
            if(xSrc.curveName == ySrc.curveName)
            {
               // Both inputs are the same curve (presumably one is the X and one is the Y axis of a 2D plot)
               // Use the Curve Name.
               plotMid = ySrc.curveName;
            }
            else
            {
               // Use the Plot Name.
               plotMid = ySrc.plotName;
            }
         }
         else
         {
            // Use both Curve Names.
            plotMid = xSrc.curveName + " - " + ySrc.curveName;
         }
      }
      else
      {
         plotMid = xSrc.curveName;
      }

      // Generate the plot name.
      if(plotPrefix != "")
      {
         plotName = plotPrefix + plotMid;
      }
      else
      {
         plotName = plotMid;
      }
      if(plotSuffix != "")
      {
         plotName = plotName + plotSuffix;
      }

      curveName = plotMid;
      plotNameMustBeUnique = true;
   }
   else
   {
      // This defaults to remain on the same plot as the parent.
      plotName = xSrc.plotName;
      plotNameMustBeUnique = false; // Plot Name should be the same.
      curveName = plotTypeNames[plotType] + " of " + xSrc.curveName;
   }

   if(plotNameMustBeUnique && m_curveCmdr->validPlot(plotName))
   {
      int numToAddToEndOfPlotName = 0;
      QString plotNameValid = plotName + " ";
      do
      {
         plotName = plotNameValid + QString::number(numToAddToEndOfPlotName);
         ++numToAddToEndOfPlotName;
      }
      while(m_curveCmdr->validPlot(plotName));
   }

   if(m_curveCmdr->validCurve(plotName, curveName))
   {
      int numToAddToEndOfCurveName = 0;
      QString curveNameValid = curveName + " ";
      do
      {
         curveName = curveNameValid + QString::number(numToAddToEndOfCurveName);
         ++numToAddToEndOfCurveName;
      }
      while(m_curveCmdr->validCurve(plotName, curveName));
   }


}


void curveProperties::storeUserChildPlotNames(ePlotType plotType)
{
   if(plotType < 0)
      plotType = (ePlotType)ui->cmbPlotType->currentIndex();

   QString sugPlotName;
   QString sugCurveName;
   getSuggestedChildPlotCurveName(plotType, sugPlotName, sugCurveName);

   // Only save the plot name if it isn't already a plot name or it wasn't the suggested plot name.
   QString curPlotNameText = ui->cmbDestPlotName->currentText();
   m_childCurveNewPlotNameUser =
      (m_curveCmdr->validPlot(curPlotNameText) || curPlotNameText == sugPlotName) ?
      "" : curPlotNameText;

   // Only save the curve name if it wasn't the suggested curve name.
   QString curCurveNameText = ui->txtDestCurveName->text();
   m_childCurveNewCurveNameUser = curCurveNameText == sugCurveName ? "" : curCurveNameText;

}

void curveProperties::setUserChildPlotNames()
{
   QString sugPlotName;
   QString sugCurveName;
   getSuggestedChildPlotCurveName((ePlotType)ui->cmbPlotType->currentIndex(), sugPlotName, sugCurveName);

   if(m_childCurveNewPlotNameUser != "")
   {
      ui->cmbDestPlotName->lineEdit()->setText(m_childCurveNewPlotNameUser);
   }
   else
   {
      ui->cmbDestPlotName->lineEdit()->setText(sugPlotName);
   }
   if(m_childCurveNewCurveNameUser != "")
   {
      ui->txtDestCurveName->setText(m_childCurveNewCurveNameUser);
   }
   else
   {
      ui->txtDestCurveName->setText(sugCurveName);
   }
}


void curveProperties::on_cmbXAxisSrc_currentIndexChanged(int index)
{
   setUserChildPlotNames();
}

void curveProperties::on_cmbYAxisSrc_currentIndexChanged(int index)
{
   setUserChildPlotNames();
}

QString curveProperties::getOpenSaveDir()
{
   std::string retVal;
   if(persistentParam_getParam_str(PERSIST_PARAM_CURVE_SAVE_PREV_DIR_STR, retVal))
   {
      if(fso::DirExists(retVal))
      {
         return QString(retVal.c_str());
      }
   }
   return "";
}

QString curveProperties::getOpenSavePath(QString fileName)
{
   QString suggestedSavePath = getOpenSaveDir();
   if(suggestedSavePath != "")
   {
      suggestedSavePath = suggestedSavePath + QString(fso::dirSep().c_str()) + fileName;
   }
   else
   {
      suggestedSavePath = fileName;
   }
   return suggestedSavePath;
}

void curveProperties::setOpenSavePath(QString path)
{
   if(path != "")
   {
      persistentParam_setParam_str( PERSIST_PARAM_CURVE_SAVE_PREV_DIR_STR,
                                    fso::GetDir(path.toStdString()) );
   }
}

void curveProperties::fillInIpBlockTab()
{
   ipBlocker::tIpAddrs ipAddrs = m_ipBlocker->getIpAddrList();
   ui->cmbIpAddrs->clear();
   for(ipBlocker::tIpAddrs::iterator ipAddrsIter = ipAddrs.begin(); ipAddrsIter != ipAddrs.end(); ++ipAddrsIter)
   {
      ui->cmbIpAddrs->addItem(tPlotterIpAddr::convert(ipAddrsIter->m_ipV4Addr));
   }

   ipBlocker::tMapOfBlockedIps blockIps = m_ipBlocker->getBlockList();
   ui->ipBlockPlotNames->clear();
   foreach(tPlotterIpAddr ipAddr, blockIps.keys())
   {
      QString ipAddrStr = tPlotterIpAddr::convert(ipAddr.m_ipV4Addr);

      if(blockIps[ipAddr].size() > 0)
      {
         for(ipBlocker::tIpBlockListOfPlotNames::iterator iter = blockIps[ipAddr].begin(); iter != blockIps[ipAddr].end(); ++iter)
         {
            ui->ipBlockPlotNames->addItem(ipAddrStr + PLOT_CURVE_SEP + *iter);
         }
      }
      else
      {
         ui->ipBlockPlotNames->addItem(ipAddrStr + PLOT_CURVE_SEP + "*");
      }
   }

   ui->grpIpBlockPlotNames->setVisible(!ui->chkBlockAll->isChecked());

}


void curveProperties::on_cmdIpBlockAdd_clicked()
{
   if(ui->chkBlockAll->isChecked())
   {
      m_ipBlocker->addToBlockList(tPlotterIpAddr::convert(ui->cmbIpAddrs->currentText()));
   }
   else
   {
      m_ipBlocker->addToBlockList(tPlotterIpAddr::convert(ui->cmbIpAddrs->currentText()), ui->cmbIpBlockPlotNames->currentText());
   }
   fillInIpBlockTab();
}

void curveProperties::on_cmdIpBlockRemove_clicked()
{
   QList<QListWidgetItem*> selectedItems = ui->ipBlockPlotNames->selectedItems();
   for(QList<QListWidgetItem*>::iterator iter = selectedItems.begin(); iter != selectedItems.end(); ++iter)
   {
      QListWidgetItem* item = *iter;
      QString ipBlockPlotNameStr = item->text();
      QString ipAddr = dString::SplitLeft(ipBlockPlotNameStr.toStdString(), PLOT_CURVE_SEP.toStdString()).c_str();
      QString plotName = dString::SplitRight(ipBlockPlotNameStr.toStdString(), PLOT_CURVE_SEP.toStdString()).c_str();

      if(plotName == "*")
      {
         m_ipBlocker->removeFromBlockList(tPlotterIpAddr::convert(ipAddr));
      }
      else
      {
         m_ipBlocker->removeFromBlockList(tPlotterIpAddr::convert(ipAddr), plotName);
      }
   }
   fillInIpBlockTab();
}

void curveProperties::on_chkBlockAll_clicked()
{
   fillInIpBlockTab();
}

void curveProperties::on_cmdIpBlockRemoveAll_clicked()
{
    m_ipBlocker->clearBlockList();
    fillInIpBlockTab();
}
