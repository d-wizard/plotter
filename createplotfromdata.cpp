/* Copyright 2017, 2019, 2021 Dan Williams. All Rights Reserved.
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
#include <QMessageBox>
#include <QKeyEvent>
#include <QClipboard>
#include "createplotfromdata.h"
#include "ui_createplotfromdata.h"
#include "AutoDelimiter.h"
#include "persistentParameters.h"
#include "dString.h"

static const std::string HEX_TYPE_PERSIST_PARAM_NAME = "createPlotFromDataHexType";

createPlotFromData::createPlotFromData(CurveCommander *curveCmdr, QString plotName, const char *data, QWidget *parent) :
   QDialog(parent),
   ui(new Ui::createPlotFromData),
   m_curveCmdr(curveCmdr),
   m_inData(""),
   m_delim(""),
   m_changingDelim(false),
   m_userSpecifiedDelim(false),
   m_changingBase(false),
   m_userSpecifiedBase(false)
{
   m_dataVectInt.clear();
   m_dataVectFloat.clear();

   ui->setupUi(this);

   setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint)); // Get rid of the help button.

   // Restore Hex Type from persistent parameters
   double hexTypeIndexDouble = -1;
   if(persistentParam_getParam_f64(HEX_TYPE_PERSIST_PARAM_NAME, hexTypeIndexDouble))
   {
      int hexTypeIndex = hexTypeIndexDouble;
      if(hexTypeIndex >= 0 && hexTypeIndex < ui->cmbHexType->count())
      {
         ui->cmbHexType->setCurrentIndex(hexTypeIndex);
      }
   }

   plotName = ""; // This is kind of a hack. I don't like the idea of suggesting a copy/paste curve being created on the same plot that it was pasted on.
   ui->cmbPlotName->lineEdit()->setText(plotName);

   fillInValidPlotCurve();
   handleNewData(data);
}

createPlotFromData::~createPlotFromData()
{
   delete ui;
}

bool createPlotFromData::isGuiHex()
{
   return ui->cmbBase->currentIndex() != 0;
}

void createPlotFromData::updateGuiPlotCurveInfo()
{
   tCurveCommanderInfo allCurves = m_curveCmdr->getCurveCommanderInfo();

   int newPlotNameCmbIndex = -1;
   int count = 0;
   QString origPlotName = ui->cmbPlotName->currentText();

   ui->cmbPlotName->clear();

   foreach( QString curPlotName, allCurves.keys() )
   {
      ui->cmbPlotName->addItem(curPlotName);
      if(curPlotName == origPlotName)
      {
         newPlotNameCmbIndex = count;
      }
      count++;
   }

   if(newPlotNameCmbIndex >= 0)
   {
      ui->cmbPlotName->setCurrentIndex(newPlotNameCmbIndex);
   }
   else
   {
      ui->cmbPlotName->lineEdit()->setText(origPlotName);
   }
}

void createPlotFromData::fillInValidPlotCurve()
{
   QString plotName = ui->cmbPlotName->currentText();
   if(plotName == "")
   {
      QString defaultNewPlotName = "New Plot";
      QString newPlotName = defaultNewPlotName;
      int count = 1;
      while(m_curveCmdr->validPlot(newPlotName) == true)
      {
         newPlotName = defaultNewPlotName + " " + QString::number(count);
         count++;
      }
      plotName = newPlotName;
      ui->cmbPlotName->lineEdit()->setText(plotName);
   }
   if(ui->txtCurveName->text() == "")
   {
      QString defaultNewCurveName = "New Data";
      QString newCurveName = defaultNewCurveName;
      int count = 1;
      while(m_curveCmdr->validCurve(plotName, newCurveName) == true)
      {
         newCurveName = defaultNewCurveName + " " + QString::number(count);
         count++;
      }
      ui->txtCurveName->setText(newCurveName);
   }
}



void createPlotFromData::handleNewData(const char *data, bool newDataIsFromGui)
{
   if(newDataIsFromGui)
   {
      m_inData = ui->txtDataToPlot->text();
   }
   else
   {
      if(data != NULL)
      {
         m_inData = data;
      }

      ui->txtDataToPlot->setText(m_inData);
   }


   bool isHex;
   if(m_userSpecifiedBase == false)
   {
      isHex = autoDetectHex(m_inData.toStdString());

      m_changingBase = true;
      if(isHex)
         ui->cmbBase->setCurrentIndex(1);
      else
         ui->cmbBase->setCurrentIndex(0);
      m_changingBase = false;
   }
   else
   {
      isHex = isGuiHex();
   }

   if( (m_userSpecifiedDelim == false) && ui->chkAutoDelim->isChecked())
   {

      m_delim = getDelimiter(m_inData.toStdString(), isHex).c_str();

      m_changingDelim = true;
      ui->txtDelimiter->setText(m_delim);
      m_changingDelim = false;
   }
   else
   {
      m_delim = ui->txtDelimiter->text();
   }

   // Some GUI elements only need to be visiable if input is hex.
   ui->lblHexType->setVisible(isHex);
   ui->cmbHexType->setVisible(isHex);

   parseInputData();
   updateGuiPlotCurveInfo();
}

void createPlotFromData::parseInputData()
{
   m_dataVectFloat.clear();

   bool isHex = isGuiHex();
   if(m_delim == "" || m_inData == "")
   {
      ui->txtNumPoints->setText(QString::number(m_dataVectFloat.size()));
      return;
   }

   QString localDelim = m_delim;
   QString localInData = m_inData;


   if(ui->chkLineEndAsDelim->isChecked())
   {
      localInData = localInData.replace("\r\n", localDelim).replace("\r", localDelim).replace("\n", localDelim); // Replace line endings with delimiter.
   }

   QString prunedData = QString::fromStdString( removeNonDelimiterChars(
             localInData.toStdString(),
             localDelim.toStdString(),
             isHex) );

   QStringList dataSplit = prunedData.split(localDelim, Qt::SkipEmptyParts);
   int i_numInValues = dataSplit.count();

   if(isHex == false)
   {
      for(int i_index = 0; i_index < i_numInValues; ++i_index)
      {
         double newEntry;
         bool success = dString::strTo(dataSplit[i_index].toStdString(), newEntry);
         if(success)
         {
            m_dataVectFloat.push_back(newEntry);
         }
      }
   }
   else
   {
      eHexType hexType = (eHexType)ui->cmbHexType->currentIndex();
      for(int i_index = 0; i_index < i_numInValues; ++i_index)
      {
         if(dataSplit[i_index] != "")
         {
            unsigned long long newEntryFromHex = strtoull(dataSplit[i_index].toStdString().c_str(), NULL, 16);
            switch(hexType)
            {
               case E_HEX_TYPE_UINT:
                  m_dataVectFloat.push_back(newEntryFromHex);
               break;
               case E_HEX_TYPE_SINT_8:
                  signed char tempS8;
                  memcpy(&tempS8, &newEntryFromHex, sizeof(tempS8));
                  m_dataVectFloat.push_back(tempS8);
               break;
               case E_HEX_TYPE_SINT_16:
                  signed short tempS16;
                  memcpy(&tempS16, &newEntryFromHex, sizeof(tempS16));
                  m_dataVectFloat.push_back(tempS16);
               break;
               case E_HEX_TYPE_SINT_32:
                  signed long tempS32;
                  memcpy(&tempS32, &newEntryFromHex, sizeof(tempS32));
                  m_dataVectFloat.push_back(tempS32);
               break;
               case E_HEX_TYPE_SINT_64:
                  signed long long tempS64;
                  memcpy(&tempS64, &newEntryFromHex, sizeof(tempS64));
                  m_dataVectFloat.push_back(tempS64);
               break;
               case E_HEX_TYPE_FLOAT_32:
                  float tempF32;
                  memcpy(&tempF32, &newEntryFromHex, sizeof(tempF32));
                  m_dataVectFloat.push_back(tempF32);
               break;
               case E_HEX_TYPE_FLOAT_64:
                  double tempF64;
                  memcpy(&tempF64, &newEntryFromHex, sizeof(tempF64));
                  m_dataVectFloat.push_back(tempF64);
               break;
            }

         }
      }
   }

   ui->txtNumPoints->setText(QString::number(m_dataVectFloat.size()));

}



void createPlotFromData::on_cmdCancel_clicked()
{
   m_curveCmdr->createPlotFromDataGuiClose();
}

void createPlotFromData::closeEvent(QCloseEvent* /*event*/)
{
   m_curveCmdr->createPlotFromDataGuiClose();
}

