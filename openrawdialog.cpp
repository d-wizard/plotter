/* Copyright 2024 Dan Williams. All Rights Reserved.
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
#include <assert.h>
#include "openrawdialog.h"
#include "ui_openrawdialog.h"
#include "localPlotCreate.h"
#include "FileSystemOperations.h"

typedef enum
{
   E_RAW_TYPE_SIGNED_INT_8,
   E_RAW_TYPE_SIGNED_INT_16,
   E_RAW_TYPE_SIGNED_INT_32,
   E_RAW_TYPE_SIGNED_INT_64,
   E_RAW_TYPE_UNSIGNED_INT_8,
   E_RAW_TYPE_UNSIGNED_INT_16,
   E_RAW_TYPE_UNSIGNED_INT_32,
   E_RAW_TYPE_UNSIGNED_INT_64,
   E_RAW_TYPE_FLOAT_32,
   E_RAW_TYPE_FLOAT_64,
   E_RAW_TYPE_INTERLEAVED_SIGNED_INT_8,
   E_RAW_TYPE_INTERLEAVED_SIGNED_INT_16,
   E_RAW_TYPE_INTERLEAVED_SIGNED_INT_32,
   E_RAW_TYPE_INTERLEAVED_SIGNED_INT_64,
   E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_8,
   E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_16,
   E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_32,
   E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_64,
   E_RAW_TYPE_INTERLEAVED_FLOAT_32,
   E_RAW_TYPE_INTERLEAVED_FLOAT_64
}eRawTypes;

static const QString RAW_TYPE_DROPDOWN[] =
{
   "Signed Int 8 Bit",
   "Signed Int 16 Bit",
   "Signed Int 32 Bit",
   "Signed Int 64 Bit",
   "Unsigned Int 8 Bit",
   "Unsigned Int 16 Bit",
   "Unsigned Int 32 Bit",
   "Unsigned Int 64 Bit",
   "Float 32 Bit",
   "Float 64 Bit",
   "Interleaved Signed Int 8 Bit",
   "Interleaved Signed Int 16 Bit",
   "Interleaved Signed Int 32 Bit",
   "Interleaved Signed Int 64 Bit",
   "Interleaved Unsigned Int 8 Bit",
   "Interleaved Unsigned Int 16 Bit",
   "Interleaved Unsigned Int 32 Bit",
   "Interleaved Unsigned Int 64 Bit",
   "Interleaved Float 32 Bit",
   "Interleaved Float 64 Bit"
};

openRawDialog::openRawDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::openRawDialog)
{
   ui->setupUi(this);

   for(unsigned int i = 0; i < ARRAY_SIZE(RAW_TYPE_DROPDOWN); ++i)
   {
      ui->cmbRawType->addItem(RAW_TYPE_DROPDOWN[i]);
   }
}

openRawDialog::~openRawDialog()
{
   delete ui;
}

bool openRawDialog::deterimineRawType(CurveCommander* curveCmdr, const QString& filePath, const QString& suggestedPlotName)
{
   // Fill in plot name combo box
   setPlotComboBox(curveCmdr, suggestedPlotName);

   // Fill in the curve names.
   setCurveNames();

   // Show the Dialog. This function returns when OK, Cancel or Close (X) is pressed.
   bool done = false;
   bool openTheFile = false;
   QString plotName, curveName1, curveName2;
   while(!done)
   {
      m_okSelected = false;
      this->exec();

      // Grab values from UI.
      plotName = ui->cmbPlotName->currentText();
      curveName1 = ui->txtCurveName->text();
      curveName2 = ui->txtCurveName2->text();

      if(m_okSelected)
      {
         // User clicked OK, make sure inputs make sense.
         bool validateCurve2 = isInterleaved();
         bool curve1Valid = localPlotCreate::validateNewPlotCurveName(curveCmdr, plotName, curveName1);
         bool curve2Valid = !validateCurve2 || localPlotCreate::validateNewPlotCurveName(curveCmdr, plotName, curveName2);

         if(curve1Valid && curve2Valid)
         {
            done = true;
            openTheFile = true;
         }

         // Update the GUI with any changes made. The GUI values are what will be used if plots do get generated (i.e. if openTheFile == true)
         setPlotComboBox(curveCmdr, plotName);
         ui->txtCurveName->setText(curveName1);
         ui->txtCurveName2->setText(curveName2);
      }
      else
      {
         done = true; // User clicked Cancel or Close (X), exit loop now.
      }
   }

   if(openTheFile)
   {
      plotTheFile(curveCmdr, filePath);
   }

   return openTheFile;
}

void openRawDialog::on_buttonBox_accepted()
{
   m_okSelected = true;
}

void openRawDialog::on_buttonBox_rejected()
{
   m_cancelSelected = true;
}

void openRawDialog::setPlotComboBox(CurveCommander* curveCmdr, const QString& plotName)
{
   tCurveCommanderInfo allCurves = curveCmdr->getCurveCommanderInfo();
   int matchingIndex = -1;
   int plotNameIndex = 0;
   foreach( QString curPlotName, allCurves.keys() )
   {
      ui->cmbPlotName->addItem(curPlotName);
      if(curPlotName == plotName)
         matchingIndex = plotNameIndex;
      ++plotNameIndex;
   }
   if(matchingIndex >= 0)
      ui->cmbPlotName->setCurrentIndex(matchingIndex);
   else
      ui->cmbPlotName->setCurrentText(plotName);

}

void openRawDialog::setCurveNames()
{
   QString curve1 = "";
   QString curve2 = "";
   bool showCurve2 = isInterleaved();
   if(showCurve2)
   {
      curve1 = "real";
      curve2 = "imag";
   }
   else
   {
      curve1 = "CurveName";
   }

   // Only set if unitialized
   if((ui->txtCurveName->text() == "" || ui->txtCurveName->text() == "real" || ui->txtCurveName->text() == "CurveName"))
      ui->txtCurveName->setText(curve1);
   // Only set if unitialized
   if(ui->txtCurveName2->text() == "" && showCurve2)
      ui->txtCurveName2->setText(curve2);

   ui->lblCurveName2->setVisible(showCurve2);
   ui->txtCurveName2->setVisible(showCurve2);
}

bool openRawDialog::isInterleaved()
{
   bool retVal = false;
   switch((eRawTypes)ui->cmbRawType->currentIndex())
   {
      case E_RAW_TYPE_INTERLEAVED_SIGNED_INT_8:
      case E_RAW_TYPE_INTERLEAVED_SIGNED_INT_16:
      case E_RAW_TYPE_INTERLEAVED_SIGNED_INT_32:
      case E_RAW_TYPE_INTERLEAVED_SIGNED_INT_64:
      case E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_8:
      case E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_16:
      case E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_32:
      case E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_64:
      case E_RAW_TYPE_INTERLEAVED_FLOAT_32:
      case E_RAW_TYPE_INTERLEAVED_FLOAT_64:
         retVal = true;
      break;
      default:
         retVal = false;
      break;
   }
   return retVal;
}


void openRawDialog::on_cmbRawType_currentIndexChanged(int /*index*/)
{
   setCurveNames();
}

