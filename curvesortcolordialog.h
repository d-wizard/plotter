/* Copyright 2026 Dan Williams. All Rights Reserved.
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
   static constexpr float RED_HUE = 0.0;
   static constexpr float BLUE_HUE = 0.7;
   static constexpr float HUE_MOD = 2.0;

   typedef struct tCurveSortColor
   {
      // Sort
      bool sort = false;
      bool sortAscending = false;

      // Color
      bool setColor = false;
      float hueStart = RED_HUE;
      float hueEnd = BLUE_HUE;
      float hueMod = HUE_MOD;
   }tCurveSortColor;
   
public:
   curveSortColorDialog(const tCurveSortColor& start, QWidget *parent);
   explicit curveSortColorDialog(QWidget *parent);
   ~curveSortColorDialog();

   bool getResult(tCurveSortColor& results);
   void cancel(){on_buttonBox_rejected();}

private slots:
   void on_buttonBox_accepted();

   void on_buttonBox_rejected();

   void on_slideHueStart_valueChanged(int value);

   void on_slideHueEnd_valueChanged(int value);

   void on_slideHueMod_valueChanged(int value);

   void on_chkHueRev_stateChanged(int arg1);

   void on_radColorDefault_clicked();

   void on_radColorRedToBlue_clicked();

   void on_radColorBlueToRed_clicked();

   void on_radColorManual_clicked();

private:
   Ui::curveSortColorDialog *ui;

   int m_hueWidthPixels = 0;
   int m_hueHeightPixels = 0;

   void setHueFromGui();
   void setHueImage(float startHue, float stopHue, float modHue);

   bool m_applySelected = false;
   tCurveSortColor m_result;
};

#endif // CURVESORTCOLORDIALOG_H
