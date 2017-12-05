/* Copyright 2017 Dan Williams. All Rights Reserved.
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
#ifndef CREATEPLOTFROMDATA_H
#define CREATEPLOTFROMDATA_H

#include <QDialog>
#include <string>
#include <string.h>
#include <QString>
#include <QVector>
#include "CurveCommander.h"
#include "PlotHelperTypes.h"


namespace Ui {
class createPlotFromData;
}

class createPlotFromData : public QDialog
{
   Q_OBJECT
   
public:
   explicit createPlotFromData(CurveCommander *curveCmdr, QString plotName, const char *data, QWidget *parent = 0);
   ~createPlotFromData();

   void updateGuiPlotCurveInfo();

   void handleNewData(const char *data, bool newDataIsFromGui = false);

private slots:
   void on_cmdCancel_clicked();

   void on_cmdCreate_clicked();

   void on_txtDataToPlot_editingFinished();

   void on_txtDataToPlot_textChanged(const QString &arg1);

   void on_txtDataToPlot_textEdited(const QString &arg1);

   void on_txtDelimiter_textEdited(const QString &arg1);

   void on_cmbBase_currentIndexChanged(int index);

   void on_cmbHexType_currentIndexChanged(int index);

private:
   typedef enum
   {
      E_HEX_TYPE_UINT,
      E_HEX_TYPE_SINT_8,
      E_HEX_TYPE_SINT_16,
      E_HEX_TYPE_SINT_32,
      E_HEX_TYPE_SINT_64,
      E_HEX_TYPE_FLOAT_32,
      E_HEX_TYPE_FLOAT_64
   }eHexType;

   Ui::createPlotFromData *ui;

   CurveCommander* m_curveCmdr;

   QString m_inData;
   QString m_delim;
   QVector<long long> m_dataVectInt;
   dubVect m_dataVectFloat;

   bool m_changingDelim;
   bool m_userSpecifiedDelim;

   bool m_changingBase;
   bool m_userSpecifiedBase;

   bool isGuiHex();

   void closeEvent(QCloseEvent* event);

   void parseInputData();

   void fillInValidPlotCurve();

};

#endif // CREATEPLOTFROMDATA_H
