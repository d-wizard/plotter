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
#include "saveRestoreCurve.h"

static void pack(char** packArray, const void* toPack, size_t packSize)
{
   memcpy((*packArray), toPack, packSize);
   (*packArray) += packSize;
}

static void unpack(char** packArray, void* toUnpack, size_t packSize)
{
   memcpy(toUnpack, (*packArray), packSize);
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
   pack(&packArray, curve->getYPoints(), dataPointsSize1Axis);

   // Pack X Axis Data if 2D plot
   if(params.plotDim == E_PLOT_DIM_2D)
      pack(&packArray, curve->getXPoints(), dataPointsSize1Axis);

}


RestoreCurve::RestoreCurve(PackedCurveData& packedCurve)
{
   char* packedArray = &packedCurve[0];

   params.curveName = QString(packedArray);
   packedArray += (params.curveName.size() + 1);
   unpack(&packedArray, &params.plotDim, sizeof(params.plotDim));
   unpack(&packedArray, &params.plotType, sizeof(params.plotType));
   unpack(&packedArray, &params.numPoints, sizeof(params.numPoints));
   unpack(&packedArray, &params.sampleRate, sizeof(params.sampleRate));

   // Get X Axis Math Operations
   unpack(&packedArray, &params.numXMapOps, sizeof(params.numXMapOps));
   params.mathOpsXAxis.clear();
   for(unsigned int i = 0; i < params.numXMapOps; ++i)
   {
      tOperation newOp;
      unpack(&packedArray, &newOp, sizeof(newOp));
      params.mathOpsXAxis.push_back(newOp);
   }

   // Get Y Axis Math Operations
   unpack(&packedArray, &params.numYMapOps, sizeof(params.numYMapOps));
   params.mathOpsYAxis.clear();
   for(unsigned int i = 0; i < params.numYMapOps; ++i)
   {
      tOperation newOp;
      unpack(&packedArray, &newOp, sizeof(newOp));
      params.mathOpsYAxis.push_back(newOp);
   }

   // Unpack Y Axis Data Points
   params.yOrigPoints.resize(params.numPoints);
   unpack(&packedArray, &params.yOrigPoints[0], (params.numPoints * sizeof(double)));

   // Unpack X Axis Data Points
   if(params.plotDim == E_PLOT_DIM_2D)
   {
      params.xOrigPoints.resize(params.numPoints);
      unpack(&packedArray, &params.xOrigPoints[0], (params.numPoints * sizeof(double)));
   }

}

