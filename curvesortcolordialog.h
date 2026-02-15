#ifndef CURVESORTCOLORDIALOG_H
#define CURVESORTCOLORDIALOG_H

#include <QDialog>

namespace Ui {
class curveSortColorDialog;
}

class curveSortColorDialog : public QDialog
{
   Q_OBJECT

public:
   explicit curveSortColorDialog(QWidget *parent = nullptr);
   ~curveSortColorDialog();

private slots:
   void on_buttonBox_accepted();

   void on_buttonBox_rejected();

private:
   Ui::curveSortColorDialog *ui;
};

#endif // CURVESORTCOLORDIALOG_H
