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
#include <algorithm>
#include <QPixmap>
#include <QImage>
#include <QScreen>
#include <math.h>
#include "curvesortcolordialog.h"
#include "ui_curvesortcolordialog.h"
#include "hsvrgb.h"


curveSortColorDialog::curveSortColorDialog(const tCurveSortColor& start, QWidget *parent) :
   QDialog(parent),
   ui(new Ui::curveSortColorDialog)
{
   ui->setupUi(this);

   // Set GUI elements from the starting settings.
   switch(start.sort)
   {
      // Fall through is intended.
      default:
      case eSortType::NO_CHANGE:
         ui->radSortNone->setChecked(true);
      break;
      case eSortType::ASCENDING:
         ui->radSortAscending->setChecked(true);
      break;
      case eSortType::DESCENDING:
         ui->radSortDescending->setChecked(true);
      break;
   }
   switch(start.color)
   {
      // Fall through is intended.
      default:
      case eColorType::NO_CHANGE:
      break;
      case eColorType::DEFAULT:
         ui->radColorDefault->setChecked(true);
      break;
      case eColorType::RED_TO_BLUE:
         ui->radColorRedToBlue->setChecked(true);
      break;
      case eColorType::BLUE_TO_RED:
         ui->radColorBlueToRed->setChecked(true);
      break;
      case eColorType::CUSTOM:
      {
         ui->radColorManual->setChecked(true);
         ui->slideHueStart->setValue(start.hueStart * ui->slideHueStart->maximum() / 2);
         ui->slideHueEnd->setValue(start.hueEnd * ui->slideHueEnd->maximum() / 2);
         ui->slideHueMod->setValue((start.hueMod - 0.01) * ui->slideHueMod->maximum() / 10);
      }
      break;
   }

   // Get screen DPI (logical DPI respects OS scaling)
   QScreen *screen = QGuiApplication::primaryScreen();
   double dpiX = screen->logicalDotsPerInchX();
   double dpiY = screen->logicalDotsPerInchY();

   // Desired physical size
   double widthInches = 4.0;
   double heightInches = 0.25;

   // Convert inches to pixels
   m_hueWidthPixels = static_cast<int>(widthInches * dpiX + 0.5);
   m_hueHeightPixels = static_cast<int>(heightInches * dpiY + 0.5);

   setHueFromGui();
}

curveSortColorDialog::curveSortColorDialog(QWidget *parent) :
   curveSortColorDialog(tCurveSortColor(), parent)
{
}

curveSortColorDialog::~curveSortColorDialog()
{
   delete ui;
}

void curveSortColorDialog::on_buttonBox_accepted()
{
   m_applySelected = true;
   this->accept(); // Return from exec()
}

void curveSortColorDialog::on_buttonBox_rejected()
{
   m_applySelected = false;
   this->accept(); // Return from exec()
}

bool curveSortColorDialog::getResult(tCurveSortColor& results)
{
   results = m_result;

   if(ui->radSortNone->isChecked())
      results.sort = eSortType::NO_CHANGE;
   else if(ui->radSortAscending->isChecked())
      results.sort = eSortType::ASCENDING;
   else
      results.sort = eSortType::DESCENDING;

   if(ui->radColorDontChange->isChecked())
      results.color = eColorType::NO_CHANGE;
   else if(ui->radColorDefault->isChecked())
      results.color = eColorType::DEFAULT;
   else if(ui->radColorRedToBlue->isChecked())
      results.color = eColorType::RED_TO_BLUE;
   else if(ui->radColorBlueToRed->isChecked())
      results.color = eColorType::BLUE_TO_RED;
   else
      results.color = eColorType::CUSTOM;

   return m_applySelected;
}

void curveSortColorDialog::on_slideHueStart_valueChanged(int /*value*/)
{
   setHueFromGui();
}

void curveSortColorDialog::on_slideHueEnd_valueChanged(int /*value*/)
{
   setHueFromGui();
}