void createPlotFromData::on_cmdCreate_clicked()
{
   QString plotName = ui->cmbPlotName->currentText();
   QString curveName = ui->txtCurveName->text();
   bool validNewPlotCurveName = false;

   if(plotName == "" || curveName == "")
   {
      QMessageBox msgBox;
      msgBox.setWindowTitle("Invalid Plot/Curve Name");
      msgBox.setText("Provide a name for both the Plot and the Curve.");
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setDefaultButton(QMessageBox::Ok);
      msgBox.exec();
   }
   else if(m_curveCmdr->validCurve(plotName, curveName))
   {
      QMessageBox msgBox;
      msgBox.setWindowTitle("Curve Already Exists");
      msgBox.setText(plotName + PLOT_CURVE_SEP + curveName + " already exists. Choose another plot->curve name.");
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setDefaultButton(QMessageBox::Ok);
      msgBox.exec();
   }
   else
   {
      validNewPlotCurveName = true;
   }

   if(validNewPlotCurveName)
   {
      parseInputData();

      if(m_dataVectFloat.size() > 0)
      {
         bool doPlot = true;

         // Check for just plotting 1 point.
         if(m_dataVectFloat.size() == 1)
         {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Only 1 Point");
            msgBox.setText("There is only 1 point. Something may be wrong.\nDo you want to create a new plot with only 1 point?");
            msgBox.setStandardButtons(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);
            if(msgBox.exec() == QMessageBox::No)
               doPlot = false;
         }

         if(doPlot)
         {
            m_curveCmdr->create1dCurve(plotName, curveName, E_PLOT_TYPE_1D, m_dataVectFloat);
            m_curveCmdr->createPlotFromDataGuiClose();
         }

      }
      else
      {
         QMessageBox msgBox;
         msgBox.setWindowTitle("Nothing To Plot");
         msgBox.setText("Make sure the Raw Data and Delimiter are filled in correctly.");
         msgBox.setStandardButtons(QMessageBox::Ok);
         msgBox.setDefaultButton(QMessageBox::Ok);
         msgBox.exec();
      }
   }
}

