
#include <QRandomGenerator>
#include <QPixmap>
#include <QImage>
#include <QScreen>
#include "curvesortcolordialog.h"
#include "ui_curvesortcolordialog.h"

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
   double widthInches = 2.0;
   double heightInches = 1.0;

   // Convert inches to pixels
   int widthPixels = static_cast<int>(widthInches * dpiX);
   int heightPixels = static_cast<int>(heightInches * dpiY);

   // Create image with computed pixel size
   QImage image(widthPixels, heightPixels, QImage::Format_RGB32);

   // Fill with random colors
   for (int y = 0; y < image.height(); ++y)
   {
       for (int x = 0; x < image.width(); ++x)
       {
           int r = QRandomGenerator::global()->bounded(256);
           int g = QRandomGenerator::global()->bounded(256);
           int b = QRandomGenerator::global()->bounded(256);
           image.setPixel(x, y, qRgb(r, g, b));
       }
   }

   ui->lblHue->setPixmap(QPixmap::fromImage(image));
   ui->lblHue->setFixedSize(image.size());
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
