/* Copyright 2014 - 2015 Dan Williams. All Rights Reserved.
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
#include <string.h>
#include <sstream>
#include <QString>
#include <QStringList>
#include <algorithm>
#include "saveRestoreCurve.h"
#include "dString.h"

static const std::string CSV_LINE_DELIM = "\r\n";

static void pack(char** packArray, const void* toPack, size_t packSize)
{
   memcpy((*packArray), toPack, packSize);
   (*packArray) += packSize;
}


SaveCurve::SaveCurve(MainWindow *plotGui, CurveData *curve, eSaveRestorePlotCurveType type)
{
    if(type == E_SAVE_RESTORE_RAW)
    {
        SaveRaw(curve);
    }
    else
    {
        SaveCsv(plotGui, curve);
    }
}


void SaveCurve::SaveRaw(CurveData* curve)
{
    params.curveName = curve->getCurveTitle();
    params.plotDim = curve->getPlotDim();
    params.plotType = curve->getPlotType();
    params.numPoints = curve->getNumPoints();
    params.sampleRate = curve->getSampleRate();
    params.mathOpsXAxis = curve->getMathOps(E_X_AXIS);
    params.numXMapOps = params.mathOpsXAxis.size();
    params.mathOpsYAxis = curve->getMathOps(E_Y_AXIS);
    params.numYMapOps = params.mathOpsYAxis.size();

    unsigned long dataPointsSize1Axis = (params.numPoints * sizeof(double));
    unsigned long dataPointsSize = (params.plotDim == E_PLOT_DIM_2D) ? (2*dataPointsSize1Axis) : dataPointsSize1Axis;

    if(params.curveName.size() > MAX_STORE_CURVE_NAME_SIZE)
    {
       params.curveName.resize(MAX_STORE_CURVE_NAME_SIZE);
    }

    unsigned long finalSize =
          params.curveName.size() + 1 +
          sizeof(params.plotDim) +
          sizeof(params.plotType) +
          sizeof(params.numPoints) +
          sizeof(params.sampleRate) +
          sizeof(params.numXMapOps) +
          (params.numXMapOps * sizeof(tOperation)) +
          sizeof(params.numYMapOps) +
          (params.numYMapOps * sizeof(tOperation)) +
          dataPointsSize;

    char nullTerm = '\0';

    packedCurveData.clear();
    packedCurveData.resize(finalSize);
    char* packArray = &packedCurveData[0];

    pack(&packArray, params.curveName.toStdString().c_str(), params.curveName.size());
    pack(&packArray, &nullTerm, 1);
    pack(&packArray, &params.plotDim, sizeof(params.plotDim));
    pack(&packArray, &params.plotType, sizeof(params.plotType));
    pack(&packArray, &params.numPoints, sizeof(params.numPoints));
    pack(&packArray, &params.sampleRate, sizeof(params.sampleRate));

    // Pack X Axis Math Operations
    pack(&packArray, &params.numXMapOps, sizeof(params.numXMapOps));
    for(tMathOpList::iterator iter = params.mathOpsXAxis.begin(); iter != params.mathOpsXAxis.end(); ++iter)
       pack(&packArray, &(*iter), sizeof(*iter));

    // Pack Y Axis Math Operations
    pack(&packArray, &params.numYMapOps, sizeof(params.numYMapOps));
    for(tMathOpList::iterator iter = params.mathOpsYAxis.begin(); iter != params.mathOpsYAxis.end(); ++iter)
       pack(&packArray, &(*iter), sizeof(*iter));

    // Pack Y Axis Data
    pack(&packArray, curve->getOrigYPoints(), dataPointsSize1Axis);

    // Pack X Axis Data if 2D plot
    if(params.plotDim == E_PLOT_DIM_2D)
       pack(&packArray, curve->getOrigXPoints(), dataPointsSize1Axis);
}

void SaveCurve::SaveCsv(MainWindow *plotGui, CurveData *curve)
{
    params.curveName = curve->getCurveTitle();
    params.plotDim = curve->getPlotDim();
    params.plotType = curve->getPlotType();
    params.numPoints = curve->getNumPoints();
    params.sampleRate = curve->getSampleRate();
    params.mathOpsXAxis = curve->getMathOps(E_X_AXIS);
    params.numXMapOps = params.mathOpsXAxis.size();
    params.mathOpsYAxis = curve->getMathOps(E_Y_AXIS);
    params.numYMapOps = params.mathOpsYAxis.size();


    std::stringstream csvFile;
    if(curve->getPlotDim() == E_PLOT_DIM_2D)
    {
       csvFile << curve->getCurveTitle().toStdString() << " - X Axis,";
       csvFile << curve->getCurveTitle().toStdString() << " - Y Axis\r\n";

       for(unsigned int i = 0; i < curve->getNumPoints(); ++i)
       {
          plotGui->setDisplayIoMapipXAxis(csvFile, curve);
          csvFile << curve->getXPoints()[i] << ",";
          plotGui->setDisplayIoMapipYAxis(csvFile);
          csvFile << curve->getYPoints()[i] << CSV_LINE_DELIM;
       }
       plotGui->clearDisplayIoMapIp(csvFile);
    }
    else
    {
        csvFile << curve->getCurveTitle().toStdString() << CSV_LINE_DELIM;

        plotGui->setDisplayIoMapipYAxis(csvFile);
        for(unsigned int i = 0; i < curve->getNumPoints(); ++i)
        {
           csvFile << curve->getYPoints()[i] << CSV_LINE_DELIM;
        }
        plotGui->clearDisplayIoMapIp(csvFile);
    }


    packedCurveData.resize(csvFile.str().size());
    memcpy(&packedCurveData[0], csvFile.str().c_str(), csvFile.str().size());
}

RestoreCurve::RestoreCurve(PackedCurveData& packedCurve)
{
   isValid = false;

   m_packedArray = &packedCurve[0];
   m_packedSize = packedCurve.size();

   if(m_packedSize <= 0)
      return; // invalid input.

   // Validate Curve Name... find null terminator
   bool curveNameValid = false;
   for(unsigned int i = 0; i <= MAX_STORE_CURVE_NAME_SIZE && i < m_packedSize; ++i)
   {
      if(m_packedArray[i] == '\0')
      {
         curveNameValid = true;
         break;
      }
   }

   if(curveNameValid == false)
      // Invalid curve name, return.
      return;

   // Valid curve name, copy it out and continue.
   params.curveName = QString(m_packedArray);
   m_packedArray += (params.curveName.size() + 1);
   m_packedSize -= (params.curveName.size() + 1);

   try
   {
      // Read in remaining parameters. Catch execption if attempting
      // to read passed the end of the input file. Early return if
      // parameter read from input is not valid.

      unpack(&params.plotDim, sizeof(params.plotDim));
      if(valid_ePlotDim(params.plotDim) == false)
         return;

      unpack(&params.plotType, sizeof(params.plotType));
      if(valid_ePlotType(params.plotType) == false)
         return;

      unpack(&params.numPoints, sizeof(params.numPoints));
      if(params.numPoints <= 0)
         return;

      unpack(&params.sampleRate, sizeof(params.sampleRate));

      // Get X Axis Math Operations
      unpack(&params.numXMapOps, sizeof(params.numXMapOps));
      params.mathOpsXAxis.clear();
      for(unsigned int i = 0; i < params.numXMapOps; ++i)
      {
         tOperation newOp;
         unpack(&newOp, sizeof(newOp));

         // Validate new operation... return if invalid.
         if(valid_eMathOp(newOp.op))
            params.mathOpsXAxis.push_back(newOp);
         else
            return;
      }

      // Get Y Axis Math Operations
      unpack(&params.numYMapOps, sizeof(params.numYMapOps));
      params.mathOpsYAxis.clear();
      for(unsigned int i = 0; i < params.numYMapOps; ++i)
      {
         tOperation newOp;
         unpack(&newOp, sizeof(newOp));

         // Validate new operation... return if invalid.
         if(valid_eMathOp(newOp.op))
            params.mathOpsYAxis.push_back(newOp);
         else
            return;
      }

      // Unpack Y Axis Data Points
      params.yOrigPoints.resize(params.numPoints);
      unpack(&params.yOrigPoints[0], (params.numPoints * sizeof(double)));

      // Unpack X Axis Data Points
      if(params.plotDim == E_PLOT_DIM_2D)
      {
         params.xOrigPoints.resize(params.numPoints);
         unpack(&params.xOrigPoints[0], (params.numPoints * sizeof(double)));
      }
   }
   catch(int dontCare)
   {
      return; // Tried to unpack beyond the input file size, return.
   }

   isValid = true; // If invalid would have early returned. If we got here, the input is valid.
}

void RestoreCurve::unpack(void* toUnpack, size_t unpackSize)
{
   if(unpackSize <= m_packedSize)
   {
      memcpy(toUnpack, m_packedArray, unpackSize);
      m_packedArray += unpackSize;
      m_packedSize -= unpackSize;
   }
   else
   {
      throw 0;
   }
}


SavePlot::SavePlot(MainWindow* plotGui, QString plotName, QVector<CurveData*>& plotInfo, eSaveRestorePlotCurveType type)
{
    if(type == E_SAVE_RESTORE_RAW)
    {
        SaveRaw(plotGui, plotName, plotInfo);
    }
    else
    {
        SaveCsv(plotGui, plotInfo);
    }
}

void SavePlot::SaveRaw(MainWindow* plotGui, QString plotName, QVector<CurveData*>& plotInfo)
{
   QVector<PackedCurveData> curveRawFiles;
   UINT_32 fileSize = 0;
   for(int i = 0; i < plotInfo.size(); ++i)
   {
      SaveCurve curveFile(plotGui, plotInfo[i], E_SAVE_RESTORE_RAW);
      curveRawFiles.push_back(curveFile.packedCurveData);
      fileSize += curveFile.packedCurveData.size();
   }

   UINT_32 numCurves = curveRawFiles.size();

   fileSize += (plotName.size() + 1); // Plot Name plus null term.
   fileSize += sizeof(numCurves);     // Storing the number of curves.
   fileSize += (numCurves * sizeof(UINT_32)); // Storing the number of bytes for each packed curve.

   packedCurveData.resize(fileSize);
   char* packPtr = &packedCurveData[0];
   char* packEnd = packPtr + fileSize;
   char nullTerm = '\0';

   //static void pack(char** packArray, const void* toPack, size_t packSize)
   pack(&packPtr, plotName.toStdString().c_str(), plotName.size());
   pack(&packPtr, &nullTerm, 1);
   pack(&packPtr, &numCurves, sizeof(numCurves));

   for(UINT_32 i = 0; i < numCurves; ++i)
   {
      UINT_32 packedCurveSize = curveRawFiles[i].size();
      pack(&packPtr, &packedCurveSize, sizeof(packedCurveSize));
   }

   for(UINT_32 i = 0; i < numCurves; ++i)
   {
      pack(&packPtr, &curveRawFiles[i][0], curveRawFiles[i].size());
   }

   assert(packPtr == packEnd);

}

void SavePlot::SaveCsv(MainWindow* plotGui, QVector<CurveData*> &plotInfo)
{
   QVector<QStringList> curveCsvFiles;
   for(int i = 0; i < plotInfo.size(); ++i)
   {
      SaveCurve curveFile(plotGui, plotInfo[i], E_SAVE_RESTORE_CSV);

      int rawCurveFileSize = curveFile.packedCurveData.size();
      char* rawCurveFile = new char[rawCurveFileSize+1];
      memcpy(rawCurveFile, &curveFile.packedCurveData[0], rawCurveFileSize);
      rawCurveFile[rawCurveFileSize] = '\0';

      curveCsvFiles.push_back(QString(rawCurveFile).split(CSV_LINE_DELIM.c_str(), QString::SkipEmptyParts));

      delete [] rawCurveFile;
   }

   int maxNumLines = 0;
   for(int i = 0; i < curveCsvFiles.size(); ++i)
   {
      if(curveCsvFiles[i].size() > maxNumLines)
      {
         maxNumLines = curveCsvFiles[i].size();
      }
   }

   QString packed;
   for(int i = 0; i < maxNumLines; ++i)
   {
      int numCurves = curveCsvFiles.size();
      for(int j = 0; j < numCurves; ++j)
      {
         if(curveCsvFiles[j].size() > i)
         {
            packed.append(curveCsvFiles[j][i]);
         }
         if(j < (numCurves-1))
         {
            packed.append(",");
         }
         else
         {
            packed.append(CSV_LINE_DELIM.c_str());
         }
      }
   }

   packedCurveData.resize(packed.size());
   memcpy(&packedCurveData[0], packed.toStdString().c_str(), packed.size());
}


RestorePlot::RestorePlot(PackedCurveData &packedPlot)
{
   isValid = false;

   UINT_32 packedSize = packedPlot.size();
   if(packedSize <= 0)
      return; // Invalid, early return.

   // Find null termination of plot name.
   int nullTermSearchLen = std::min(1024, (int)packedSize);
   bool nullTermFound = false;
   char* pc_packed = &packedPlot[0];

   for(int i = 0; i < nullTermSearchLen; ++i)
   {
      if(pc_packed[i] == '\0')
      {
         nullTermFound = true;
         break;
      }
   }

   if(nullTermFound == false)
      return; // Invalid, early return.

   // Null termination of plot name was found, read in the plot name from the packed data.
   QString testPlotName(&pc_packed[0]);

   UINT_32 numCurves = 0;

   UINT_32 unpackIndex = 0;

   if((testPlotName.size() + sizeof(numCurves)) > packedSize)
   {
      return; // Early return on failure.
   }

   unpackIndex = testPlotName.size() + 1;

   memcpy(&numCurves, &pc_packed[unpackIndex], sizeof(numCurves));
   unpackIndex += sizeof(numCurves);

   if(numCurves == 0)
   {
      return; // Early return on failure.
   }

   if( (unpackIndex + (numCurves * sizeof(UINT_32))) > packedSize )
   {
      return; // Early return on failure.
   }

   QVector<UINT_32> packedCurveSizes;
   packedCurveSizes.resize(numCurves);

   UINT_32 totalCurveSize = 0;
   for(UINT_32 i = 0; i < numCurves; ++i)
   {
      memcpy(&packedCurveSizes[i], &pc_packed[unpackIndex], sizeof(packedCurveSizes[i]));
      unpackIndex += sizeof(packedCurveSizes[i]);
      totalCurveSize += packedCurveSizes[i];
   }

   if( (unpackIndex + totalCurveSize) != packedSize)
   {
      return; // Early return on failure.
   }

   for(UINT_32 i = 0; i < numCurves; ++i)
   {
      PackedCurveData newPackedCurve;
      newPackedCurve.resize(packedCurveSizes[i]);

      memcpy(&newPackedCurve[0], &pc_packed[unpackIndex], packedCurveSizes[i]);
      unpackIndex += packedCurveSizes[i];

      RestoreCurve restoreCurve(newPackedCurve);

      if(restoreCurve.isValid == false)
      {
         return; // Early return on failure.
      }

      params.push_back(restoreCurve.params);
   }
   plotName = testPlotName;

   isValid = true;
}

RestoreCsv::RestoreCsv(PackedCurveData &packedPlot)
{
   isValid = false;
   hasBadCells = false;

   UINT_32 totalSamplesInCsv = 0;

   if(packedPlot.size() <= 0)
      return; // Invalid, early return.

   std::string csvFile(&packedPlot[0], packedPlot.size());

   csvFile = dString::ConvertLineEndingToUnix(csvFile);

   std::vector<std::string> csvRows = dString::SplitV(csvFile, "\n");

   try
   {
      std::vector<std::string> csvCells = dString::SplitV(csvRows[0], ",");

      // Determine if the first row is a header row.
      bool firstRowIsAllNums = true;
      int numCol = csvCells.size();
      for(int i = 0; i < numCol; ++i)
      {
         double testDoub;
         if(dString::strTo(csvCells[i], testDoub) == false)
         {
            firstRowIsAllNums = false;
            break;
         }
      }

      params.resize(numCol);

      // Fill in the values from the CSV File.
      int rowStart = firstRowIsAllNums ? 0 : 1;
      for(int i = rowStart; i < (int)csvRows.size(); ++i)
      {
         csvCells = dString::SplitV(csvRows[i], ",");
         for(int j = 0; j < (int)csvCells.size(); ++j)
         {
            if(j < numCol && csvCells[j] != "")
            {
               double testDoub = 0.0;
               if(dString::strTo(csvCells[j], testDoub))
               {
                  params[j].yOrigPoints.push_back(testDoub);
               }
               else
               {
                  hasBadCells = true;
                  params[j].yOrigPoints.push_back(testDoub);
               }
            }
         }
      }

      // Get curve names from first row.
      if(firstRowIsAllNums == false)
      {
         csvCells = dString::SplitV(csvRows[0], ",");
         for(int i = 0; i < numCol; ++i)
         {
            params[i].curveName = csvCells[i].c_str();
         }
      }
      else
      {
         for(int i = 0; i < numCol; ++i)
         {
            params[i].curveName = "Curve " + QString::number(i+1);
         }
      }

      // Fill in the rest of the parameters.
      for(int i = 0; i < numCol; ++i)
      {
         params[i].plotDim = E_PLOT_DIM_1D;
         params[i].plotType = E_PLOT_TYPE_1D;
         params[i].numPoints = params[i].yOrigPoints.size();
         params[i].sampleRate = 0.0;
         params[i].numXMapOps = 0;
         params[i].mathOpsXAxis.clear();
         params[i].numYMapOps = 0;
         params[i].mathOpsYAxis.clear();
         params[i].xOrigPoints.clear();

         totalSamplesInCsv += params[i].numPoints;
      }

   }
   catch(int dontCare)
   {
      return; // Invalid, early return.
   }

   isValid = totalSamplesInCsv > 0;
}







