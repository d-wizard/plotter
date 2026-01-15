/* Copyright 2024 - 2026 Dan Williams. All Rights Reserved.
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
#pragma once

#include <vector>
#include <QString>
#include <QDialog>
#include <QComboBox>
#include "CurveCommander.h"

namespace Ui {
class openRawDialog;
}

class openRawDialog : public QDialog
{
   Q_OBJECT
   
public:
   explicit openRawDialog(QWidget *parent = 0);
   ~openRawDialog();

   bool deterimineRawType(CurveCommander* curveCmdr, const QString& filePath, const QString& suggestedPlotName);

private slots:
   void on_buttonBox_accepted();

   void on_buttonBox_rejected();

   void on_cmbRawType_currentIndexChanged(int index);

   void on_chkSliceInput_stateChanged(int arg1);

   void on_radSliceBytes_clicked();

   void on_radSliceSamples_clicked();

   void on_spnSliceStart_valueChanged(double arg1);

   void on_spnSliceEnd_valueChanged(double arg1);

private:
   Ui::openRawDialog *ui;

   bool m_okSelected = false;
   bool m_cancelSelected = false;

   size_t m_curFileSizeBytes = 0;

   void setPlotComboBox(CurveCommander* curveCmdr, const QString& plotName);
   void setCurveNames();
   void setStatsLabel();
   bool isInterleaved();

   void plotTheFile(CurveCommander* curveCmdr, const QString& filePath);

   template <typename tRawFileType>
   void fillFromRaw(const std::vector<char>& inFile, dubVect& result, int dimension = 1, int offsetIndex = 0); 

   void setSliceVisible();

   // Helper functions
   unsigned getSampleSizeFromGui();
   void getSliceValueBytes(int64_t& bytesToRemoveFromFront, int64_t& totalBytes);

};