void createPlotFromData::on_txtDataToPlot_editingFinished()
{
   handleNewData(NULL, true);
}

void createPlotFromData::on_txtDataToPlot_textChanged(const QString &arg1)
{
   (void)arg1; // Ignore unused warning.
   handleNewData(NULL, true);
}

void createPlotFromData::on_txtDataToPlot_textEdited(const QString &arg1)
{
   (void)arg1; // Ignore unused warning.
   handleNewData(NULL, true);
}

void createPlotFromData::on_txtDelimiter_textEdited(const QString &arg1)
{
   (void)arg1; // Ignore unused warning.
   if(m_changingDelim == false)
   {
      m_userSpecifiedDelim = true;
      m_delim = ui->txtDelimiter->text();
      handleNewData(NULL, true);
   }
}

void createPlotFromData::on_cmbBase_currentIndexChanged(int index)
{
   (void)index; // Ignore unused warning.
   if(m_changingBase == false)
   {
      m_userSpecifiedBase = true;
      handleNewData(NULL, true);
   }
}

void createPlotFromData::on_cmbHexType_currentIndexChanged(int index)
{
   double indexToWrite = index;
   persistentParam_setParam_f64(HEX_TYPE_PERSIST_PARAM_NAME, indexToWrite);
   handleNewData(NULL, true);
}

void createPlotFromData::on_chkAutoDelim_clicked()
{
   if(ui->chkAutoDelim->isChecked())
      m_userSpecifiedDelim = false;
   handleNewData(NULL, true);
}

void createPlotFromData::on_chkLineEndAsDelim_clicked()
{
   handleNewData(NULL, true);
}
