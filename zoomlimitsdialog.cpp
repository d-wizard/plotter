/* Copyright 2022, 2025 Dan Williams. All Rights Reserved.
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
#include "zoomlimitsdialog.h"
#include "ui_zoomlimitsdialog.h"
#include "dString.h"

ZoomLimits::ZoomLimits()
{
}

ZoomLimits::~ZoomLimits()
{
}

ZoomLimits::tZoomLimitInfo ZoomLimits::GetPlotLimits(eAxis axis)
{
   if(axis == E_Y_AXIS)
      return m_heightLimit;
   return m_widthLimit;
}

void ZoomLimits::SetPlotLimits(eAxis axis, const tZoomLimitInfo& limits)
{
   if(axis == E_Y_AXIS)
      m_heightLimit = limits;
   else
      m_widthLimit = limits;
}

void ZoomLimits::ApplyLimits(maxMinXY &plotDimensions)
{
   // Apply Limits to Width
   double xRange = plotDimensions.maxX - plotDimensions.minX;
   if(m_widthLimit.limitType == E_ZOOM_LIMIT__FROM_MAX && xRange > m_widthLimit.fromMinMaxVal)
   {
      plotDimensions.minX = plotDimensions.maxX - m_widthLimit.fromMinMaxVal;
   }
   else if(m_widthLimit.limitType == E_ZOOM_LIMIT__FROM_MIN && xRange > m_widthLimit.fromMinMaxVal)
   {
      plotDimensions.maxX = plotDimensions.minX + m_widthLimit.fromMinMaxVal;
   }

   // Apply Limits to Height
   double yRange = plotDimensions.maxY - plotDimensions.minY;
   if(m_heightLimit.limitType == E_ZOOM_LIMIT__FROM_MAX && yRange > m_heightLimit.fromMinMaxVal)
   {
      plotDimensions.minY = plotDimensions.maxY - m_heightLimit.fromMinMaxVal;
   }
   else if(m_heightLimit.limitType == E_ZOOM_LIMIT__FROM_MIN && yRange > m_heightLimit.fromMinMaxVal)
   {
      plotDimensions.maxY = plotDimensions.minY + m_heightLimit.fromMinMaxVal;
   }

   // Apply Absolute Limits
   if(m_widthLimit.limitType == E_ZOOM_LIMIT__ABSOLUTE)
   {
      plotDimensions.minX = m_widthLimit.absMin;
      plotDimensions.maxX = m_widthLimit.absMax;
   }
   if(m_heightLimit.limitType == E_ZOOM_LIMIT__ABSOLUTE)
   {
      plotDimensions.minY = m_heightLimit.absMin;
      plotDimensions.maxY = m_heightLimit.absMax;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

zoomLimitsDialog::zoomLimitsDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::zoomLimitsDialog)
{
   ui->setupUi(this);
   ui->cmbHeightTypes->addItem("None");
   ui->cmbHeightTypes->addItem("Absolute");
   ui->cmbHeightTypes->addItem("Limit From Min");
   ui->cmbHeightTypes->addItem("Limit From Max");
   ui->cmbWidthTypes->addItem("None");
   ui->cmbWidthTypes->addItem("Absolute");
   ui->cmbWidthTypes->addItem("Limit From Min");
   ui->cmbWidthTypes->addItem("Limit From Max");
}

zoomLimitsDialog::~zoomLimitsDialog()
{
   delete ui;
}

bool zoomLimitsDialog::getZoomLimits(ZoomLimits* plotZoom)
{
   m_plotZoom = plotZoom;

   m_widthZoomInfo  = m_plotZoom->GetPlotLimits(E_X_AXIS);
   m_heightZoomInfo = m_plotZoom->GetPlotLimits(E_Y_AXIS);

   fillAxisGui(E_X_AXIS);
   fillAxisGui(E_Y_AXIS);

   // Show the Dialog. This function returns when OK, Cancel or Close (X) is pressed.
   this->exec();

   if(m_applySelected)
   {
      m_plotZoom->SetPlotLimits(E_X_AXIS, m_widthZoomInfo);
      m_plotZoom->SetPlotLimits(E_Y_AXIS, m_heightZoomInfo);
   }

   return m_applySelected;
}

void zoomLimitsDialog::on_cmdApply_clicked()
{
   bool success = true;
   success = fillStructFromGui(E_X_AXIS) && success;
   success = fillStructFromGui(E_Y_AXIS) && success;

   if(success)
   {
      m_applySelected = true;
      this->accept(); // Return from exec()
   }
   else
   {
      QMessageBox::critical(this, "Invalid Settings", "Something is wrong with the settings. Double check and try again.", QMessageBox::Ok);
   }
}

void zoomLimitsDialog::on_cmdCancel_clicked()
{
   m_applySelected = false;
   this->accept(); // Return from exec()
}

void zoomLimitsDialog::on_cmbWidthTypes_currentIndexChanged(int index)
{
   m_widthZoomInfo.limitType = static_cast<ZoomLimits::eZoomLimitType>(index);
   fillAxisGui(E_X_AXIS);
}

void zoomLimitsDialog::on_cmbHeightTypes_currentIndexChanged(int index)
{
   m_heightZoomInfo.limitType = static_cast<ZoomLimits::eZoomLimitType>(index);
   fillAxisGui(E_Y_AXIS);
}


void zoomLimitsDialog::fillAxisGui(eAxis axis)
{
   if(axis == E_Y_AXIS)
      fillAxisGui(m_heightZoomInfo, ui->cmbHeightTypes, ui->lblY1st, ui->txtY1st, ui->lblY2nd, ui->txtY2nd);
   else
      fillAxisGui(m_widthZoomInfo,  ui->cmbWidthTypes,  ui->lblX1st, ui->txtX1st, ui->lblX2nd, ui->txtX2nd);

}

void zoomLimitsDialog::fillAxisGui( ZoomLimits::tZoomLimitInfo& limits,
                                    QComboBox* combo,
                                    QLabel* label1,
                                    QLineEdit* text1,
                                    QLabel* label2,
                                    QLineEdit* text2)
{
   if(limits.limitType >= 0 && limits.limitType < ZoomLimits::E_ZOOM_LIMIT__INVALID)
   {
      combo->setCurrentIndex(limits.limitType);
   }

   switch(limits.limitType)
   {
      default:
      case ZoomLimits::E_ZOOM_LIMIT__NONE:
         label1->setVisible(false);
         label2->setVisible(false);
         text1->setVisible(false);
         text2->setVisible(false);
      break;
      case ZoomLimits::E_ZOOM_LIMIT__ABSOLUTE:
         label1->setVisible(true);
         label2->setVisible(true);
         text1->setVisible(true);
         text2->setVisible(true);

         label1->setText("Min");
         label2->setText("Max");
         text1->setText(QString::number(limits.absMin));
         text2->setText(QString::number(limits.absMax));
      break;
      case ZoomLimits::E_ZOOM_LIMIT__FROM_MIN:
         label1->setVisible(false);
         label2->setVisible(false);
         text1->setVisible(true);
         text2->setVisible(false);

         text1->setText(QString::number(limits.fromMinMaxVal));
      break;
      case ZoomLimits::E_ZOOM_LIMIT__FROM_MAX:
         label1->setVisible(false);
         label2->setVisible(false);
         text1->setVisible(true);
         text2->setVisible(false);

         text1->setText(QString::number(limits.fromMinMaxVal));
      break;
   }
}

bool zoomLimitsDialog::getValueFromTextBox(QLineEdit* textBox, double& value)
{
   std::string txt = textBox->text().toStdString();
   return dString::strTo(txt, value);
}

bool zoomLimitsDialog::fillStructFromGui(eAxis axis)
{
   bool success = false;
   if(axis == E_Y_AXIS)
      success = fillStructFromGui(m_heightZoomInfo, ui->cmbHeightTypes, ui->txtY1st, ui->txtY2nd);
   else
      success = fillStructFromGui(m_widthZoomInfo,  ui->cmbWidthTypes,  ui->txtX1st, ui->txtX2nd);
   return success;
}

bool zoomLimitsDialog::fillStructFromGui( ZoomLimits::tZoomLimitInfo& limits,
                                          QComboBox* combo,
                                          QLineEdit* text1,
                                          QLineEdit* text2 )
{
   if(limits.limitType >= 0 && limits.limitType < ZoomLimits::E_ZOOM_LIMIT__INVALID)
      combo->setCurrentIndex(limits.limitType);

   bool success = true; // true until proven otherwise.
   double textToNumber;
   switch(limits.limitType)
   {
      default:
      case ZoomLimits::E_ZOOM_LIMIT__NONE:
         // Nothing to do.
      break;
      case ZoomLimits::E_ZOOM_LIMIT__ABSOLUTE:
         if(getValueFromTextBox(text1, textToNumber))
         {
            limits.absMin = textToNumber;
         }
         else
         {
            success = false;
         }
         if(getValueFromTextBox(text2, textToNumber))
         {
            limits.absMax = textToNumber;
         }
         else
         {
            success = false;
         }
         if(limits.absMin >= limits.absMax)
         {
            success = false;
         }
      break;
      case ZoomLimits::E_ZOOM_LIMIT__FROM_MIN:
         if(getValueFromTextBox(text1, textToNumber))
         {
            limits.fromMinMaxVal = textToNumber;
         }
         else
         {
            success = false;
         }
         if(limits.fromMinMaxVal <= 0)
         {
            success = false;
         }
      break;
      case ZoomLimits::E_ZOOM_LIMIT__FROM_MAX:
         if(getValueFromTextBox(text1, textToNumber))
         {
            limits.fromMinMaxVal = textToNumber;
         }
         else
         {
            success = false;
         }
         if(limits.fromMinMaxVal <= 0)
         {
            success = false;
         }
      break;
   }

   return success;
}

