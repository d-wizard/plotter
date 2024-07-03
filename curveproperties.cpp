/* Copyright 2014 - 2024 Dan Williams. All Rights Reserved.
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
#include <QColorDialog>
#include <fstream>
#include <iomanip>
#include <assert.h>
#include "overwriterenamedialog.h"
#include "saveRestoreCurve.h"
#include "FileSystemOperations.h"
#include "persistentParameters.h"
#include "localPlotCreate.h"

const int TAB_CREATE_CHILD_CURVE = 0;
const int TAB_CREATE_MATH = 1;
const int TAB_RESTORE_MSG = 2;
const int TAB_OPEN_SAVE_CURVE = 3;
const int TAB_IP_BLOCK = 4;
//const int TAB_HOTKEYS = 5; (not actually used, so comment out)
const int TAB_PROPERTIES = 6;

const double CURVE_PROP_PI = 3.1415926535897932384626433832795028841971693993751058;
const double CURVE_PROP_2PI = 6.2831853071795864769252867665590057683943387987502116;
const double CURVE_PROP_E = 2.7182818284590452353602874713526624977572470936999595;

const QString OPEN_SAVE_FILTER_PLOT_STR = "Plots (*.plot)";
const QString OPEN_SAVE_FILTER_CURVE_STR = "Curves (*.curve)";
const QString OPEN_SAVE_FILTER_CSV_STR = "Comma Separted Values (*.csv)";
const QString OPEN_SAVE_FILTER_C_HEADER_AUTO_TYPE_STR = "C Header - Auto Type (*.h)";
const QString OPEN_SAVE_FILTER_C_HEADER_INT_STR = "C Header - Integer (*.h)";
const QString OPEN_SAVE_FILTER_C_HEADER_FLOAT_STR = "C Header - Float (*.h)";

const QString OPEN_SAVE_FILTER_DELIM = ";;";


const QString mathOpsStr[] = {
"ADD",
"MUTIPLY",
"DIVIDE",
"SHIFT UP",
"SHIFT DOWN",
"POWER",
"LOG",
"ALOG",
"MOD",
"ABS",
"ROUND",
"ROUND UP",
"ROUND DOWN",
"LIMIT UPPER",
"LIMIT LOWER",
"SIN",
"COS",
"TAN",
"ASIN",
"ACOS",
"ATAN"
};

const QString mathOpsSymbol[] = {
"+",
"*",
"/",
"<<",
">>",
"^",
"log",
"alog",
"mod",
"abs",
"round",
"round up",
"round down",
"limit upper",
"limit lower",
"sin",
"cos",
"tan",
"asin",
"acos",
"atan"
};


const QString mathOpsValueLabel[] = {
"Value to Add",
"Value to Multiply by",
"Value to Divide by",
"Bits to Shift Up by",
"Bits to Shift Down by",
"Raise to Power Value",
"Base of log",
"Base of inverse log",
"Modulus Denominator",
"", // ABS has no value, so set the text to nothing.
"Decimal Point to Round",
"Decimal Point to Round Up",
"Decimal Point to Round Down",
"Upper Hard Limit Vaue",
"Lower Hard Limit Vaue",
"", // Trig functiona have no values.
"", // Trig functiona have no values.
"", // Trig functiona have no values.
"", // Trig functiona have no values.
"", // Trig functiona have no values.
""  // Trig functiona have no values.
};

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
   "Math",
   "FFT Measurement",
   "Curve Stats"
};

const QString fftMeasureNames[] = {
   "Signal Power",
   "Signal BW",
   "Noise Power",
   "Noise BW",
   "Noise Per Hz",
   "SNR",
   "SNR dB Hz",
   "No Measurement"
};

const QString childStatsNames[] = {
   "Num Samples",
   "X Min",
   "X Max",
   "Y Min",
   "Y Max",
   "Sample Rate"
};

// This is persistent only while the executable is running.
static QMap<ePlotType, bool> gSuggestChildOnParentPlot;

curveProperties::curveProperties(CurveCommander *curveCmdr, QString plotName, QString curveName, QWidget *parent) :
   QWidget(parent),
   m_ipBlocker(curveCmdr->getIpBlocker()),
   ui(new Ui::curveProperties),
   m_curveCmdr(curveCmdr),
   m_selectedMathOpLeft(0),
   m_selectedMathOpRight(0),
   m_numMathOpsReadFromSrc(0),
   m_guiIsChanging(false),
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

   // Create Combo Box Pointers (tPltCrvCmbBoxPtr).
   tPltCrvCmbBoxPtr cmbXAxisSrc          = tPltCrvCmbBoxPtr(new tPlotCurveComboBox(ui->cmbXAxisSrc         ));
   tPltCrvCmbBoxPtr cmbYAxisSrc          = tPltCrvCmbBoxPtr(new tPlotCurveComboBox(ui->cmbYAxisSrc         ));
   tPltCrvCmbBoxPtr cmbSrcCurve_math     = tPltCrvCmbBoxPtr(new tPlotCurveComboBox(ui->cmbSrcCurve_math    ));
   tPltCrvCmbBoxPtr cmbCurveToSave       = tPltCrvCmbBoxPtr(new tPlotCurveComboBox(ui->cmbCurveToSave      ));
   tPltCrvCmbBoxPtr cmbPropPlotCurveName = tPltCrvCmbBoxPtr(new tPlotCurveComboBox(ui->cmbPropPlotCurveName));
   tPltCrvCmbBoxPtr cmbDestPlotName      = tPltCrvCmbBoxPtr(new tPlotCurveComboBox(ui->cmbDestPlotName     ));
   tPltCrvCmbBoxPtr cmbPlotToSave        = tPltCrvCmbBoxPtr(new tPlotCurveComboBox(ui->cmbPlotToSave       ));
   tPltCrvCmbBoxPtr cmbIpBlockPlotNames  = tPltCrvCmbBoxPtr(new tPlotCurveComboBox(ui->cmbIpBlockPlotNames ));

   // Set tCmbBoxAndValue
   m_cmbXAxisSrc          = tCmbBoxValPtr(new tCmbBoxAndValue(cmbXAxisSrc, tCmbBoxAndValue::E_PREFERRED_AXIS_X));
   m_cmbYAxisSrc          = tCmbBoxValPtr(new tCmbBoxAndValue(cmbYAxisSrc, tCmbBoxAndValue::E_PREFERRED_AXIS_Y));
   m_cmbSrcCurve_math     = tCmbBoxValPtr(new tCmbBoxAndValue(cmbSrcCurve_math, true, true));
   m_cmbCurveToSave       = tCmbBoxValPtr(new tCmbBoxAndValue(cmbCurveToSave));
   m_cmbPropPlotCurveName = tCmbBoxValPtr(new tCmbBoxAndValue(cmbPropPlotCurveName, false));

   m_cmbDestPlotName      = tCmbBoxValPtr(new tCmbBoxAndValue(cmbDestPlotName));
   m_cmbPlotToSave        = tCmbBoxValPtr(new tCmbBoxAndValue(cmbPlotToSave));
   m_cmbIpBlockPlotNames  = tCmbBoxValPtr(new tCmbBoxAndValue(cmbIpBlockPlotNames));


   // Initialize the list of all the combo boxes that display PlotName->CurveName
   m_plotCurveCombos.clear();
   m_plotCurveCombos.append(m_cmbXAxisSrc);
   m_plotCurveCombos.append(m_cmbYAxisSrc);
   m_plotCurveCombos.append(m_cmbSrcCurve_math);
   m_plotCurveCombos.append(m_cmbCurveToSave);
   m_plotCurveCombos.append(m_cmbPropPlotCurveName);

   // Initialize the list of all the combo boxes that display all the plot names
   m_plotNameCombos.clear();
   m_plotNameCombos.append(m_cmbDestPlotName);
   m_plotNameCombos.append(m_cmbPlotToSave);
   m_plotNameCombos.append(m_cmbIpBlockPlotNames);

   // Set current tab index.
   ui->tabWidget->setCurrentIndex(TAB_CREATE_CHILD_CURVE);
   on_cmbPlotType_currentIndexChanged(ui->cmbPlotType->currentIndex());

   // Initialize GUI elements.
   updateGuiPlotCurveInfo(plotName, curveName);

   // Set Suggested Plot / Curve Names.
   setUserChildPlotNames();

   // Attempt to restore the Child Curve combo box values from persistent memory.
   initCmbBoxValueFromPersistParam(ui->cmbPlotType, PERSIST_PARAM_CHILD_CURVE_PLOT_TYPE);
   initCmbBoxValueFromPersistParam(ui->cmbChildMathOperators, PERSIST_PARAM_CHILD_CURVE_MATH_TYPE);
   initCmbBoxValueFromPersistParam(ui->cmbChildStatsTypes, PERSIST_PARAM_CHILD_CURVE_STAT_TYPE);
   initSpnBoxValueFromPersistParam(ui->spnFFtMeasChildPlotSize, PERSIST_PARAM_FFT_MEAS_CHILD_SIZE);

   // Set "Match Parent Scroll" Check Box.
   double chkBoxVal = 0.0;
   if(persistentParam_getParam_f64(PERSIST_PARAM_MATCH_PARENT_SCROLL, chkBoxVal))
      ui->chkMatchParentScroll->setChecked(chkBoxVal != 0.0); // Restore value from Persistent Parameters.
   setMatchParentScrollChkBoxVisible();

   // Set some GUI elements that were modified to make the GUI editing easier.
   int sliceMinMax = 99999999;
   ui->spnXSrcStart->setMinimum(-sliceMinMax);
   ui->spnXSrcStart->setMaximum(sliceMinMax);
   ui->spnYSrcStart->setMinimum(-sliceMinMax);
   ui->spnYSrcStart->setMaximum(sliceMinMax);
   ui->spnXSrcStop->setMinimum(-sliceMinMax);
   ui->spnXSrcStop->setMaximum(sliceMinMax);
   ui->spnYSrcStop->setMinimum(-sliceMinMax);
   ui->spnYSrcStop->setMaximum(sliceMinMax);
}

curveProperties::~curveProperties()
{
   delete ui;
}

void curveProperties::updateGuiPlotCurveInfo(QString plotName, QString curveName)
{
   bool userSpecifiedPlotCurveName = plotName != "" && curveName != "";

   QString firstPlotName = "";  //  Plot Name of the first plot/curve in the combo box.
   QString firstCurveName = ""; // Curve Name of the first plot/curve in the combo box.
   bool firstPlotCurveNameCmbBoxFound = false;

   tCurveCommanderInfo allCurves = m_curveCmdr->getCurveCommanderInfo();

   m_guiIsChanging = true; // Set to true to keep GUI change functions (e.g. currentIndexChanged) from marking Combo Boxes as 'User Specified'

   // Save current values of the GUI elements. Clear the combo box members.
   for(int i = 0; i < m_plotCurveCombos.size(); ++i)
   {
      m_plotCurveCombos[i]->cmbBoxVal = m_plotCurveCombos[i]->cmbBoxPtr->currentText();
      m_plotCurveCombos[i]->cmbBoxPtr->clear();
   }

   // Save current values of the GUI elements. Clear Plot Name combo boxes.
   for(int i = 0; i < m_plotNameCombos.size(); ++i)
   {
      m_plotNameCombos[i]->cmbBoxVal = m_plotNameCombos[i]->cmbBoxPtr->currentText();
      m_plotNameCombos[i]->cmbBoxPtr->clear();
   }

   // Save and clear FFT Measurement Parent Combo
   QString fftMeasureComboVal = ui->cmbXAxisSrc_fftMeasurement->currentText();
   ui->cmbXAxisSrc_fftMeasurement->clear();

   QList<QString> curveNames; // All the curve names in the default plot (either the plot specified by plotName or firstPlotName)

   foreach( QString curPlotName, allCurves.keys() )
   {
      // Add to dest plot name combo box
      for(int i = 0; i < m_plotNameCombos.size(); ++i)
      {
         m_plotNameCombos[i]->cmbBoxPtr->addItem(curPlotName, tPlotCurveComboBox::E_COMBOBOX_PLOT_NAME_ONLY);
      }

      // Add the "All Curves" curve name to the drop down box.
      for(int i = 0; i < m_plotCurveCombos.size(); ++i)
      {
         if(m_plotCurveCombos[i]->displayAllCurves)
         {
            m_plotCurveCombos[i]->cmbBoxPtr->addItem(curPlotName, tPlotCurveComboBox::E_COMBOBOX_CURVE_ALL_CURVES);
         }
      }

      // Add the Parent FFT Measurement Sources
      if(allCurves[curPlotName].plotGui->areFftMeasurementsVisible())
      {
         ui->cmbXAxisSrc_fftMeasurement->addItem(curPlotName + PLOT_CURVE_SEP + fftMeasureNames[E_FFT_MEASURE__SIG_POWER]);
         ui->cmbXAxisSrc_fftMeasurement->addItem(curPlotName + PLOT_CURVE_SEP + fftMeasureNames[E_FFT_MEASURE__NOISE_POWER]);
         ui->cmbXAxisSrc_fftMeasurement->addItem(curPlotName + PLOT_CURVE_SEP + fftMeasureNames[E_FFT_MEASURE__NOISE_PER_HZ]);
         ui->cmbXAxisSrc_fftMeasurement->addItem(curPlotName + PLOT_CURVE_SEP + fftMeasureNames[E_FFT_MEASURE__SNR]);
         ui->cmbXAxisSrc_fftMeasurement->addItem(curPlotName + PLOT_CURVE_SEP + fftMeasureNames[E_FFT_MEASURE__SNR_PER_HZ]);
      }

      tCurveDataInfo* curves = &(allCurves[curPlotName].curves);
      foreach( QString curveName, curves->keys() )
      {
         CurveData* curveData = (*curves)[curveName];

         // Store the first plot/curve name in the combo box.
         if(firstPlotCurveNameCmbBoxFound == false)
         {
            firstPlotName = curPlotName;
            firstCurveName = curveName;
            firstPlotCurveNameCmbBoxFound = firstPlotName != "" && firstCurveName != "";
         }

         if(curPlotName == plotName && curveData->isDisplayed())
         {
            curveNames.push_back(curveName); // Fill in list of all curve names for the given plot.
         }
         else if(userSpecifiedPlotCurveName == false && firstPlotCurveNameCmbBoxFound == true && curPlotName == firstPlotName)
         {
            // plotName was not explicitly specified. Store off all curve names in the first plot in the combo box.
            curveNames.push_back(curveName);
         }

         if( curveData->getPlotDim() == E_PLOT_DIM_1D)
         {
            for(int i = 0; i < m_plotCurveCombos.size(); ++i)
            {
               m_plotCurveCombos[i]->cmbBoxPtr->addItem(curPlotName, curveName, tPlotCurveComboBox::E_COMBOBOX_CURVE_ENTIRE_CURVE);
            }
         }
         else
         {
            for(int i = 0; i < m_plotCurveCombos.size(); ++i)
            {
               // Add the "All Axes" plot/curve name to the drop down box.
               if(m_plotCurveCombos[i]->displayAllAxes)
               {
                  m_plotCurveCombos[i]->cmbBoxPtr->addItem(curPlotName, curveName, tPlotCurveComboBox::E_COMBOBOX_CURVE_AXIS_ALL);
               }

               if(m_plotCurveCombos[i]->displayAxesSeparately)
               {
                  m_plotCurveCombos[i]->cmbBoxPtr->addItem(curPlotName, curveName, tPlotCurveComboBox::E_COMBOBOX_CURVE_AXIS_X);
                  m_plotCurveCombos[i]->cmbBoxPtr->addItem(curPlotName, curveName, tPlotCurveComboBox::E_COMBOBOX_CURVE_AXIS_Y);
               }
               else
               {
                  m_plotCurveCombos[i]->cmbBoxPtr->addItem(curPlotName, curveName, tPlotCurveComboBox::E_COMBOBOX_CURVE_ENTIRE_CURVE);
               }
            }
         }
      }
   }

   // Set the Plot/Curve combo boxes.
   QString defaultRealCurveName;
   QString defaultImagCurveName;
   if(userSpecifiedPlotCurveName)
   {
      // User brought this GUI up from a plot window. Set the selected value of all the plot/curve
      // combo boxes to the plot/curve name of the selected curve in the plot window.
      findRealImagCurveNames(curveNames, curveName, defaultRealCurveName, defaultImagCurveName);
      setCombosToPlotCurve(plotName, curveName, defaultRealCurveName, defaultImagCurveName, false);
   }
   else
   {
      // This function is being called because some curve has changed (created/deleted/updated).
      // Attempt to set the selected value of the plot/curve combos back to the value
      // it was before the change.
      if(firstPlotCurveNameCmbBoxFound)
      {
         // We are setting the combo boxes for plot/curve's to (more or less) default values. First appempt to
         // set the plot/curve name back to the values they were previously set to. If that fails attempt to
         // set the real/imag specific combo boxes to match the curve names.
         findRealImagCurveNames(curveNames, firstCurveName, defaultRealCurveName, defaultImagCurveName);
         setCombosToPlotCurve(firstPlotName, firstCurveName, defaultRealCurveName, defaultImagCurveName, true);
      }
      else
      {
         // I don't think we should ever get here... but just incase...
         setCombosToPrevValues();
      }
   }

   // Set FFT Measurement Combo Box back
   for(int i = 0; i < ui->cmbXAxisSrc_fftMeasurement->count(); ++i)
   {
      if(ui->cmbXAxisSrc_fftMeasurement->itemText(i) == fftMeasureComboVal)
      {
         ui->cmbXAxisSrc_fftMeasurement->setCurrentIndex(i);
         break;
      }
   }

   if(ui->tabWidget->currentIndex() == TAB_PROPERTIES)
   {
      fillInPropTab();
   }

   setUserChildPlotNames();
   setMatchParentScrollChkBoxVisible();

   if(userSpecifiedPlotCurveName)
   {
      // Store off the IP Address of the last message from the plot / curve that was selected when this function was called.
      m_lastUpdatedPlotIpAddr = m_curveCmdr->getCurveData(plotName, curveName)->getLastMsgIpAddr().m_ipV4Addr;
   }

   // Done Modifying the GUI.
   m_guiIsChanging = false;

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
   setMatchParentScrollChkBoxVisible();
}

void curveProperties::setCombosToPrevValues()
{
   // Set Plot/Curve Name Combos to previous values
   for(int i = 0; i < m_plotCurveCombos.size(); ++i)
   {
      trySetComboItemIndex(m_plotCurveCombos[i]->cmbBoxPtr, m_plotCurveCombos[i]->cmbBoxVal);
   }

   // Set Plot Name Combos to previous values
   for(int i = 0; i < m_plotNameCombos.size(); ++i)
   {
      trySetComboItemIndex(m_plotNameCombos[i]->cmbBoxPtr, m_plotNameCombos[i]->cmbBoxVal);
   }
}

void curveProperties::setCombosToPlotCurve(const QString& plotName, const QString& curveName, const QString& realCurveName, const QString& imagCurveName, bool restoreUserSpecifed)
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

      // Only "Try to Restore" if there is something to restore from.
      if(restoreUserSpecifed && m_plotCurveCombos[i]->userSpecified())
      {
         updateComboSuccess = trySetComboItemIndex(m_plotCurveCombos[i]->cmbBoxPtr, m_plotCurveCombos[i]->cmbBoxVal);
         m_plotCurveCombos[i]->userSpecified(updateComboSuccess); // If trySetComboItemIndex failed, then user specified value is no longer valid.
      }

      if(updateComboSuccess == false && m_plotCurveCombos[i]->preferredAxis != tCmbBoxAndValue::E_PREFERRED_AXIS_DONT_CARE)
      {
         if( m_plotCurveCombos[i]->preferredAxis == tCmbBoxAndValue::E_PREFERRED_AXIS_X &&
             trySetComboItemIndex(m_plotCurveCombos[i]->cmbBoxPtr, plotCurveName1D_real) )
         {
            updateComboSuccess = true;
         }
         else if( m_plotCurveCombos[i]->preferredAxis == tCmbBoxAndValue::E_PREFERRED_AXIS_Y &&
                  trySetComboItemIndex(m_plotCurveCombos[i]->cmbBoxPtr, plotCurveName1D_imag) )
         {
            updateComboSuccess = true;
         }
      }

      // Try to set to 1D Curve Name.
      if(updateComboSuccess == false)
      {
         updateComboSuccess = trySetComboItemIndex(m_plotCurveCombos[i]->cmbBoxPtr, plotCurveName1D);
         if(updateComboSuccess == false)
         {
            // 1D Curve Name doesn't exists, must be 2D. Set to 2D Curve Name.
            if(m_plotCurveCombos[i]->preferredAxis == tCmbBoxAndValue::E_PREFERRED_AXIS_Y)
               updateComboSuccess = trySetComboItemIndex(m_plotCurveCombos[i]->cmbBoxPtr, plotCurveName2Dy);
            else
               updateComboSuccess = trySetComboItemIndex(m_plotCurveCombos[i]->cmbBoxPtr, plotCurveName2Dx);
         }
      }

      // If updating with user specifed values (i.e. restoreUserSpecifed is false), mark this combo box as user specified.
      if(!restoreUserSpecifed && updateComboSuccess)
      {
         m_plotCurveCombos[i]->userSpecified(true);
      }
   }

   // Set Plot Name Combos
   for(int i = 0; i < m_plotNameCombos.size(); ++i)
   {
      bool updateComboSuccess = false;

      // Only "Try to Restore" if there is something to restore from.
      if(restoreUserSpecifed && m_plotNameCombos[i]->userSpecified())
      {
         updateComboSuccess = trySetComboItemIndex(m_plotNameCombos[i]->cmbBoxPtr, m_plotNameCombos[i]->cmbBoxVal);
         m_plotNameCombos[i]->userSpecified(updateComboSuccess); // If trySetComboItemIndex failed, then user specified value is no longer valid.
      }

      if(updateComboSuccess == false)
      {
         updateComboSuccess = trySetComboItemIndex(m_plotNameCombos[i]->cmbBoxPtr, plotName);
      }

      // If updating with user specifed values (i.e. restoreUserSpecifed is false), mark this combo box as user specified.
      if(!restoreUserSpecifed && updateComboSuccess)
      {
         m_plotNameCombos[i]->userSpecified(true);
      }
   }

}

bool curveProperties::trySetComboItemIndex(tPltCrvCmbBoxPtr cmbBox, QString text)
{
   return cmbBox->trySetComboItemIndex(text);
}

void curveProperties::on_cmbPlotType_currentIndexChanged(int index)
{
   bool xVis = true;
   bool yVis = plotTypeHas2DInput((ePlotType)index);
   bool fftCheckBoxVisible = false;
   bool slice = ui->chkSrcSlice->checkState() == Qt::Checked;
   bool sliceVis = true;
   bool mathCmbVis = false;
   bool childStatsCmbVis = false;
   bool fftMeasureVis = false;

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
      case E_PLOT_TYPE_CURVE_STATS:
         ui->lblXAxisSrc->setText("Source");
         childStatsCmbVis = true;
         sliceVis = false;
      break;
      case E_PLOT_TYPE_2D:
         ui->lblXAxisSrc->setText("X Axis Source");
         ui->lblYAxisSrc->setText("Y Axis Source");
      break;
      case E_PLOT_TYPE_REAL_FFT:
      case E_PLOT_TYPE_DB_POWER_FFT_REAL:
         ui->lblXAxisSrc->setText("Real Source");
         fftCheckBoxVisible = true;
      break;
      case E_PLOT_TYPE_COMPLEX_FFT:
      case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
         fftCheckBoxVisible = true;
         // fall through
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
      case E_PLOT_TYPE_FFT_MEASUREMENT:
         ui->lblXAxisSrc->setText("Measurement");
         xVis = false;
         sliceVis = false;
         fftMeasureVis = true;
      break;
   }

   m_cmbXAxisSrc->setVisible(xVis);
   m_cmbYAxisSrc->setVisible(yVis);
   ui->cmbXAxisSrc_fftMeasurement->setVisible(fftMeasureVis);

   ui->lblXAxisSrc->setVisible(xVis || fftMeasureVis);
   ui->lblYAxisSrc->setVisible(yVis);

   ui->grpSlicingOptions->setVisible(sliceVis);

   ui->spnXSrcStart->setVisible(xVis && slice && sliceVis);
   ui->spnXSrcStop->setVisible(xVis && slice && sliceVis);
   ui->cmdXUseZoomForSlice->setVisible(xVis && slice && sliceVis);

   ui->spnYSrcStart->setVisible(yVis && slice && sliceVis);
   ui->spnYSrcStop->setVisible(yVis && slice && sliceVis);
   ui->cmdYUseZoomForSlice->setVisible(yVis && slice && sliceVis);

   ui->grpAvgOptions->setVisible(index == E_PLOT_TYPE_AVERAGE);

   ui->grpFftOptions->setVisible(fftCheckBoxVisible);

   ui->cmbChildMathOperators->setVisible(mathCmbVis);

   ui->cmbChildStatsTypes->setVisible(childStatsCmbVis);

   ui->lblFftMeasChildPlotSize->setVisible(fftMeasureVis || childStatsCmbVis);
   ui->spnFFtMeasChildPlotSize->setVisible(fftMeasureVis || childStatsCmbVis);

   ui->lineChildSrcVirtSep->setVisible(fftMeasureVis || childStatsCmbVis || (slice && sliceVis));

   setUserChildPlotNames();
   setMatchParentScrollChkBoxVisible();
}

bool curveProperties::validateSlice(tParentCurveInfo& sliceInfo)
{
   CurveData* parentCurve = m_curveCmdr->getCurveData(sliceInfo.dataSrc.plotName, sliceInfo.dataSrc.curveName);
   int numParentPoints = (int)parentCurve->getNumPoints();

   // Start Index is inclusive. Stop Index is exclusive.
   int start = sliceInfo.startIndex;
   if(start < 0)
      start += numParentPoints;
   int stop = sliceInfo.stopIndex;
   if(stop <= 0)
      stop += numParentPoints;

   return (start >= 0 && start < numParentPoints) && (stop > 0 && stop <= numParentPoints) && (stop > start);
}

void curveProperties::childCurveTabApply()
{
   QString newChildPlotName = m_cmbDestPlotName->currentText();
   QString newChildCurveName = ui->txtDestCurveName->text();

   bool validChildPlotCurveNames = (newChildPlotName != "" && newChildCurveName != "");
   bool curveExists = validChildPlotCurveNames && m_curveCmdr->validCurve(newChildPlotName, newChildCurveName);
   bool createTheChildPlot = validChildPlotCurveNames && !curveExists;
   bool invalidSrcSlice = false; // Assume false for now. Could be changed later.

   if(createTheChildPlot)
   {
      bool forceContiguousParentPoints = ui->chkFftSrcContiguous->isVisible() && ui->chkFftSrcContiguous->isChecked();
      bool matchParentScrollMode = ui->chkMatchParentScroll->isVisible() && ui->chkMatchParentScroll->isChecked();

      // New Child plot->curve does not exist, continue creating the child curve.
      ePlotType plotType = (ePlotType)ui->cmbPlotType->currentIndex();
      if( plotTypeHas2DInput(plotType) == false )
      {
         tParentCurveInfo axisParent;
         axisParent.dataSrc = m_cmbXAxisSrc->getPlotCurveAxis();
         if(ui->chkSrcSlice->checkState() == Qt::Checked)
         {
            axisParent.startIndex = ui->spnXSrcStart->value();
            axisParent.stopIndex = ui->spnXSrcStop->value();

            invalidSrcSlice = !validateSlice(axisParent);
            createTheChildPlot = createTheChildPlot && !invalidSrcSlice; // Update createTheChildPlot with source slice validity
         }
         else
         {
            axisParent.startIndex = 0;
            axisParent.stopIndex = 0;
         }

         axisParent.windowFFT = ui->chkWindow->isChecked();
         axisParent.scaleFftWindow = ui->chkScaleFftWindow->isChecked();
         axisParent.avgAmount = atof(ui->txtAvgAmount->text().toStdString().c_str());

         // Determine FFT Measurement type (only valid for E_PLOT_TYPE_FFT_MEASUREMENT plot types).
         axisParent.fftMeasurementType = E_FFT_MEASURE__NO_FFT_MEASUREMENT;
         axisParent.curveStatType = E_CURVE_STATS__NO_CURVE_STAT;
         axisParent.curveStatstPlotSize = -1;
         if(plotType == E_PLOT_TYPE_FFT_MEASUREMENT)
         {
            // FFT Measurement Child Plots are done a little differently. Use the return value to ensure we should actually create the Child Plot.
            createTheChildPlot = createTheChildPlot && determineChildFftMeasurementAxisValues(axisParent);

            // Store off the value of the FFT Measurement Child Curve.
            if(createTheChildPlot)
            {
               persistentParam_setParam_f64(PERSIST_PARAM_FFT_MEAS_CHILD_SIZE, ui->spnFFtMeasChildPlotSize->value());
            }
         }
         else if(plotType == E_PLOT_TYPE_CURVE_STATS)
         {
            axisParent.curveStatType = (eCurveStats)(ui->cmbChildStatsTypes->currentIndex());
            axisParent.curveStatstPlotSize = ui->spnFFtMeasChildPlotSize->value();
         }

         if(createTheChildPlot)
         {
            m_cmbXAxisSrc->userSpecified(true); // User hit the Apply button, i.e. user specified.
            m_curveCmdr->createChildCurve( newChildPlotName,
                                           newChildCurveName,
                                           plotType,
                                           forceContiguousParentPoints,
                                           matchParentScrollMode,
                                           axisParent);
            setPersistentSuggestChildOnParentPlot(plotType, newChildPlotName == axisParent.dataSrc.plotName);
         }
      }
      else
      {
         tParentCurveInfo xAxisParent;
         xAxisParent.dataSrc = m_cmbXAxisSrc->getPlotCurveAxis();

         tParentCurveInfo yAxisParent;
         yAxisParent.dataSrc = m_cmbYAxisSrc->getPlotCurveAxis();


         if(ui->chkSrcSlice->checkState() == Qt::Checked)
         {
            xAxisParent.startIndex = ui->spnXSrcStart->value();
            xAxisParent.stopIndex = ui->spnXSrcStop->value();
            yAxisParent.startIndex = ui->spnYSrcStart->value();
            yAxisParent.stopIndex = ui->spnYSrcStop->value();

            invalidSrcSlice = !validateSlice(xAxisParent) || !validateSlice(yAxisParent);
            createTheChildPlot = createTheChildPlot && !invalidSrcSlice; // Update createTheChildPlot with source slice validity
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
         xAxisParent.scaleFftWindow = ui->chkScaleFftWindow->isChecked();
         xAxisParent.mathBetweenCurvesOperator =
            (eMathBetweenCurves_operators)ui->cmbChildMathOperators->currentIndex();
         xAxisParent.fftMeasurementType = E_FFT_MEASURE__NO_FFT_MEASUREMENT;
         xAxisParent.curveStatstPlotSize = -1;

         // Y Axis values need to match X Axis value.
         yAxisParent.windowFFT = xAxisParent.windowFFT;
         yAxisParent.scaleFftWindow = xAxisParent.scaleFftWindow;
         yAxisParent.mathBetweenCurvesOperator = xAxisParent.mathBetweenCurvesOperator;
         yAxisParent.fftMeasurementType = E_FFT_MEASURE__NO_FFT_MEASUREMENT;
         yAxisParent.curveStatstPlotSize = -1;

         if(createTheChildPlot)
         {
            m_cmbXAxisSrc->userSpecified(true); // User hit the Apply button, i.e. user specified.
            m_cmbYAxisSrc->userSpecified(true); // User hit the Apply button, i.e. user specified.
            m_curveCmdr->createChildCurve( newChildPlotName,
                                           newChildCurveName,
                                           plotType,
                                           forceContiguousParentPoints,
                                           matchParentScrollMode,
                                           xAxisParent,
                                           yAxisParent);
            setPersistentSuggestChildOnParentPlot(plotType, newChildPlotName == xAxisParent.dataSrc.plotName || newChildPlotName == yAxisParent.dataSrc.plotName);
         }
      }
   } // End if(createTheChildPlot)

   // After all that was done, see if a child curve was actually created.
   if(!createTheChildPlot)
   {
      // Create pop ups for certian errors.
      if(curveExists)
      {
         QMessageBox msgBox;
         msgBox.setWindowTitle("Curve Already Exists");
         msgBox.setText(newChildPlotName + PLOT_CURVE_SEP + newChildCurveName + " already exists. Choose another plot->curve name.");
         msgBox.setStandardButtons(QMessageBox::Ok);
         msgBox.setDefaultButton(QMessageBox::Ok);
         msgBox.exec();
      }
      else if(invalidSrcSlice)
      {
         QMessageBox msgBox;
         msgBox.setWindowTitle("Source Slice Values Are Invalid");
         msgBox.setText("Please update the slice values and try again.");
         msgBox.setStandardButtons(QMessageBox::Ok);
         msgBox.setDefaultButton(QMessageBox::Ok);
         msgBox.exec();
      }
   }
   else
   {
      // Child curve has been created. Clear user input of plot / curve name.
      m_childCurveNewPlotNameUser = "";
      m_childCurveNewCurveNameUser = "";

      // Save the plot type of the child curve that is being created in persistent memory. This way the next time the Properties window is
      // opened it will initialize to the same child plot type.
      persistentParam_setParam_f64(PERSIST_PARAM_CHILD_CURVE_PLOT_TYPE, ui->cmbPlotType->currentIndex());
      if(ui->cmbPlotType->currentIndex() == E_PLOT_TYPE_MATH_BETWEEN_CURVES)
      {
         // Save the math type used to persistent memory.
         persistentParam_setParam_f64(PERSIST_PARAM_CHILD_CURVE_MATH_TYPE, ui->cmbChildMathOperators->currentIndex());
      }
      else if(ui->cmbPlotType->currentIndex() == E_PLOT_TYPE_CURVE_STATS)
      {
         // Save the stats type used to persistent memory.
         persistentParam_setParam_f64(PERSIST_PARAM_CHILD_CURVE_STAT_TYPE, ui->cmbChildStatsTypes->currentIndex());
      }
   }
}

void curveProperties::on_cmdApply_clicked()
{
   int tab = ui->tabWidget->currentIndex();
   if(tab == TAB_CREATE_CHILD_CURVE)
   {
      childCurveTabApply();
   }
   else if(tab == TAB_CREATE_MATH)
   {
      mathTabApply();
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
   (void)index; // Tell the compiler not to warn that this variable is unused.

   int tab = ui->tabWidget->currentIndex();

   storeUserChildPlotNames();

   updateGuiPlotCurveInfo();

   bool showApplyButton = false;

   switch(tab)
   {
      case TAB_CREATE_CHILD_CURVE:
      {
         showApplyButton = true;
         setUserChildPlotNames();
         setMatchParentScrollChkBoxVisible();
      }
      break;

      case TAB_CREATE_MATH:
      {
         ui->chkSampRateAllCurves->setChecked(false); // Always reset when switching to the Math tab.
         fillInMathTab();
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
         fillInIpBlockTab(true);
      }
      break;

      case TAB_PROPERTIES:
      {
         showApplyButton = true;
         fillInPropTab(true);
      }
      break;

   }

   ui->cmdApply->setVisible(showApplyButton);

}


QVector<QString> curveProperties::getAllCurveNamesInPlot(QString plotName)
{
   QVector<QString> retVal;
   tCurveCommanderInfo allCurves = m_curveCmdr->getCurveCommanderInfo();
   tCurveDataInfo* curves = &(allCurves[plotName].curves);
   foreach( QString curveName, curves->keys() )
   {
      retVal.push_back(curveName);
   }
   return retVal;
}

void curveProperties::fillInMathTab()
{
   tPlotCurveComboBox::eCmbBoxCurveType type = m_cmbSrcCurve_math->getElementType();
   tPlotCurveAxis curveInfo = m_cmbSrcCurve_math->getPlotCurveAxis();
   CurveData* curve = m_curveCmdr->getCurveData(curveInfo.plotName, curveInfo.curveName);
   QVector<QString> allCurveName; // Will be used if no curve matches the user selection in the combobox.

   bool multCurveVisable = type == tPlotCurveComboBox::E_COMBOBOX_CURVE_AXIS_ALL ||
                           type == tPlotCurveComboBox::E_COMBOBOX_CURVE_ALL_CURVES;

   ui->grpMultCurve->setVisible(multCurveVisable);
   ui->chkSampRateAllCurves->setVisible(!multCurveVisable); // No need to allow user to set whether this applys to all curves when the curve selected is * (i.e. all curves)

   if(curve == NULL)
   {
      // Need to fill in sample rate, use the sample rate of the first curve found.
      allCurveName = getAllCurveNamesInPlot(curveInfo.plotName);
      if(allCurveName.size() > 0)
      {
         CurveData* firstCurveInList = m_curveCmdr->getCurveData(curveInfo.plotName, allCurveName[0]);
         setMathSampleRate(firstCurveInList);
      }
      else
      {
         ui->txtSampleRate->setText("");
         ui->chkSampRateAllCurves->setChecked(false);
      }
   }
   else
   {
      setMathSampleRate(curve);
   }

   setUserMathFromSrc(curveInfo, curve, type, allCurveName);
   displayUserMathOp();
}

void curveProperties::mathTabApply()
{
   tPlotCurveComboBox::eCmbBoxCurveType type = m_cmbSrcCurve_math->getElementType();
   tPlotCurveAxis curve = m_cmbSrcCurve_math->getPlotCurveAxis();

   // Initialize to default, single plot / curve / axis behavior.
   bool applyToOnlyOneCurveAxis = true;
   bool applyToAllCurves = false;
   bool overwriteAllCurOps = true;
   bool replaceFromTop = ui->radMultCurveTop->isChecked();

   switch(type)
   {
   default:
      break;
   case tPlotCurveComboBox::E_COMBOBOX_CURVE_AXIS_ALL:
      applyToOnlyOneCurveAxis = false;
      overwriteAllCurOps = ui->chkMultCurveOverwrite->isChecked();
      break;
   case tPlotCurveComboBox::E_COMBOBOX_CURVE_ALL_CURVES:
      applyToOnlyOneCurveAxis = false;
      applyToAllCurves = true;
      overwriteAllCurOps = ui->chkMultCurveOverwrite->isChecked();
      break;
   }

   double sampleRate = atof(ui->txtSampleRate->text().toStdString().c_str());

   MainWindow* parentPlotGui = m_curveCmdr->getMainPlot(curve.plotName);
   if(parentPlotGui != NULL)
   {
      if(applyToOnlyOneCurveAxis)
      {
         parentPlotGui->setCurveProperties(curve.curveName, curve.axis, sampleRate, m_mathOps);
         if(ui->chkSampRateAllCurves->isChecked() || !ui->chkSampRateAllCurves->isVisible()) // If chkSampRateAllCurves is invisible, then it is implied that the sample rate will be set for all curves.
         {
            parentPlotGui->setSampleRate_allCurves(sampleRate);
         }
      }
      else if(applyToAllCurves)
         parentPlotGui->setCurveProperties_allCurves(sampleRate, m_mathOps, overwriteAllCurOps, replaceFromTop, m_numMathOpsReadFromSrc);
      else
         parentPlotGui->setCurveProperties_allAxes(curve.curveName, sampleRate, m_mathOps, overwriteAllCurOps, replaceFromTop, m_numMathOpsReadFromSrc);

      m_cmbSrcCurve_math->userSpecified(true); // User hit the Apply button, i.e. user specified.
      fillInMathTab();
   }

   ui->chkSampRateAllCurves->setChecked(false); // Uncheck whenever Apply is clicked.
}

void curveProperties::on_cmbSrcCurve_math_currentIndexChanged(int index)
{
   (void)index; // Tell the compiler not to warn that this variable is unused.

   m_cmbSrcCurve_math->userSpecified(true, m_guiIsChanging);

   fillInMathTab();
}

void curveProperties::setUserMathFromSrc(tPlotCurveAxis &curveInfo, CurveData* curve, tPlotCurveComboBox::eCmbBoxCurveType type, QVector<QString>& curveNames)
{
   if(curve != NULL && type != tPlotCurveComboBox::E_COMBOBOX_CURVE_AXIS_ALL)
   {
      // Standard, single plot / curve / axis mode.
      m_mathOps = curve->getMathOps(curveInfo.axis);
      m_numMathOpsReadFromSrc = m_mathOps.size();
   }
   else
   {
      // Display math operations that are shared across multiple curves / axes.
      tMathOpList mathOpList;
      QVector<tMathOpList> allMathOpLists;

      switch(type)
      {
         case tPlotCurveComboBox::E_COMBOBOX_CURVE_ALL_CURVES:
         {
            // Fill in the list will all the curves / axes.
            for(int i = 0; i < curveNames.size(); ++i)
            {
               curve = m_curveCmdr->getCurveData(curveInfo.plotName, curveNames[i]);

               if(curve != NULL)
               {
                  if(curve->getPlotDim() == E_PLOT_DIM_1D)
                  {
                     allMathOpLists.push_back(curve->getMathOps(E_Y_AXIS));
                  }
                  else
                  {
                     allMathOpLists.push_back(curve->getMathOps(E_X_AXIS));
                     allMathOpLists.push_back(curve->getMathOps(E_Y_AXIS));
                  }
               }
            }
         }
         break;
         case tPlotCurveComboBox::E_COMBOBOX_CURVE_AXIS_ALL:
            // Fill in the list will all the axes for the given curve.
            if(curve != NULL)
            {
               allMathOpLists.push_back(curve->getMathOps(E_X_AXIS));
               allMathOpLists.push_back(curve->getMathOps(E_Y_AXIS));
            }
         break;
         default:
         break;
      }

      // Compare all the math operations in the list to find the common ones.
      // Start at either the top or bottom of the lists and stop when one of the lists
      // differs from the rest.
      if(allMathOpLists.size() > 0)
      {
         mathOpList = allMathOpLists[0];
         if(ui->radMultCurveTop->isChecked())
         {
            // Start from top. Find all common math operations at the top
            // of each list.
            for(int i = 1; i < allMathOpLists.size(); ++i)
            {
               tMathOpList matchingMathOps;
               tMathOpList::iterator curAllListEntry = allMathOpLists[i].begin();
               tMathOpList::iterator finalMathListEntry = mathOpList.begin();
               while(curAllListEntry != allMathOpLists[i].end() &&
                     finalMathListEntry != mathOpList.end())
               {
                  if(*curAllListEntry == *finalMathListEntry)
                  {
                     matchingMathOps.push_back(*curAllListEntry);
                     curAllListEntry++;
                     finalMathListEntry++;
                  }
                  else
                  {
                     break;
                  }
               }

               mathOpList = matchingMathOps;
            }
         }
         else
         {
            // Start from bottom. Find all common math operations at the bottom
            // of each list.
            for(int i = 1; i < allMathOpLists.size(); ++i)
            {
               tMathOpList matchingMathOps;
               tMathOpList::reverse_iterator curAllListEntry = allMathOpLists[i].rbegin();
               tMathOpList::reverse_iterator finalMathListEntry = mathOpList.rbegin();
               while(curAllListEntry != allMathOpLists[i].rend() &&
                     finalMathListEntry != mathOpList.rend())
               {
                  if(*curAllListEntry == *finalMathListEntry)
                  {
                     matchingMathOps.push_front(*curAllListEntry);
                     curAllListEntry++;
                     finalMathListEntry++;
                  }
                  else
                  {
                     break;
                  }
               }

               mathOpList = matchingMathOps;
            }
         }
         m_mathOps = mathOpList;
         m_numMathOpsReadFromSrc = m_mathOps.size();
      } // End if(allMathOpLists.size() > 0)
   } // End else (if(curve != NULL))
}

void curveProperties::displayUserMathOp()
{
   ui->opsOnCurve->clear();

   tMathOpList::iterator iter;

   for(iter = m_mathOps.begin(); iter != m_mathOps.end(); ++iter)
   {
#if 0 // Old version (didn't do a good job of switching in/out of scientific notation)
      char number[50];
      snprintf(number, sizeof(number), "%f", iter->num);
      number[sizeof(number)-1] = 0;
#else // New version (better at displaying those sig-figs)
      std::stringstream iostr;
      iostr << iter->num;
      QString number(iostr.str().c_str());
#endif
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
   bool invalidNumberFromStr = false;

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
         invalidNumberFromStr = true;
      }
   }

   if(invalidNumberFromStr == false || needsValue_eMathOp((eMathOp)m_selectedMathOpLeft) == false)
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
   ui->cmbRestorePlotNameFilter->addItem(GUI_ALL_VALUES);
   ui->cmbRestoreCurveNameFilter->addItem(GUI_ALL_VALUES);

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
   (void)index; // Tell the compiler not to warn that this variable is unused.

   fillRestoreTabListBox();
}

void curveProperties::on_cmbRestoreCurveNameFilter_currentIndexChanged(int index)
{
   (void)index; // Tell the compiler not to warn that this variable is unused.

   fillRestoreTabListBox();
}

// Get source curve start and stop indexes from the current zoom
// This will only work if the source curve is 1D
void curveProperties::on_cmdXUseZoomForSlice_clicked()
{
   useZoomForSlice(m_cmbXAxisSrc, ui->spnXSrcStart, ui->spnXSrcStop);
}

// Get source curve start and stop indexes from the current zoom
// This will only work if the source curve is 1D
void curveProperties::on_cmdYUseZoomForSlice_clicked()
{
   useZoomForSlice(m_cmbYAxisSrc, ui->spnYSrcStart, ui->spnYSrcStop);
}

void curveProperties::useZoomForSlice(tCmbBoxValPtr cmbAxisSrc, QSpinBox* spnStart, QSpinBox* spnStop)
{
   tPlotCurveAxis curve = cmbAxisSrc->getPlotCurveAxis();
   CurveData* parentCurve = m_curveCmdr->getCurveData(curve.plotName, curve.curveName);
   if(parentCurve != NULL && parentCurve->getPlotDim() == E_PLOT_DIM_1D)
   {
      // Valid 1D parent, continue
      MainWindow* mainPlot = m_curveCmdr->getMainPlot(curve.plotName);
      if(mainPlot != NULL)
      {
         maxMinXY dim = parentCurve->get1dDisplayedIndexes();

         dim.minX += 1; // Min is the sample before the first sample displayed.

         // Bound.
         if(dim.minX < 0 || dim.minX >= parentCurve->getNumPoints())
            dim.minX = 0;
         if(dim.maxX < 0 || dim.maxX >= parentCurve->getNumPoints())
            dim.maxX = 0;

         // Set GUI elements.
         spnStart->setValue(dim.minX);
         spnStop->setValue(dim.maxX);

         // User hit a button, i.e. user specified.
         cmbAxisSrc->userSpecified(true);
      }
   }
   else if(parentCurve != NULL && parentCurve->getPlotDim() == E_PLOT_DIM_2D)
   {
      // Valid 2D parent, continue
      MainWindow* mainPlot = m_curveCmdr->getMainPlot(curve.plotName);
      if(mainPlot != NULL)
      {
         unsigned numNonContiguousSamples = 0;
         maxMinXY dim = parentCurve->get2dDisplayedIndexes(numNonContiguousSamples);
         bool noPointsInZoomWindow = dim.minX < 0 || dim.maxX < 0; // Negative values mean no samples were in the zoom window.
         bool updateSlices = false;

         if(noPointsInZoomWindow)
         {
            QMessageBox msgBox;
            msgBox.setWindowTitle("No Samples in Zoom");
            msgBox.setText("There are no samples in the zoom window.");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
         }
         else
         {
            if(numNonContiguousSamples == 0)
            {
               updateSlices = true;
            }
            else
            {
               // Not contigous, ask user.
               unsigned totalNumSamp = dim.maxX - dim.minX + 1;
               unsigned numInZoomWindow = totalNumSamp - numNonContiguousSamples;
               QMessageBox::StandardButton reply;
               reply = QMessageBox::question( this,
                                              "Extra Samples Will Be Plotted",
                                              QString::number(numInZoomWindow) + " samples are within the current zoom window,"
                                              "but there are " + QString::number(numNonContiguousSamples) + " non-contiguous samples mixed in.\n\n"
                                              "Update Source Slice indexes even though some samples outside the current zoom window will get plotted?",
                                              QMessageBox::Yes|QMessageBox::No );
               if (reply == QMessageBox::Yes)
               {
                  updateSlices = true;
               }
            }
         }

         if(updateSlices)
         {
            dim.maxX += 1; // dim.maxX is inclusive but spnStop is exclusive. Add 1 to make it exclusive

            // Bound.
            if(dim.minX < 0 || dim.minX >= parentCurve->getNumPoints())
               dim.minX = 0;
            if(dim.maxX < 0 || dim.maxX >= parentCurve->getNumPoints())
               dim.maxX = 0;

            // Set GUI elements.
            spnStart->setValue(dim.minX);
            spnStop->setValue(dim.maxX);

            // User hit a button, i.e. user specified.
            cmbAxisSrc->userSpecified(true);
         }
      }
   }
   else
   {
      spnStart->setValue(0);
      spnStop->setValue(0);
   }
}

void curveProperties::on_cmdSaveCurveToFile_clicked()
{
   tPlotCurveAxis toSave = m_cmbCurveToSave->getPlotCurveAxis();
   CurveData* toSaveCurveData = m_curveCmdr->getCurveData(toSave.plotName, toSave.curveName);
   MainWindow* plotGui = m_curveCmdr->getMainPlot(toSave.plotName);

   if(toSaveCurveData != NULL)
   {
      // Use the last saved location to determine the folder to save the curve to.
      QString suggestedSavePath = m_curveCmdr->getOpenSavePath(toSave.curveName);

      // Read the last used filter from persistent memory.
      std::string persistentSaveStr = PERSIST_PARAM_CURVE_SAVE_PREV_SAVE_SELECTION_STR;
      std::string persistentReadValue;
      persistentParam_getParam_str(persistentSaveStr, persistentReadValue);

      // Generate Filter List string (i.e. the File Save types)
      QStringList filterList;
      filterList.append(OPEN_SAVE_FILTER_CURVE_STR);
      filterList.append(OPEN_SAVE_FILTER_CSV_STR);
      filterList.append(OPEN_SAVE_FILTER_C_HEADER_AUTO_TYPE_STR);
      filterList.append(OPEN_SAVE_FILTER_C_HEADER_INT_STR);
      filterList.append(OPEN_SAVE_FILTER_C_HEADER_FLOAT_STR);
      QString filterString = filterList.join(OPEN_SAVE_FILTER_DELIM);

      // Initialize selection with stored value (if there was one).
      QString selectedFilter;
      if(filterList.contains(persistentReadValue.c_str()))
         selectedFilter = persistentReadValue.c_str();

      // Open the save file dialog.
      QString fileName = QFileDialog::getSaveFileName(this, tr("Save Curve To File"),
                                                       suggestedSavePath,
                                                       filterString,
                                                       &selectedFilter,
                                                       QFileDialog::DontConfirmOverwrite);

      eSaveRestorePlotCurveType saveType = parseSaveFileName(fileName, selectedFilter);

      if(fileName != "") // Check for 'Cancel' case.
      {
         // Write user selections to persisent memory.
         m_curveCmdr->setOpenSavePath(fileName);
         persistentParam_setParam_str(persistentSaveStr, selectedFilter.toStdString());
         persistentParam_setParam_f64(PERSIST_PARAM_CURVE_SAVE_PREV_SAVE_SELECTION_INDEX, saveType);

         if(saveType != E_SAVE_RESTORE_INVALID)
         {
            SaveCurve packedCurve(plotGui, toSaveCurveData, saveType);
            PackedCurveData dataToWriteToFile;
            packedCurve.getPackedData(dataToWriteToFile);
            fso::WriteFile(fileName.toStdString(), &dataToWriteToFile[0], dataToWriteToFile.size());

            // User hit a button, i.e. user specified.
            m_cmbCurveToSave->userSpecified(true);
         }
      }
   }

}


void curveProperties::on_cmdSavePlotToFile_clicked()
{
   tCurveCommanderInfo allPlots = m_curveCmdr->getCurveCommanderInfo();
   QString plotName = m_cmbPlotToSave->currentText();

   if(allPlots.find(plotName) != allPlots.end())
   {
      // Use the last saved location to determine the folder to save the curve to.
      QString suggestedSavePath = m_curveCmdr->getOpenSavePath(plotName);

      // Read the last used filter from persistent memory.
      std::string persistentSaveStr = PERSIST_PARAM_PLOT_SAVE_PREV_SAVE_SELECTION_STR;
      std::string persistentReadValue;
      persistentParam_getParam_str(persistentSaveStr, persistentReadValue);

      // Generate Filter List string (i.e. the File Save types)
      QStringList filterList;
      filterList.append(OPEN_SAVE_FILTER_PLOT_STR);
      filterList.append(OPEN_SAVE_FILTER_CSV_STR);
      filterList.append(OPEN_SAVE_FILTER_C_HEADER_AUTO_TYPE_STR);
      filterList.append(OPEN_SAVE_FILTER_C_HEADER_INT_STR);
      filterList.append(OPEN_SAVE_FILTER_C_HEADER_FLOAT_STR);
      QString filterString = filterList.join(OPEN_SAVE_FILTER_DELIM);

      // Initialize selection with stored value (if there was one).
      QString selectedFilter;
      if(filterList.contains(persistentReadValue.c_str()))
         selectedFilter = persistentReadValue.c_str();

      // Open the save file dialog.
      QString fileName = QFileDialog::getSaveFileName(this, tr("Save Plot To File"),
                                                       suggestedSavePath,
                                                       filterString,
                                                       &selectedFilter,
                                                       QFileDialog::DontConfirmOverwrite);

      eSaveRestorePlotCurveType saveType = parseSaveFileName(fileName, selectedFilter);

      if(fileName != "") // Check for 'Cancel' case.
      {
         // Fill in vector of curve data in the correct order.
         QVector<CurveData*> curves;
         curves.resize(allPlots[plotName].curves.size());
         foreach( QString key, allPlots[plotName].curves.keys() )
         {
            int index = allPlots[plotName].plotGui->getCurveIndex(key);
            curves[index] = allPlots[plotName].curves[key];
         }

         // Write user selections to persisent memory.
         m_curveCmdr->setOpenSavePath(fileName);
         persistentParam_setParam_str(persistentSaveStr, selectedFilter.toStdString());
         persistentParam_setParam_f64(PERSIST_PARAM_PLOT_SAVE_PREV_SAVE_SELECTION_INDEX, saveType);

         if(saveType != E_SAVE_RESTORE_INVALID)
         {
            SavePlot savePlot(allPlots[plotName].plotGui, plotName, curves, saveType);
            PackedCurveData dataToWriteToFile;
            savePlot.getPackedData(dataToWriteToFile);
            fso::WriteFile(fileName.toStdString(), &dataToWriteToFile[0], dataToWriteToFile.size());

            // User hit the button, i.e. user specified.
            m_cmbPlotToSave->userSpecified(true);
         }
      }
   }
}

eSaveRestorePlotCurveType curveProperties::parseSaveFileName(QString& pathInOut, const QString& selectedFilter)
{
   // If path is empty, cancel or some equivalent was pressed. Return right away in that case.
   if(pathInOut == "")
      return E_SAVE_RESTORE_INVALID;

   // Use selectedFilter to determine the file format to save.
   eSaveRestorePlotCurveType saveType = E_SAVE_RESTORE_INVALID;
   QString ext = "";
   if(selectedFilter == OPEN_SAVE_FILTER_PLOT_STR)
   {
      saveType = E_SAVE_RESTORE_RAW;
      ext = ".plot";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_CURVE_STR)
   {
      saveType = E_SAVE_RESTORE_RAW;
      ext = ".curve";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_CSV_STR)
   {
      saveType = E_SAVE_RESTORE_CSV;
      ext = ".csv";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_C_HEADER_AUTO_TYPE_STR)
   {
      saveType = E_SAVE_RESTORE_C_HEADER_AUTO_TYPE;
      ext = ".h";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_C_HEADER_INT_STR)
   {
      saveType = E_SAVE_RESTORE_C_HEADER_INT;
      ext = ".h";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_C_HEADER_FLOAT_STR)
   {
      saveType = E_SAVE_RESTORE_C_HEADER_FLOAT;
      ext = ".h";
   }

   if(saveType != E_SAVE_RESTORE_INVALID) // Attempt to update pathInOut, but only if saveType is a valid File Format.
   {
      // Check if the correct file extension needs to be added to the save path (this seems to be needed in Linux)
      if(pathInOut.size() < ext.size())
      {
         pathInOut += ext; // path is too short to be the extension, so just add the extension to the end
      }
      else
      {
         // Check if the correct extension is at the end of the save path, if not fix the save path.
         QString pathEnd = pathInOut.right(ext.size());
         if(pathEnd != ext)
         {
            if(pathEnd.toLower() == ext)
            {
               pathInOut = pathInOut.left(pathInOut.size()-ext.size()); // Extension matches but is the wrong case, remove extension (it will be added back in the line below).
            }
            pathInOut += ext;
         }
      }
   }
   
   if(fso::FileExists(pathInOut.toStdString()))
   {
      // A file with this path name already exists. Ask the user if it should be overwritten.
      QMessageBox::StandardButton reply;
      reply = QMessageBox::question( this,
                                     "Confirm Save As!",
                                     pathInOut + " already exists.\nDo you want to replace it?",
                                     QMessageBox::Yes|QMessageBox::No );
      if (reply != QMessageBox::Yes)
      {
         // User doesn't want to replace the existing file.
         pathInOut = "";
         saveType = E_SAVE_RESTORE_INVALID;
      }
   }

   return saveType;
}

void curveProperties::on_cmdOpenCurveFromFile_clicked()
{
   m_curveCmdr->showOpenPlotFileDialog();
}

bool curveProperties::validateNewPlotCurveName(QString& plotName, QString& curveName)
{
   return localPlotCreate::validateNewPlotCurveName(m_curveCmdr, plotName, curveName);
}

void curveProperties::on_cmbPropPlotCurveName_currentIndexChanged(int index)
{
   (void)index; // Tell the compiler not to warn that this variable is unused.

   m_cmbPropPlotCurveName->userSpecified(true, m_guiIsChanging);

   fillInPropTab(true);
}

void curveProperties::fillInPropTab(bool userChangedPropertiesGuiSettings)
{
   tPlotCurveAxis plotCurveInfo = m_cmbPropPlotCurveName->getPlotCurveAxis();
   CurveData* parentCurve = m_curveCmdr->getCurveData(plotCurveInfo.plotName, plotCurveInfo.curveName);
   MainWindow* parentPlot = m_curveCmdr->getMainPlot(plotCurveInfo.plotName);
   if(parentCurve != NULL && parentPlot != NULL)
   {
      ui->txtPropPlotName->setText(plotCurveInfo.plotName);
      ui->txtPropCurveName->setText(plotCurveInfo.curveName);
      ui->txtPropNumSamp->setText(QString::number(parentCurve->getNumPoints()));
      ui->txtPropDim->setText(parentCurve->getPlotDim() == E_PLOT_DIM_1D ? "1D" : "2D");
      ui->spnPropCurvePos->setMaximum(parentPlot->getNumCurves()-1);

      maxMinXY curveDataMaxMin = parentCurve->getMaxMinXYOfData();
      ui->txtPropXMin->setText(QString::number(curveDataMaxMin.minX));
      ui->txtPropXMax->setText(QString::number(curveDataMaxMin.maxX));
      ui->txtPropYMin->setText(QString::number(curveDataMaxMin.minY));
      ui->txtPropYMax->setText(QString::number(curveDataMaxMin.maxY));

      // Set the calculated sample rate (this number might jump around a bit, make sure number of decimal points doesn't change).
      std::stringstream calcSampRateStr;
      calcSampRateStr << std::setprecision(3) << std::fixed << parentCurve->getCalculatedSampleRateFromPlotMsgs();
      ui->txtCalcSampleRate->setText(calcSampRateStr.str().c_str());

      // Fill in the Child Curve Parents GUI elements.
      fillInPropTab_childCurveParents(plotCurveInfo);

      // Fill in Last Msg Ip Addr field.
      ui->txtLastIp->setText(tPlotterIpAddr::convert(parentCurve->getLastMsgIpAddr().m_ipV4Addr));

      // Only update the writable values when the user manually manipulated the Curve Properties GUI.
      // We don't want to instantly overwrite a user change to a writable value
      // every time the curve changes.
      if(userChangedPropertiesGuiSettings)
      {
         // These are writable values.
         ui->spnPropCurvePos->setValue(parentPlot->getCurveIndex(plotCurveInfo.curveName));
         ui->chkPropHide->setChecked(parentCurve->getHidden());
         ui->chkPropVisable->setChecked(parentCurve->getVisible());

         // If hidden is checked, don't bother the user with the state of visible.
         ui->chkPropVisable->setVisible(!ui->chkPropHide->isChecked());

         // Set the Curve Color button to the color of the curve.
         CurveAppearance appear = parentCurve->getCurveAppearance();
         setPropTabCurveColor(appear.color);
         setPropTabCurveWidth(appear.width);
      }
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
       setPropTabCurveColor(Qt::black, true);
       setPropTabCurveWidth(1.0);
   }
}

void curveProperties::fillInPropTab_childCurveParents(tPlotCurveAxis &plotCurveInfo)
{
   QVector<tPlotCurveAxis> parent = m_curveCmdr->getCurveParents(plotCurveInfo.plotName, plotCurveInfo.curveName);
   int numNewItems = parent.size();
   int numCurrentGuiItems = ui->propParentCurves->count();

   // Store off the values that need be to written to the GUI.
   QVector<QString> newItems;
   for(int i = 0; i < numNewItems; ++i)
   {
      newItems.push_back(parent[i].plotName + PLOT_CURVE_SEP + parent[i].curveName);
   }

   // Check if the GUI values need to be changed.
   bool updateGui = true;
   if(numNewItems == numCurrentGuiItems)
   {
      // Check if all items match.
      updateGui = false;
      for(int i = 0; i < numNewItems; ++i)
      {
         if(ui->propParentCurves->item(i)->text() != newItems[i])
         {
            updateGui = true; // Mismatch detected, need to update GUI.
            break;
         }
      }
   }

   // Update the GUI values (only if a change is needed).
   if(updateGui)
   {
      ui->propParentCurves->clear();
      for(int i = 0; i < numNewItems; ++i)
      {
         ui->propParentCurves->addItem(newItems[i]);
      }
   }
}

void curveProperties::propTabApply()
{
   tPlotCurveAxis plotCurveInfo = m_cmbPropPlotCurveName->getPlotCurveAxis();
   CurveData* parentCurve = m_curveCmdr->getCurveData(plotCurveInfo.plotName, plotCurveInfo.curveName);
   MainWindow* parentPlot = m_curveCmdr->getMainPlot(plotCurveInfo.plotName);
   if(parentCurve != NULL && parentPlot != NULL)
   {
      // Set Curve Visible / Hidden State.
      parentPlot->setCurveVisibleHidden(plotCurveInfo.curveName, ui->chkPropVisable->isChecked(), ui->chkPropHide->isChecked());

      // Check for change of Curve Display Index
      if(parentPlot->getCurveIndex(plotCurveInfo.curveName) != ui->spnPropCurvePos->value())
      {
         parentPlot->setCurveIndex(plotCurveInfo.curveName, ui->spnPropCurvePos->value());
      }

      // Check for curve appearance change.
      CurveAppearance appearance = parentCurve->getCurveAppearance();
      QColor newColor = getPropTabCurveColor();
      double newWidth = getPropTabCurveWidth();
      if(newColor != appearance.color || newWidth != appearance.width)
      {
         // Update the appearance of the curve.
         appearance.color = newColor;
         appearance.width = newWidth;
         parentCurve->setCurveAppearance(appearance);
         parentPlot->saveCurveAppearance(plotCurveInfo.curveName, appearance); // Keep track of the user's changes from default values.
      }

   }
   fillInPropTab(true);
}

void curveProperties::on_cmdUnlinkParent_clicked()
{
   tPlotCurveAxis plotCurveInfo = m_cmbPropPlotCurveName->getPlotCurveAxis();
   m_curveCmdr->unlinkChildFromParents(plotCurveInfo.plotName, plotCurveInfo.curveName);
   fillInPropTab(); // Update Propties Tab with new information.
}

void curveProperties::on_cmdRemoveCurve_clicked()
{
   tPlotCurveAxis plotCurveInfo = m_cmbPropPlotCurveName->getPlotCurveAxis();
   // Ask the user if they are sure they want to remove the plot.
   QMessageBox::StandardButton reply;
   reply = QMessageBox::question( this,
                                  "Remove Plot",
                                  "Are you sure you want to permanently remove this curve: " + m_cmbPropPlotCurveName->currentText(),
                                  QMessageBox::Yes|QMessageBox::No );
   if (reply == QMessageBox::Yes)
   {
      m_curveCmdr->removeCurve(plotCurveInfo.plotName, plotCurveInfo.curveName);
      return; // removing Curve, nothing more to do.
   }
}

void curveProperties::setPersistentSuggestChildOnParentPlot(ePlotType plotType, bool childOnParentPlot)
{
   gSuggestChildOnParentPlot[plotType] = childOnParentPlot;
}

bool curveProperties::getPersistentSuggestChildOnParentPlot(ePlotType plotType, bool& childOnParentPlot)
{
   bool exists = gSuggestChildOnParentPlot.find(plotType) != gSuggestChildOnParentPlot.end();
   if(exists)
   {
      childOnParentPlot = gSuggestChildOnParentPlot[plotType];
   }

   return exists;
}

void curveProperties::getSuggestedChildPlotCurveName(ePlotType plotType, QString& plotName, QString& curveName)
{
   plotName = "";
   curveName = "";

   tPlotCurveAxis xSrc = m_cmbXAxisSrc->getPlotCurveAxis();
   tPlotCurveAxis ySrc = m_cmbYAxisSrc->getPlotCurveAxis();

   // Check for no plots.
   if(ySrc.plotName == "")
   {
      // Properies window is open, but there are not plots. Early return.
      return;
   }

   bool twoDInput = plotTypeHas2DInput(plotType);
   bool plotNameMustBeUnique = false;

   twoDInput = plotTypeHas2DInput(plotType);

   if(plotType == E_PLOT_TYPE_FFT_MEASUREMENT)
   {
      QString plotPrefix = plotTypeNames[plotType] + " of ";
      QStringList split = ui->cmbXAxisSrc_fftMeasurement->currentText().split(PLOT_CURVE_SEP); // Get Parent Plot Name and Suggested Curve Name for combo box.
      if(split.size() == 2)
      {
         // Generate the plot / curve names.
         plotName = plotPrefix + split[0]; // Use the Parent Plot Name.
         curveName = split[1]; // Set the suggested Child Curve Name to the FFT Measurement String.
         plotNameMustBeUnique = false;
      }
      else
      {
         // Generate the plot / curve names.
         plotName = plotPrefix + xSrc.plotName;
         curveName = xSrc.curveName;
         plotNameMustBeUnique = true;
      }
   }
   else if(plotType == E_PLOT_TYPE_CURVE_STATS)
   {
      unsigned childStatsIndex = ((unsigned)ui->cmbChildStatsTypes->currentIndex()) % ARRAY_SIZE(childStatsNames);
      plotName = plotTypeNames[plotType] + " of " + xSrc.curveName;
      curveName = childStatsNames[childStatsIndex];
   }
   else
   {
      // Determine if a previous child plot with the same plot type was generated on its parent plot.
      bool prevWasOnParent = false;
      bool validSuggestPrevOnParent = getPersistentSuggestChildOnParentPlot(plotType, prevWasOnParent);

      if( (validSuggestPrevOnParent && !prevWasOnParent) ||
          (!validSuggestPrevOnParent && plotType != E_PLOT_TYPE_AVERAGE) )
      {
         // Suggest Child on its own plot.
         QString plotPrefix = plotTypeNames[plotType] + " of ";
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

         // Generate the plot / curve names.
         plotName = plotPrefix + plotMid;
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
   QString curPlotNameText = m_cmbDestPlotName->currentText();
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
      m_cmbDestPlotName->setText(m_childCurveNewPlotNameUser);
   }
   else
   {
      m_cmbDestPlotName->setText(sugPlotName);
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
   (void)index; // Tell the compiler not to warn that this variable is unused.

   m_cmbXAxisSrc->userSpecified(true, m_guiIsChanging);
   if(m_cmbYAxisSrc->isVisible())
   {
      m_cmbYAxisSrc->userSpecified(true, m_guiIsChanging); // Lock the matching Y Axis value.
   }

   setUserChildPlotNames();
   setMatchParentScrollChkBoxVisible();
}

void curveProperties::on_cmbYAxisSrc_currentIndexChanged(int index)
{
   (void)index; // Tell the compiler not to warn that this variable is unused.

   m_cmbYAxisSrc->userSpecified(true, m_guiIsChanging);

   setUserChildPlotNames();
   setMatchParentScrollChkBoxVisible();
}

void curveProperties::fillInIpBlockTab(bool useLastIpAddr)
{
   // Fill In IP Address Combo Box
   QString origIpAddrText = ui->cmbIpAddrs->currentText();
   ipBlocker::tIpAddrs ipAddrs = m_ipBlocker->getIpAddrList();
   ui->cmbIpAddrs->clear();
   for(ipBlocker::tIpAddrs::iterator ipAddrsIter = ipAddrs.begin(); ipAddrsIter != ipAddrs.end(); ++ipAddrsIter)
   {
      ui->cmbIpAddrs->addItem(tPlotterIpAddr::convert(ipAddrsIter->m_ipV4Addr));
   }

   if(useLastIpAddr == true && m_lastUpdatedPlotIpAddr.m_ipV4Addr != 0)
   {
      // Set the combo text to the IP Address of the last message sent for the plot was set when the properties GUI was opened.
      ui->cmbIpAddrs->lineEdit()->setText(tPlotterIpAddr::convert(m_lastUpdatedPlotIpAddr.m_ipV4Addr));
   }
   else if(origIpAddrText != "")
   {
      // Restore combo box text.
      ui->cmbIpAddrs->lineEdit()->setText(origIpAddrText);
   }

   // Fill In IP Address Combo Box
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
         // Empty list means block all. Use * to indicate that all plots are blocked.
         ui->ipBlockPlotNames->addItem(ipAddrStr + PLOT_CURVE_SEP + GUI_ALL_VALUES);
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
      m_ipBlocker->addToBlockList(tPlotterIpAddr::convert(ui->cmbIpAddrs->currentText()), m_cmbIpBlockPlotNames->currentText());
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

      // Technically, a plot name might be the same as GUI_ALL_VALUES. Check for that case.
      if(plotName == GUI_ALL_VALUES && m_curveCmdr->validPlot(plotName) == false)
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

void curveProperties::on_radMultCurveTop_clicked()
{
   ui->radMultCurveBottom->setChecked(!ui->radMultCurveTop->isChecked());
   fillInMathTab();
}

void curveProperties::on_radMultCurveBottom_clicked()
{
   ui->radMultCurveTop->setChecked(!ui->radMultCurveBottom->isChecked());
   fillInMathTab();
}

void curveProperties::on_chkWindow_clicked(bool checked)
{
   ui->chkScaleFftWindow->setEnabled(checked);
}

void curveProperties::on_cmdCreateFromData_clicked()
{
   m_curveCmdr->showCreatePlotFromDataGui("", NULL);
}


void curveProperties::initCmbBoxValueFromPersistParam(QComboBox* cmbBoxPtr, const std::string persistParamName)
{
   // Attempt to restore the combo box value from persistent memory.
   double index_f64 = -1; // Init to invalid index.
   bool restoreSuccess = persistentParam_getParam_f64(persistParamName, index_f64);
   if(restoreSuccess)
   {
      int index_int = index_f64; // Double float to int.
      if(index_int >= 0 && index_int < cmbBoxPtr->count())
      {
         cmbBoxPtr->setCurrentIndex(index_int);
      }
   }
}

void curveProperties::initSpnBoxValueFromPersistParam(QSpinBox* spnBoxPtr, const std::string persistParamName)
{
   // Attempt to restore the spin box value from persistent memory.
   double value_f64 = -1; // Init to invalid index.
   bool restoreSuccess = persistentParam_getParam_f64(persistParamName, value_f64);
   if(restoreSuccess)
   {
      int value_int = value_f64; // Double float to int.
      spnBoxPtr->setValue(value_int);
   }
}

void curveProperties::on_chkPropHide_clicked()
{
   // If hidden is checked, don't bother the user with the state of visible.
   ui->chkPropVisable->setVisible(!ui->chkPropHide->isChecked());
}

void curveProperties::on_cmbXAxisSrc_fftMeasurement_currentIndexChanged(int index)
{
   (void)index; // Tell the compiler not to warn that this variable is unused.

   setUserChildPlotNames();
   setMatchParentScrollChkBoxVisible();
}

// FFT Measurement Child Plots use a different Combo Box than the rest of the Child Curve Plot
// Types. Also, they do not reference a 'Curve Name'. Instead the Curve Name indicates which
// FFT Measurement to plot.
bool curveProperties::determineChildFftMeasurementAxisValues(tParentCurveInfo& axisParent)
{
   bool validMatchFound = false;
   // Need to update dataSrc to get the plot / curve names from the correct combo box.
   QStringList split = ui->cmbXAxisSrc_fftMeasurement->currentText().split(PLOT_CURVE_SEP);
   if(split.size() == 2) // Make sure the FFT Measurement Combo Box text is valid
   {
      axisParent.dataSrc.plotName = split[0];
      axisParent.dataSrc.curveName = split[1];
      axisParent.dataSrc.axis = E_Y_AXIS;

      for(int i = 0; i < E_FFT_MEASURE__NO_FFT_MEASUREMENT; ++i)
      {
         if(axisParent.dataSrc.curveName == fftMeasureNames[i])
         {
            axisParent.fftMeasurementType = (eFftSigNoiseMeasurements)i;
            axisParent.curveStatstPlotSize = ui->spnFFtMeasChildPlotSize->value();
            validMatchFound = true;
            break;
         }
      }
   }
   return validMatchFound;
}

void curveProperties::setMatchParentScrollChkBoxVisible()
{
   // All Child Plots will use m_cmbXAxisSrc. So, if the curve defined in that GUI dropdown box
   // is in scroll mode and the plot type isn't FFT, show the "Match Parent Scroll Mode" check box.
   bool isVisible = false;

   ePlotType plotType = (ePlotType)ui->cmbPlotType->currentIndex();

   // FFT child plots handle scroll mode in their own way, also plots based on FFT measurements shouldn't inherit parent's scroll mode.
   if(plotTypeIsFft(plotType) == false && plotType != E_PLOT_TYPE_FFT_MEASUREMENT && plotType != E_PLOT_TYPE_CURVE_STATS)
   {
      tPlotCurveAxis curve = m_cmbXAxisSrc->getPlotCurveAxis();
      CurveData* parentCurve = m_curveCmdr->getCurveData(curve.plotName, curve.curveName);
      if(parentCurve != NULL && parentCurve->getScrollMode())
      {
         isVisible = true;
      }
   }

   ui->grpScrollModeOptions->setVisible(isVisible);
}

void curveProperties::on_chkMatchParentScroll_clicked()
{
   persistentParam_setParam_f64(PERSIST_PARAM_MATCH_PARENT_SCROLL, ui->chkMatchParentScroll->isChecked() ? 1.0 : 0.0); // Store off check box state.
}

QColor curveProperties::getPropTabCurveColor()
{
   return ui->cmdPropColor->palette().color(QPalette::ButtonText);
}

void curveProperties::setPropTabCurveColor(QColor color, bool setToDefault)
{
   if(color.isValid())
   {
      // No Text is needed.
      ui->cmdPropColor->setText("");

      // Set the background color.
      if(setToDefault)
      {
         ui->cmdPropColor->setStyleSheet("");
      }
      else
      {
         QString rgbColorStr = QString::number(color.red()) + "," + QString::number(color.green()) + "," + QString::number(color.blue());
         ui->cmdPropColor->setStyleSheet("background-color: rgb(" + rgbColorStr + ")");
      }

      // Store color as the ButtonText color (this is easier to read than background color.
      QPalette colorButtonPalette = ui->cmdPropColor->palette();
      colorButtonPalette.setColor(QPalette::ButtonText, color);
      ui->cmdPropColor->setPalette(colorButtonPalette);
      ui->cmdPropColor->update();
   }
   else
   {
      assert(0);
   }
}


void curveProperties::on_cmdPropColor_clicked()
{
   tPlotCurveAxis plotCurveInfo = m_cmbPropPlotCurveName->getPlotCurveAxis();
   CurveData* parentCurve = m_curveCmdr->getCurveData(plotCurveInfo.plotName, plotCurveInfo.curveName);
   if(parentCurve != NULL)
   {
      QColor newColor = QColorDialog::getColor(getPropTabCurveColor(), this);

      if(newColor.isValid())
      {
         setPropTabCurveColor(newColor);
      }
   }
}

void curveProperties::on_cmbChildStatsTypes_currentIndexChanged(int index)
{
   (void)index; // Tell the compiler not to warn that this variable is unused.

   setUserChildPlotNames();
   setMatchParentScrollChkBoxVisible();
}

double curveProperties::getPropTabCurveWidth()
{
   return ui->spnWidth->value();
}

void curveProperties::setPropTabCurveWidth(double width)
{
   ui->spnWidth->setValue(width);
}

void curveProperties::on_cmdAutoSetSampRate_clicked()
{
   // Fill in the sample rate based on the rate that the plot data is getting plotted at.
   tPlotCurveAxis curve = m_cmbSrcCurve_math->getPlotCurveAxis();
   CurveData* parentCurve = m_curveCmdr->getCurveData(curve.plotName, curve.curveName);

   // If all curves are selected (via the * curve name), just use the first curve name in the plot.
   if(parentCurve == NULL && curve.curveName == "")
   {
      auto allCurveName = getAllCurveNamesInPlot(curve.plotName);
      if(allCurveName.size() > 0)
      {
         parentCurve = m_curveCmdr->getCurveData(curve.plotName, allCurveName[0]);
      }
   }

   if(parentCurve != NULL)
   {
      const double calcSampRate = parentCurve->getCalculatedSampleRateFromPlotMsgs();
      if(calcSampRate > 0.0)
      {
         // The calculated sample rate will have some error. Keep reducing significant figures and rounding
         // to find the closest match with the least amount of significant figures.
         static const double HOW_CLOSE_FOR_MATCH = 0.01; // 1.0 %
         static const int MAX_NUM_SIG_FIGS = 4;

         int exponent = (int)(log10(calcSampRate));
         double sampRateToUse = 0.0;
         bool found = false;
         for(int sigFig = MAX_NUM_SIG_FIGS; sigFig >= 1; --sigFig)
         {
            double divisor = pow(10, exponent - sigFig + 1);
            int sigFigRateInt = (int)(calcSampRate / divisor); // Make sure to round down.
            double sigFigRateLo = (double)sigFigRateInt * divisor;
            double sigFigRateHi = (double)(sigFigRateInt+1) * divisor;

            double loError = std::min(abs(1.0 - (sigFigRateLo/calcSampRate)), abs(1.0 - (calcSampRate/sigFigRateLo)));
            double hiError = std::min(abs(1.0 - (sigFigRateHi/calcSampRate)), abs(1.0 - (calcSampRate/sigFigRateHi)));

            if(loError < hiError && loError <= HOW_CLOSE_FOR_MATCH)
            {
               sampRateToUse = sigFigRateLo;
               found = true;
            }
            else if(hiError < loError && hiError <= HOW_CLOSE_FOR_MATCH)
            {
               sampRateToUse = sigFigRateHi;
               found = true;
            }
         }

         if(found)
         {
            std::stringstream calcSampRateStr;
            calcSampRateStr << std::setprecision(3) << std::fixed << sampRateToUse;
            ui->txtSampleRate->setText(calcSampRateStr.str().c_str());
         }
      }
   }
}