void openRawDialog::plotTheFile(CurveCommander* curveCmdr, const QString& filePath)
{
   QString plotName = ui->cmbPlotName->currentText();
   QString curveName1 = ui->txtCurveName->text();
   QString curveName2 = ui->txtCurveName2->text();

   std::vector<char> inputFileBytes;
   fso::ReadBinaryFile(filePath.toStdString(), inputFileBytes);
   dubVect curveValues1;
   dubVect curveValues2;

   auto openRawType = (eRawTypes)ui->cmbRawType->currentIndex();
   switch(openRawType)
   {
      case E_RAW_TYPE_SIGNED_INT_8:    { fillFromRaw<int8_t  >(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_SIGNED_INT_16:   { fillFromRaw<int16_t >(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_SIGNED_INT_32:   { fillFromRaw<int32_t >(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_SIGNED_INT_64:   { fillFromRaw<int64_t >(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_UNSIGNED_INT_8:  { fillFromRaw<uint8_t >(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_UNSIGNED_INT_16: { fillFromRaw<uint16_t>(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_UNSIGNED_INT_32: { fillFromRaw<uint32_t>(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_UNSIGNED_INT_64: { fillFromRaw<uint64_t>(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_FLOAT_32:        { fillFromRaw<float   >(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_FLOAT_64:        { fillFromRaw<double  >(inputFileBytes, curveValues1); } break;
      case E_RAW_TYPE_INTERLEAVED_SIGNED_INT_8:    { fillFromRaw<int8_t  >(inputFileBytes, curveValues1, 2, 0); fillFromRaw<int8_t  >(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_SIGNED_INT_16:   { fillFromRaw<int16_t >(inputFileBytes, curveValues1, 2, 0); fillFromRaw<int16_t >(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_SIGNED_INT_32:   { fillFromRaw<int32_t >(inputFileBytes, curveValues1, 2, 0); fillFromRaw<int32_t >(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_SIGNED_INT_64:   { fillFromRaw<int64_t >(inputFileBytes, curveValues1, 2, 0); fillFromRaw<int64_t >(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_8:  { fillFromRaw<uint8_t >(inputFileBytes, curveValues1, 2, 0); fillFromRaw<uint8_t >(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_16: { fillFromRaw<uint16_t>(inputFileBytes, curveValues1, 2, 0); fillFromRaw<uint16_t>(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_32: { fillFromRaw<uint32_t>(inputFileBytes, curveValues1, 2, 0); fillFromRaw<uint32_t>(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_64: { fillFromRaw<uint64_t>(inputFileBytes, curveValues1, 2, 0); fillFromRaw<uint64_t>(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_FLOAT_32:        { fillFromRaw<float   >(inputFileBytes, curveValues1, 2, 0); fillFromRaw<float   >(inputFileBytes, curveValues2, 2, 1);} break;
      case E_RAW_TYPE_INTERLEAVED_FLOAT_64:        { fillFromRaw<double  >(inputFileBytes, curveValues1, 2, 0); fillFromRaw<double  >(inputFileBytes, curveValues2, 2, 1);} break;
   }

   // Finally, this will actually create the plots.
   if(curveValues1.size() > 0)
      curveCmdr->create1dCurve(plotName, curveName1, E_PLOT_TYPE_1D, curveValues1);
   if(curveValues2.size() > 0)
      curveCmdr->create1dCurve(plotName, curveName2, E_PLOT_TYPE_1D, curveValues2);

}

template <typename tRawFileType>
void openRawDialog::fillFromRaw(const std::vector<char>& inFile, dubVect& result, int dimension, int offsetIndex)
{
   assert(dimension > 0 && dimension < 3);
   assert(offsetIndex < dimension);

   // Determine some sizes.
   constexpr size_t RAW_TYPE_SIZE = sizeof(tRawFileType);
   const size_t inFileSizeBytes = inFile.size();
   const size_t blockSizeBytes = RAW_TYPE_SIZE * dimension;
   const size_t offsetBytes = RAW_TYPE_SIZE * offsetIndex;
   const char* inFilePtr = inFile.data();

   size_t numLoops = inFileSizeBytes / blockSizeBytes; // round down.
   result.resize(numLoops);
   tRawFileType rawVal;
   for(size_t i = 0; i < numLoops; ++i)
   {
      memcpy(&rawVal, &inFilePtr[i*blockSizeBytes+offsetBytes], RAW_TYPE_SIZE);
      result[i] = (double)(rawVal);
   }
}
