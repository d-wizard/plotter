#ifndef SETSAMPLERATEDIALOG_H
#define SETSAMPLERATEDIALOG_H

#include <QDialog>

namespace Ui {
class setsampleratedialog;
}

class CurveCommander; // Forward declare.

class setsampleratedialog : public QDialog
{
   Q_OBJECT

public:
   setsampleratedialog(CurveCommander* curveCmdr, const QString& plotName, QWidget *parent = nullptr);
   ~setsampleratedialog();

private slots:
   void on_cmdQuery_clicked();

private:
   Ui::setsampleratedialog *ui;

   CurveCommander* m_curveCmdr;
   QString m_plotName;
   int m_prevCurveIndex = 0;
};

#endif // SETSAMPLERATEDIALOG_H
