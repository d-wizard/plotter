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
#ifndef OVERWRITERENAMEDIALOG_H
#define OVERWRITERENAMEDIALOG_H

#include <QDialog>

namespace Ui {
class overwriteRenameDialog;
}

typedef enum
{
   OVERWRITE,
   RENAME,
   CANCEL
}ePlotExistsReturn;

class overwriteRenameDialog : public QDialog
{
   Q_OBJECT
   
public:
   explicit overwriteRenameDialog(QWidget *parent = 0);
   ~overwriteRenameDialog();
   
   ePlotExistsReturn askUserAboutExistingPlot(QString& plotName, QString& curveName);

private slots:
   void on_cmdOverwrite_clicked();

   void on_cmdRename_clicked();

   void on_cmdCancel_clicked();

private:
   Ui::overwriteRenameDialog *ui;

   ePlotExistsReturn m_return;
};

#endif // OVERWRITERENAMEDIALOG_H
