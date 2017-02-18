/* Copyright 2014 - 2017 Dan Williams. All Rights Reserved.
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
#include <QVector>
#include <list>
#include "PlotHelperTypes.h"
#include "CurveData.h"
#include "saveRestoreCurve.h"

namespace Ui {
class curveProperties;
}

class tCmbBoxAndValue
{
public:
   typedef enum
   {
      E_PREFERRED_AXIS_X,
      E_PREFERRED_AXIS_Y,
      E_PREFERRED_AXIS_DONT_CARE
   }ePreferredAxis;

   tCmbBoxAndValue(){}
   tCmbBoxAndValue(QComboBox* cmbBoxPtrIn):
      cmbBoxPtr(cmbBoxPtrIn),
      cmbBoxVal(""),
      displayAxesSeparately(true),
      preferredAxis(E_PREFERRED_AXIS_DONT_CARE)
   {}

   tCmbBoxAndValue(QComboBox* cmbBoxPtrIn, bool displayAxesSeparate):
      cmbBoxPtr(cmbBoxPtrIn),
      cmbBoxVal(""),
      displayAxesSeparately(displayAxesSeparate),
      preferredAxis(E_PREFERRED_AXIS_DONT_CARE)
   {}

   tCmbBoxAndValue(QComboBox* cmbBoxPtrIn, ePreferredAxis preferredAxisIn):
      cmbBoxPtr(cmbBoxPtrIn),
      cmbBoxVal(""),
      displayAxesSeparately(true),
      preferredAxis(preferredAxisIn)
   {}

   QComboBox*     cmbBoxPtr;
   QString        cmbBoxVal;
   bool           displayAxesSeparately;
   ePreferredAxis preferredAxis;
};

class CurveCommander;

class curveProperties : public QWidget
{
   Q_OBJECT
   
public:
   explicit curveProperties(CurveCommander* curveCmdr, QString plotName = "", QString curveName = "", QWidget *parent = 0);
   ~curveProperties();

   void updateGuiPlotCurveInfo(QString plotName = "", QString curveName = "");
   
   void existingPlotsChanged();

private slots:
   void on_cmbPlotType_currentIndexChanged(int index);
   void on_cmdApply_clicked();

   void on_cmdClose_clicked();

   void on_tabWidget_currentChanged(int index);

   void on_cmbSrcCurve_math_currentIndexChanged(int index);

   void on_availableOps_currentRowChanged(int currentRow);

   void on_opsOnCurve_currentRowChanged(int currentRow);

   void on_cmdOpRight_clicked();

   void on_cmdOpUp_clicked();

   void on_cmdOpDown_clicked();

   void on_cmdOpDelete_clicked();

   void on_chkSrcSlice_clicked();

   void on_cmbRestorePlotNameFilter_currentIndexChanged(int index);

   void on_cmbRestoreCurveNameFilter_currentIndexChanged(int index);


   void on_cmdXUseZoomForSlice_clicked();

   void on_cmdYUseZoomForSlice_clicked();

   void on_cmdSaveCurveToFile_clicked();

   void on_cmdOpenCurveFromFile_clicked();

   void on_cmbPropPlotCurveName_currentIndexChanged(int index);

   void on_cmdUnlinkParent_clicked();

   void on_cmdRemoveCurve_clicked();

   void on_cmbXAxisSrc_currentIndexChanged(int index);

   void on_cmbYAxisSrc_currentIndexChanged(int index);

   void on_cmdSavePlotToFile_clicked();

   void on_cmdIpBlockAdd_clicked();

   void on_cmdIpBlockRemove_clicked();

   void on_chkBlockAll_clicked();

   void on_cmdIpBlockRemoveAll_clicked();

private:
   void closeEvent(QCloseEvent* event);

   void setCombosToPrevValues();
   void setCombosToPlotCurve(const QString& plotName, const QString& curveName, const QString& realCurveName, const QString& imagCurveName);
   void findRealImagCurveNames(QList<QString>& curveNameList, const QString& defaultCurveName, QString& realCurveName, QString& imagCurveName);
   bool trySetComboItemIndex(QComboBox* cmbBox, QString text);
   int getMatchingComboItemIndex(QComboBox* cmbBox, QString text);

   tPlotCurveAxis getSelectedCurveInfo(QComboBox* cmbBox);

   void setMathSampleRate(CurveData *curve);
   void displayUserMathOp();
   void setUserMathFromSrc(tPlotCurveAxis& curveInfo, CurveData *curve);

   bool validateNewPlotCurveName(QString& plotName, QString& curveName);

   void fillInPropTab(bool activeTabChangedToPropertiesTab = false);
   void propTabApply();

   void getSuggestedChildPlotCurveName(ePlotType plotType, QString& plotName, QString& curveName);

   void storeUserChildPlotNames(ePlotType plotType = (ePlotType)-1);
   void setUserChildPlotNames();

   QString getOpenSaveDir();
   QString getOpenSavePath(QString fileName);
   void setOpenSavePath(QString path);

   // Restore Tab Functions / Parameters
   void fillRestoreTabListBox();
   QVector<tStoredMsg> m_storedMsgs;

   void fillRestoreFilters();
   QVector<QString> m_restoreFilterPlotName;
   QVector<QString> m_restoreFilterCurveName;

   // IP Block Functions / Parameters
   ipBlocker* m_ipBlocker;
   void fillInIpBlockTab();


   Ui::curveProperties *ui;

   CurveCommander* m_curveCmdr;

   int m_selectedMathOpLeft;
   int m_selectedMathOpRight;

   tMathOpList m_mathOps;

   QVector<tCmbBoxAndValue> m_plotCurveCombos;
   QVector<tCmbBoxAndValue> m_plotNameCombos;

   QString m_childCurveNewPlotNameUser;
   QString m_childCurveNewCurveNameUser;
   int m_prevChildCurvePlotTypeIndex;
};

#endif // CURVEPROPERTIES_H
