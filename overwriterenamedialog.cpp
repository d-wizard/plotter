/* Copyright 2014, 2020 Dan Williams. All Rights Reserved.
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
#include "overwriterenamedialog.h"
#include "ui_overwriterenamedialog.h"
#include "PlotHelperTypes.h"

overwriteRenameDialog::overwriteRenameDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::overwriteRenameDialog),
   m_return(CANCEL)
{
   ui->setupUi(this);
}

overwriteRenameDialog::~overwriteRenameDialog()
{
   delete ui;
}



ePlotExistsReturn overwriteRenameDialog::askUserAboutExistingPlot(QString& plotName, QString& curveName)
{
   ui->lblExistsMsg->setText(plotName + PLOT_CURVE_SEP + curveName + " exists. What would you like to do?");

   // Make sure the GUI pops up, in front of all the other GUIs.
   this->show();
   this->activateWindow();
   this->raise();

   // Get the user's button press on the GUI.
   this->exec();

   return m_return;
}

void overwriteRenameDialog::on_cmdOverwrite_clicked()
{
   m_return = OVERWRITE;
   this->accept(); // Return from exec()
}

void overwriteRenameDialog::on_cmdCancel_clicked()
{
   m_return = CANCEL;
   this->accept(); // Return from exec()
}

void overwriteRenameDialog::on_cmdRenameAuto_clicked()
{
   m_return = RENAME_AUTO;
   this->accept(); // Return from exec()
}

void overwriteRenameDialog::on_cmdRenameManual_clicked()
{
   m_return = RENAME_MANUAL;
   this->accept(); // Return from exec()
}
