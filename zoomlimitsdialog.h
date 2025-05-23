/* Copyright 2022, 2025 Dan Williams. All Rights Reserved.
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
#ifndef ZOOMLIMITS_H
#define ZOOMLIMITS_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include "PlotZoom.h"

namespace Ui {
class zoomLimitsDialog;
}

class ZoomLimits
{
public:
   typedef enum
   {
      E_ZOOM_LIMIT__NONE,
      E_ZOOM_LIMIT__ABSOLUTE,
      E_ZOOM_LIMIT__FROM_MIN,
      E_ZOOM_LIMIT__FROM_MAX,
      E_ZOOM_LIMIT__INVALID
   }eZoomLimitType;

   typedef struct
   {
      eZoomLimitType limitType = E_ZOOM_LIMIT__NONE;
      double absMin = 0.0;
      double absMax = 0.0;
      double fromMinMaxVal = 0.0;
   }tZoomLimitInfo;

   ZoomLimits();
   virtual ~ZoomLimits();

   tZoomLimitInfo GetPlotLimits(eAxis axis);
   void SetPlotLimits(eAxis axis, const tZoomLimitInfo &limits);

   void ApplyLimits(maxMinXY &plotDimensions);

private:
   // Plot Dimension Limits
   tZoomLimitInfo m_widthLimit;
   tZoomLimitInfo m_heightLimit;
};

class zoomLimitsDialog : public QDialog
{
   Q_OBJECT
   
public:
   explicit zoomLimitsDialog(QWidget *parent = 0);
   ~zoomLimitsDialog();

   bool getZoomLimits(ZoomLimits* plotZoom);
   void cancel(){on_cmdCancel_clicked();}

private slots:
   void on_cmdApply_clicked();

   void on_cmdCancel_clicked();

   void on_cmbWidthTypes_currentIndexChanged(int index);

   void on_cmbHeightTypes_currentIndexChanged(int index);

private:
   Ui::zoomLimitsDialog *ui;
   bool m_applySelected = false;
   ZoomLimits* m_plotZoom;

   ZoomLimits::tZoomLimitInfo m_widthZoomInfo;
   ZoomLimits::tZoomLimitInfo m_heightZoomInfo;

   void fillAxisGui(eAxis axis);
   void fillAxisGui( ZoomLimits::tZoomLimitInfo& limits,
                     QComboBox* combo,
                     QLabel* label1,
                     QLineEdit* text1,
                     QLabel* label2,
                     QLineEdit* text2 );

   bool getValueFromTextBox(QLineEdit* textBox, double& value);
   bool fillStructFromGui(eAxis axis);
   bool fillStructFromGui( ZoomLimits::tZoomLimitInfo& limits,
                           QComboBox* combo,
                           QLineEdit* text1,
                           QLineEdit* text2 );
};

#endif // zoomLimitsDialog_H
