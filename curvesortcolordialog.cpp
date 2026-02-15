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
#include "curvesortcolordialog.h"
#include "ui_curvesortcolordialog.h"
#include "hsvrgb.h"

curveSortColorDialog::curveSortColorDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::curveSortColorDialog)
{
   ui->setupUi(this);

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

   setHueImage(0.7, 0.0);
}

curveSortColorDialog::~curveSortColorDialog()
{
   delete ui;
}

void curveSortColorDialog::on_buttonBox_accepted()
{

}

void curveSortColorDialog::on_buttonBox_rejected()
{

}

void curveSortColorDialog::setHueImage(float startHue, float stopHue)
{
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
         hueVal = std::max(std::min(hueVal, 1.0f), 0.0f);

         hsv.h = (unsigned char)(hueVal * 255.0 + 0.5);
         auto rgb = HsvToRgb(hsv, true);
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

