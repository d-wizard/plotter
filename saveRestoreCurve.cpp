/* Copyright 2014 Dan Williams. All Rights Reserved.
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
#include <string.h>
#include <sstream>
#include "saveRestoreCurve.h"

static void pack(char** packArray, const void* toPack, size_t packSize)
{
   memcpy((*packArray), toPack, packSize);
   (*packArray) += packSize;
}


//PackedCurveData packedCurveData;
SaveCurve::SaveCurve(CurveData* curve)
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

SaveCurve::SaveCurve(CurveData* curve, MainWindow* plotGui)
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
         csvFile << curve->getYPoints()[i] << "\r\n";
      }
      plotGui->clearDisplayIoMapIp(csvFile);
   }
   else
   {
       csvFile << curve->getCurveTitle().toStdString();

       plotGui->setDisplayIoMapipYAxis(csvFile);
       for(unsigned int i = 0; i < curve->getNumPoints(); ++i)
       {
          csvFile << curve->getYPoints()[i] << "\r\n";
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
