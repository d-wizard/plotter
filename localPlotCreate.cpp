/* Copyright 2016 Dan Williams. All Rights Reserved.
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
#include <QMessageBox>
#include "localPlotCreate.h"
#include "FileSystemOperations.h"
#include "dString.h"
#include "saveRestoreCurve.h"
#include "overwriterenamedialog.h"
#include "plotcurvenamedialog.h"



bool localPlotCreate::validateNewPlotCurveName(CurveCommander* p_curveCmdr, QString& plotName, QString& curveName)
{
   bool namesAreValid = false;
   bool finished = false;

   if(p_curveCmdr->validCurve(plotName, curveName) == false)
   {
      // plot->curve does not exist, the input names are valid.
      namesAreValid = true;
   }
   else
   {
      // plot->curve does exists, ask user whether to overwrite existing curve, rename
      // the new curve or cancel creation of new curve.
      do
      {
         overwriteRenameDialog overRenameDlg(NULL);
         ePlotExistsReturn whatToDo = overRenameDlg.askUserAboutExistingPlot(plotName, curveName);
         if(whatToDo == RENAME)
         {
            plotCurveNameDialog plotCurveDialog;

            // Fill in Plot Name Combo Box with existing plot names.
            tCurveCommanderInfo allCurves = p_curveCmdr->getCurveCommanderInfo();
            QComboBox* plotCurveDialog_plotNameComboBox = plotCurveDialog.getPlotNameCombo();
            foreach( QString curPlotName, allCurves.keys() )
            {
               plotCurveDialog_plotNameComboBox->addItem(curPlotName);
            }

            bool renamed = plotCurveDialog.getPlotCurveNameFromUser(plotName, curveName);
            if(renamed == true && p_curveCmdr->validCurve(plotName, curveName) == false)
            {
               // Renamed to a unique plot->curve name. Return valid.
               namesAreValid = true;
               finished = true;
            }
            else if(renamed == false)
            {
               // User clicked cancel. Return invalid.
               namesAreValid = false;
               finished = true;
            }
         }
         else if(whatToDo == OVERWRITE)
         {
            // Overwrite. Do not rename. Return valid.
            namesAreValid = true;
            finished = true;
         }
         else
         {
            // Cancel. Return invalid.
            namesAreValid = false;
            finished = true;
         }

      }while(finished == false);
   }

   return namesAreValid;
}


void localPlotCreate::restorePlotFromFile(CurveCommander* p_curveCmdr, QString fileName, QString plotName)
{
   std::vector<char> curveFile;
   fso::ReadBinaryFile(fileName.toStdString(), curveFile);

   // Get plot name.
   if(plotName == "")
   {
      plotName = QString(fso::GetFileNameNoExt(fileName.toStdString()).c_str());
      if(plotName == "")
      {
         plotName = "New Plot";
      }
   }

   bool inputIsValid = fileName == "" ? true : false; // NULL string return is cancel, which is valid.
   QString ext(fso::GetExt(dString::Lower(fileName.toStdString())).c_str());
   if(ext == "curve")
   {
      RestoreCurve t_restoreCurve(curveFile);
      inputIsValid = t_restoreCurve.isValid;
      if(t_restoreCurve.isValid)
      {
         if(validateNewPlotCurveName(p_curveCmdr, plotName, t_restoreCurve.params.curveName))
         {
            restoreCurve(p_curveCmdr, plotName, &t_restoreCurve.params);
         }
      } // End if(restoreCurve.isValid)

   }
   else if(ext == "plot")
   {
      RestorePlot restorePlot(curveFile);
      inputIsValid = restorePlot.isValid;
      if(restorePlot.isValid)
      {
         restoreMultipleCurves(p_curveCmdr, restorePlot.plotName, restorePlot.params);
      }
   }
   else if(ext == "csv")
   {
      RestoreCsv restoreCsv(curveFile);
      inputIsValid = restoreCsv.isValid;
      if(restoreCsv.isValid)
      {
         restoreMultipleCurves(p_curveCmdr, plotName, restoreCsv.params);
      }
   }

   if(inputIsValid == false)
   {
      QString msgBoxFileName(fso::dirSepToOS(fileName.toStdString()).c_str());
      QMessageBox::question( NULL,
                             "Invalid File",
                             msgBoxFileName + "\n is Invalid.",
                             QMessageBox::Ok );
   }
}

void localPlotCreate::restoreCurve(CurveCommander* p_curveCmdr, QString plotName, tSaveRestoreCurveParams* curveParam)
{
   if(curveParam->plotDim == E_PLOT_DIM_1D)
   {
      p_curveCmdr->create1dCurve(plotName, curveParam->curveName, curveParam->plotType, curveParam->yOrigPoints);
   }
   else
   {
      p_curveCmdr->create2dCurve(plotName, curveParam->curveName, curveParam->xOrigPoints, curveParam->yOrigPoints);
   }

   MainWindow* plot = p_curveCmdr->getMainPlot(plotName);
   if(plot != NULL)
   {
      plot->setCurveProperties(curveParam->curveName, E_X_AXIS, curveParam->sampleRate, curveParam->mathOpsXAxis);
      plot->setCurveProperties(curveParam->curveName, E_Y_AXIS, curveParam->sampleRate, curveParam->mathOpsYAxis);
   }
}

void localPlotCreate::restoreMultipleCurves(CurveCommander* p_curveCmdr, QString plotName, QVector<tSaveRestoreCurveParams>& curves)
{
   // Make sure inputs are valid.
   if(plotName == "" || curves.size() == 0)
      return;

   if(p_curveCmdr->validPlot(plotName))
   {
      // Plot name already exists.

      if(curves.size() == 1)
      {
         // If there is only 1 curve to add, let the user change the plot/curve name to avoid conflict.
         if(validateNewPlotCurveName(p_curveCmdr, plotName, curves[0].curveName) == false)
            return; // User chose to ingore the new curve.
      }
      else
      {
         // There are more than 1 curves for the new plot, just use a new unique plot name.
         plotName = getUniquePlotName(p_curveCmdr, plotName);
      }
   }

   for(int i = 0; i < curves.size(); ++i)
   {
      restoreCurve(p_curveCmdr, plotName, &curves[i]);
   }

}

QString localPlotCreate::getUniquePlotName(CurveCommander* p_curveCmdr, QString plotName)
{
   QString newPlotName = plotName;
   int addIndex = 1;
   while(p_curveCmdr->validPlot(newPlotName))
   {
      newPlotName = plotName + " " + QString::number(addIndex);
      addIndex++;
   }
   return newPlotName;
}
