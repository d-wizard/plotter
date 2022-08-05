/* Copyright 2022 Dan Williams. All Rights Reserved.
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
#include "zoomlimitsdialog.h"
#include "ui_zoomlimitsdialog.h"

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

bool zoomLimitsDialog::getZoomLimits(PlotZoom* plotZoom)
{
   m_plotZoom = plotZoom;

   m_widthZoomInfo  = m_plotZoom->GetPlotLimits(E_X_AXIS);
   m_heightZoomInfo = m_plotZoom->GetPlotLimits(E_Y_AXIS);

   fillAxisGui(E_X_AXIS);
   fillAxisGui(E_Y_AXIS);

   // Show the Dialog. This function returns when OK, Cancel or Close (X) is pressed.
   this->exec();

   return m_applySelected;
}

void zoomLimitsDialog::on_cmdApply_clicked()
{
   m_applySelected = true;
   this->accept(); // Return from exec()
}

void zoomLimitsDialog::on_cmdCancel_clicked()
{
   m_applySelected = false;
   this->accept(); // Return from exec()
}

void zoomLimitsDialog::on_cmbWidthTypes_currentIndexChanged(int index)
{
   m_widthZoomInfo.limitType = static_cast<PlotZoom::eZoomLimitType>(index);
   fillAxisGui(E_X_AXIS);
}

void zoomLimitsDialog::on_cmbHeightTypes_currentIndexChanged(int index)
{
   m_heightZoomInfo.limitType = static_cast<PlotZoom::eZoomLimitType>(index);
   fillAxisGui(E_Y_AXIS);
}


void zoomLimitsDialog::fillAxisGui(eAxis axis)
{
   if(axis == E_Y_AXIS)
      fillAxisGui(m_heightZoomInfo, ui->cmbHeightTypes, ui->lblY1st, ui->txtY1st, ui->lblY2nd, ui->txtY2nd);
   else
      fillAxisGui(m_widthZoomInfo,  ui->cmbWidthTypes,  ui->lblX1st, ui->txtX1st, ui->lblX2nd, ui->txtX2nd);

}

void zoomLimitsDialog::fillAxisGui( PlotZoom::tZoomLimitInfo& limits,
                                    QComboBox* combo,
                                    QLabel* label1,
                                    QLineEdit* text1,
                                    QLabel* label2,
                                    QLineEdit* text2)
{
   if(limits.limitType >= 0 && limits.limitType < PlotZoom::E_ZOOM_LIMIT__INVALID)
   {
      combo->setCurrentIndex(limits.limitType);
   }

   switch(limits.limitType)
   {
      default:
      case PlotZoom::E_ZOOM_LIMIT__NONE:
         label1->setVisible(false);
         label2->setVisible(false);
         text1->setVisible(false);
         text2->setVisible(false);
      break;
      case PlotZoom::E_ZOOM_LIMIT__ABSOLUTE:
         label1->setVisible(true);
         label2->setVisible(true);
         text1->setVisible(true);
         text2->setVisible(true);

         label1->setText("Min");
         label2->setText("Max");
         text1->setText(QString::number(limits.absMin));
         text2->setText(QString::number(limits.absMax));
      break;
      case PlotZoom::E_ZOOM_LIMIT__FROM_MIN:
         label1->setVisible(false);
         label2->setVisible(false);
         text1->setVisible(true);
         text2->setVisible(false);

         text1->setText(QString::number(limits.fromMinMaxVal));
      break;
      case PlotZoom::E_ZOOM_LIMIT__FROM_MAX:
         label1->setVisible(false);
         label2->setVisible(false);
         text1->setVisible(true);
         text2->setVisible(false);

         text1->setText(QString::number(limits.fromMinMaxVal));
      break;
   }

}
