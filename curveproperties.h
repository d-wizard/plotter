/* Copyright 2014 - 2019 Dan Williams. All Rights Reserved.
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
#include "plotCurveComboBox.h"

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
   tCmbBoxAndValue(tPltCrvCmbBoxPtr cmbBoxPtrIn):
      cmbBoxPtr(cmbBoxPtrIn),
      cmbBoxVal(""),
      displayAxesSeparately(true),
      displayAllCurves(false),
      displayAllAxes(false),
      preferredAxis(E_PREFERRED_AXIS_DONT_CARE)
   {}

   tCmbBoxAndValue(tPltCrvCmbBoxPtr cmbBoxPtrIn, bool displayAxesSeparate):
      cmbBoxPtr(cmbBoxPtrIn),
      cmbBoxVal(""),
      displayAxesSeparately(displayAxesSeparate),
      displayAllCurves(false),
      displayAllAxes(false),
      preferredAxis(E_PREFERRED_AXIS_DONT_CARE)
   {}

   tCmbBoxAndValue(tPltCrvCmbBoxPtr cmbBoxPtrIn, ePreferredAxis preferredAxisIn):
      cmbBoxPtr(cmbBoxPtrIn),
      cmbBoxVal(""),
      displayAxesSeparately(true),
      displayAllCurves(false),
      displayAllAxes(false),
      preferredAxis(preferredAxisIn)
   {}

   tCmbBoxAndValue(tPltCrvCmbBoxPtr cmbBoxPtrIn, bool displayAllCurvesIn, bool displayAllAxesIn):
      cmbBoxPtr(cmbBoxPtrIn),
      cmbBoxVal(""),
      displayAxesSeparately(true),
      displayAllCurves(displayAllCurvesIn),
      displayAllAxes(displayAllAxesIn),
      preferredAxis(E_PREFERRED_AXIS_DONT_CARE)
   {}

   tPltCrvCmbBoxPtr cmbBoxPtr;
   QString          cmbBoxVal;
   bool             displayAxesSeparately;
   bool             displayAllCurves;
   bool             displayAllAxes;
   ePreferredAxis   preferredAxis;
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

   void on_radMultCurveTop_clicked();

   void on_radMultCurveBottom_clicked();

   void on_chkWindow_clicked(bool checked);

   void on_cmdCreateFromData_clicked();

   void on_chkPropHide_clicked();

private:
   void closeEvent(QCloseEvent* event);

   void setCombosToPrevValues();
   void setCombosToPlotCurve(const QString& plotName, const QString& curveName, const QString& realCurveName, const QString& imagCurveName, bool tryToRestoreFirst);
   void findRealImagCurveNames(QList<QString>& curveNameList, const QString& defaultCurveName, QString& realCurveName, QString& imagCurveName);
   bool trySetComboItemIndex(tPltCrvCmbBoxPtr cmbBox, QString text);

   void fillInMathTab();
   void mathTabApply();
   void setMathSampleRate(CurveData *curve);
   void displayUserMathOp();
   void setUserMathFromSrc(tPlotCurveAxis& curveInfo, CurveData *curve, tPlotCurveComboBox::eCmbBoxCurveType, QVector<QString>& curveNames);

   QVector<QString> getAllCurveNamesInPlot(QString plotName);

   bool validateNewPlotCurveName(QString& plotName, QString& curveName);

   void fillInPropTab(bool userChangedPropertiesGuiSettings = false);
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
   tPlotterIpAddr m_lastUpdatedPlotIpAddr;
   void fillInIpBlockTab(bool useLastIpAddr = false);

   void initCmbBoxValueFromPersistParam(QComboBox* cmbBoxPtr, const std::string persistParamName);

   Ui::curveProperties *ui;

   CurveCommander* m_curveCmdr;

   int m_selectedMathOpLeft;
   int m_selectedMathOpRight;

   tMathOpList m_mathOps;
   int m_numMathOpsReadFromSrc;

   // Plot / Curve Combo boxes
   tPltCrvCmbBoxPtr m_cmbXAxisSrc;
   tPltCrvCmbBoxPtr m_cmbYAxisSrc;
   tPltCrvCmbBoxPtr m_cmbSrcCurve_math;
   tPltCrvCmbBoxPtr m_cmbCurveToSave;
   tPltCrvCmbBoxPtr m_cmbPropPlotCurveName;
   tPltCrvCmbBoxPtr m_cmbDestPlotName;
   tPltCrvCmbBoxPtr m_cmbOpenCurvePlotName;
   tPltCrvCmbBoxPtr m_cmbPlotToSave;
   tPltCrvCmbBoxPtr m_cmbIpBlockPlotNames;


   QVector<tCmbBoxAndValue> m_plotCurveCombos;
   QVector<tCmbBoxAndValue> m_plotNameCombos;

   QString m_childCurveNewPlotNameUser;
   QString m_childCurveNewCurveNameUser;
   int m_prevChildCurvePlotTypeIndex;
};

#endif // CURVEPROPERTIES_H
