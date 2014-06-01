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
#include "plotcurvenamedialog.h"
#include "ui_plotcurvenamedialog.h"

plotCurveNameDialog::plotCurveNameDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::plotCurveNameDialog),
   m_okSelected(false)
{
   ui->setupUi(this);
}

plotCurveNameDialog::~plotCurveNameDialog()
{
   delete ui;
}

QComboBox* plotCurveNameDialog::getPlotNameCombo()
{
   return ui->cmbPlotName;
}


bool plotCurveNameDialog::getPlotCurveNameFromUser(QString& plotName, QString& curveName)
{
   for(int i = 0; i < ui->cmbPlotName->count(); ++i)
   {
      if(ui->cmbPlotName->itemText(i) == plotName)
      {
         ui->cmbPlotName->setCurrentIndex(i);
         break;
      }
   }
   ui->txtCurveName->setText(curveName);

   // Show the Dialog. This function returns when OK, Cancel or Close (X) is pressed.
   this->exec();

   if(m_okSelected)
   {
      plotName = ui->cmbPlotName->currentText();
      curveName = ui->txtCurveName->text();
   }

   return m_okSelected;
}


void plotCurveNameDialog::on_buttonBox_accepted()
{
   m_okSelected = true;
}