void curveSortColorDialog::on_slideHueMod_valueChanged(int /*value*/)
{
   setHueFromGui();
}

void curveSortColorDialog::on_chkHueRev_stateChanged(int /*arg1*/)
{
   setHueFromGui();
}

void curveSortColorDialog::on_radColorDontChange_clicked()
{
   setHueFromGui();
}

void curveSortColorDialog::on_radColorDefault_clicked()
{
   setHueFromGui();
}

void curveSortColorDialog::on_radColorRedToBlue_clicked()
{
   setHueFromGui();
}

void curveSortColorDialog::on_radColorBlueToRed_clicked()
{
   setHueFromGui();
}

void curveSortColorDialog::on_radColorManual_clicked()
{
   setHueFromGui();
}

void curveSortColorDialog::setHueFromGui()
{
   bool showHue = !ui->radColorDefault->isChecked() && !ui->radColorDontChange->isChecked();
   bool showHueControls = ui->radColorManual->isChecked();
   if(ui->radColorRedToBlue->isChecked())
   {
      setHueImage(RED_HUE, BLUE_HUE, HUE_MOD);
   }
   else if(ui->radColorBlueToRed->isChecked())
   {
      setHueImage(BLUE_HUE, RED_HUE, HUE_MOD);
   }
   else if(ui->radColorManual->isChecked())
   {
      float start = float(ui->slideHueStart->value()*2) / float(ui->slideHueStart->maximum());
      float end   = float(ui->slideHueEnd->value()*2) / float(ui->slideHueEnd->maximum());
      float mod   = float(ui->slideHueMod->value()*10) / float(ui->slideHueMod->maximum()) + 0.01;

      if(ui->chkHueRev->isChecked())
         setHueImage(end, start, mod);
      else
         setHueImage(start, end, mod);
   }

   ui->grpHue->setVisible(showHue);
   ui->lblHue->setVisible(showHue);
   ui->lblHueStart->setVisible(showHueControls);
   ui->lblHueEnd->setVisible(showHueControls);
   ui->lblHueMod->setVisible(showHueControls);
   ui->lineHue1->setVisible(showHueControls);
   ui->lineHue2->setVisible(showHueControls);
   ui->chkHueRev->setVisible(showHueControls);
   ui->slideHueStart->setVisible(showHueControls);
   ui->slideHueEnd->setVisible(showHueControls);
   ui->slideHueMod->setVisible(showHueControls);
}

void curveSortColorDialog::setHueImage(float startHue, float stopHue, float modHue)
{
   // Save off the hue values.
   m_result.hueStart = startHue;
   m_result.hueEnd = stopHue;
   m_result.hueMod = modHue;

   // Create image with computed pixel size
   QImage image(m_hueWidthPixels, m_hueHeightPixels, QImage::Format_RGB32);

   // Fill with HSV Scale
   HsvColor hsv = {0, 200, 255}; // Init here, hue will change.
   if(m_hueWidthPixels > 0 && m_hueHeightPixels > 0)
   {
      for (int x = 0; x < m_hueWidthPixels; ++x)
      {
         // Determine the hue value.
         float percent = float(x) / float(m_hueWidthPixels-1);
         float hueVal = startHue + (stopHue - startHue) * percent;
         hueVal = std::max(hueVal, 0.0f); // Don't be less than zero
         hueVal = std::fmod(hueVal, 1.0); // Wrap back around when greater than 1

         hsv.h = (unsigned char)(hueVal * 255.0 + 0.5);
         auto rgb = HsvToRgb(hsv, modHue);
         auto qtRgb = qRgb(rgb.r, rgb.g, rgb.b);
         for (int y = 0; y < m_hueHeightPixels; ++y)
         {
            image.setPixel(x, y, qtRgb);
         }
      }

      ui->lblHue->setPixmap(QPixmap::fromImage(image));
      ui->lblHue->setFixedSize(image.size());
   }
}

