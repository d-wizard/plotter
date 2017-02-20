/* Copyright 2017 Dan Williams. All Rights Reserved.
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
#ifndef PLOTCURVECOMBOBOX_H
#define PLOTCURVECOMBOBOX_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QVector>
#include <QMutex>
#include <QString>
#include <QSharedPointer>
#include "PlotHelperTypes.h"

class tPlotCurveComboBox
{
public:
   typedef enum
   {
      E_COMBOBOX_PLOT_NAME_ONLY,
      E_COMBOBOX_CURVE_AXIS_X,
      E_COMBOBOX_CURVE_AXIS_Y,
      E_COMBOBOX_CURVE_ENTIRE_CURVE,
      E_COMBOBOX_CURVE_ALL_CURVES
   }eCmbBoxCurveType;

   // Only valid contructor.
   tPlotCurveComboBox(QComboBox* cmbBoxPtrIn):
      cmbBoxChangeMutex(QMutex::Recursive),
      cmbBoxPtr(cmbBoxPtrIn)
   {}

   void setVisible(bool vis) {cmbBoxPtr->setVisible(vis);}

   QString currentText() {return cmbBoxPtr->currentText();}
   void setText(QString text) {cmbBoxPtr->lineEdit()->setText(text);}

   void clear()
   {
      QMutexLocker lock(&cmbBoxChangeMutex);
      cmbBoxPtr->clear();
      cmbBoxActualValues.clear();
   }

   void addItem(const tPlotCurveName& plotCurve, const eCmbBoxCurveType cmbBoxType)
   {
      addItem(plotCurve.plot, plotCurve.curve, cmbBoxType);
   }
   void addItem(const QString& plotName, const eCmbBoxCurveType cmbBoxType)
   {
      QString dummyCurve = "";
      addItem(plotName, dummyCurve, cmbBoxType);
   }
   void addItem(const QString& plotName, const QString& curveName, const eCmbBoxCurveType cmbBoxType)
   {
      tCmbBoxPlotCurveElement newElement;
      newElement.plotCurveName.plot = plotName;
      newElement.plotCurveName.curve = curveName;
      newElement.cmbBoxType = cmbBoxType;

      QString newCmbBoxString;
      switch(cmbBoxType)
      {
      case E_COMBOBOX_PLOT_NAME_ONLY:
         newCmbBoxString = plotName;
         break;
      case E_COMBOBOX_CURVE_AXIS_X:
         newCmbBoxString = plotName + PLOT_CURVE_SEP + curveName + X_AXIS_APPEND;
         break;
      case E_COMBOBOX_CURVE_AXIS_Y:
         newCmbBoxString = plotName + PLOT_CURVE_SEP + curveName + Y_AXIS_APPEND;
         break;
      case E_COMBOBOX_CURVE_ENTIRE_CURVE:
         newCmbBoxString = plotName + PLOT_CURVE_SEP + curveName;
         break;
      case E_COMBOBOX_CURVE_ALL_CURVES:
         newCmbBoxString = plotName + PLOT_CURVE_SEP + GUI_ALL_VALUES;
         break;
      }

      QMutexLocker lock(&cmbBoxChangeMutex);
      cmbBoxActualValues.push_back(newElement);
      cmbBoxPtr->addItem(newCmbBoxString);
   }

   QString getPlot()
   {
      QMutexLocker lock(&cmbBoxChangeMutex);

      QString plotName = cmbBoxPtr->currentText();
      int index = getMatchingComboItemIndex(plotName);
      if(index >= 0)
      {
         return cmbBoxActualValues[index].plotCurveName.plot;
      }
      return plotName;

   }

   tPlotCurveName getPlotCurve()
   {
      QMutexLocker lock(&cmbBoxChangeMutex);

      // User will never be asked to manually enter a plot / curve string. So
      // the current text value of the combo box should alway match an existing entry.
      int index = getMatchingComboItemIndex(cmbBoxPtr->currentText());
      if(index >= 0)
      {
         return cmbBoxActualValues[index].plotCurveName;
      }

      tPlotCurveName retVal = {"", ""};
      return retVal;
   }

   tPlotCurveAxis getPlotCurveAxis()
   {
      bool dummy;
      return getPlotCurveAxis(dummy);
   }

   tPlotCurveAxis getPlotCurveAxis(bool& isAllCurves)
   {
      QMutexLocker lock(&cmbBoxChangeMutex);

      tPlotCurveAxis retVal = {"", "", E_Y_AXIS};

      // User will never be asked to manually enter a plot / curve / axis string. So
      // the current text value of the combo box should alway match an existing entry.
      int index = getMatchingComboItemIndex(cmbBoxPtr->currentText());
      if(index >= 0)
      {
         tCmbBoxPlotCurveElement matchingElement = cmbBoxActualValues[index];

         retVal.plotName  = matchingElement.plotCurveName.plot;
         retVal.curveName = matchingElement.plotCurveName.curve;
         retVal.axis      = matchingElement.cmbBoxType == E_COMBOBOX_CURVE_AXIS_X ?
                            E_X_AXIS : E_Y_AXIS; // default to Y axis.
         isAllCurves = matchingElement.cmbBoxType == E_COMBOBOX_CURVE_ALL_CURVES;
      }
      return retVal;
   }

   bool trySetComboItemIndex(QString textToMatchToExistingValue)
   {
      int cmbIndex = getMatchingComboItemIndex(textToMatchToExistingValue);
      if(cmbIndex >= 0)
      {
         cmbBoxPtr->setCurrentIndex(cmbIndex);
         return true;
      }
      else
      {
         return false;
      }
   }

private:

   // Eliminate default, copy, assign
   tPlotCurveComboBox();
   tPlotCurveComboBox(tPlotCurveComboBox const&);
   void operator=(tPlotCurveComboBox const&);

   // Private Types
   typedef struct
   {
      tPlotCurveName plotCurveName;
      eCmbBoxCurveType cmbBoxType;
   }tCmbBoxPlotCurveElement;

   // Member Variables
   QMutex cmbBoxChangeMutex;
   QVector<tCmbBoxPlotCurveElement> cmbBoxActualValues;
   QComboBox* cmbBoxPtr;

   // Private Functions.
   int getMatchingComboItemIndex(QString text)
   {
      int retVal = -1;
      for(int i = 0; i < cmbBoxPtr->count(); ++i)
      {
         if(cmbBoxPtr->itemText(i) == text)
         {
            retVal = i;
            break;
         }
      }
      return retVal;
   }

};

typedef QSharedPointer<tPlotCurveComboBox> tPltCrvCmbBoxPtr;


#endif // PLOTCURVECOMBOBOX_H
