/* Copyright 2014 - 2017, 2019, 2021 - 2022 Dan Williams. All Rights Reserved.
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

static const std::string EXCEL_LINE_DELIM = "\r\n";
static const std::string CSV_CELL_DELIM = ",";
static const char        CSV_CELL_DELIM_CHAR = ',';
static const std::string CLIPBOARD_EXCEL_CELL_DELIM = "\t";

static const std::string C_HEADER_LINE_DELIM = "\r\n";

static void pack(char** packArray, const void* toPack, size_t packSize)
{
   memcpy((*packArray), toPack, packSize);
   (*packArray) += packSize;
}

static std::string convertToValidCName(std::string inStr)
{
   const char* in_cstr = inStr.c_str();
   int in_size = inStr.size();

   std::string retVal;
   for(int i = 0; i < in_size; ++i)
   {
      if(in_cstr[i] >= 'A' && in_cstr[i] <= 'Z')
         retVal += in_cstr[i];
      else if(in_cstr[i] >= 'a' && in_cstr[i] <= 'z')
         retVal += (in_cstr[i] - 'a' + 'A'); // To Upper.
      else if(in_cstr[i] >= '0' && in_cstr[i] <= '9')
      {
         if(retVal == "") // Can't start with number.
            retVal = "_"; // Prepend number with underscore.
         retVal += in_cstr[i];
      }
      else
         retVal += '_';
   }
   return retVal;
}

static std::string getPlotNameCHeaderTypeName(QString plotName, QString curveName)
{
   std::string cPlotName = convertToValidCName(plotName.toStdString());
   std::string cCurveName = convertToValidCName(curveName.toStdString());
   if(cPlotName == "")
      cPlotName = "PLOT_NAME";
   if(cCurveName == "")
      cCurveName = "CURVE_NAME";
   return cPlotName + "_" + cCurveName + "_T";
}

static std::string getPlotNameCHeaderVariableName(QString curveName)
{
   std::string retVal = convertToValidCName(curveName.toStdString());
   if(retVal == "")
      retVal = "CURVE_NAME";
   return retVal;
}

static std::string getCHeaderDataType(eSaveRestorePlotCurveType type, ePlotDataTypes xAxis, ePlotDataTypes yAxis, bool& isInt)
{
   std::string typeStr;
   isInt = true; // assume int until proven otherwise
   switch(type)
   {
      case E_SAVE_RESTORE_C_HEADER_INT:
         typeStr = "long long";
      break;
      case E_SAVE_RESTORE_C_HEADER_FLOAT:
         typeStr = "double";
         isInt = false;
      break;
      default:
      {
         if(xAxis != yAxis && xAxis != E_INVALID_DATA_TYPE)
         {
            // 2D plot, but X axis type doesn't match Y axis type. Just use double to be sure.
            typeStr = "double";
            isInt = false;
         }
         else
         {
            switch(yAxis)
            {
               case E_CHAR:
                  typeStr = "signed char";
               break;
               case E_UCHAR:
                  typeStr = "unsigned char";
               break;
               case E_INT_16:
                  typeStr = "short";
               break;
               case E_UINT_16:
                  typeStr = "unsigned short";
               break;
               case E_INT_32:
                  typeStr = "long";
               break;
               case E_UINT_32:
                  typeStr = "unsigned long";
               break;
               case E_INT_64:
                  typeStr = "long long";
               break;
               case E_UINT_64:
                  typeStr = "unsigned long long";
               break;
               case E_FLOAT_32:
                  typeStr = "float";
                  isInt = false;
               break;
               default:
                  typeStr = "double";
                  isInt = false;
               break;
            }
         }
      }
      break;
   }
   return typeStr;
}

static std::string getCHeaderTypedefStr(QString plotName, QString curveName, std::string dataTypeStr)
{
   std::string typeNameStr = getPlotNameCHeaderTypeName(plotName, curveName);
   return "#ifndef " + typeNameStr + C_HEADER_LINE_DELIM +
          "#define " + typeNameStr + " " + dataTypeStr + C_HEADER_LINE_DELIM +
          "#endif" + C_HEADER_LINE_DELIM;
}

SaveCurve::SaveCurve(MainWindow *plotGui, CurveData *curve, eSaveRestorePlotCurveType type, bool limitToZoom):
   m_limitToZoom(limitToZoom)
{
   switch(type)
   {
      default:
      case E_SAVE_RESTORE_RAW:
         SaveRaw(curve);
      break;
      case E_SAVE_RESTORE_CSV:
         SaveExcel(plotGui, curve, CSV_CELL_DELIM);
      break;
      case E_SAVE_RESTORE_CLIPBOARD_EXCEL:
         SaveExcel(plotGui, curve, CLIPBOARD_EXCEL_CELL_DELIM);
      break;
      case E_SAVE_RESTORE_C_HEADER_AUTO_TYPE:
      case E_SAVE_RESTORE_C_HEADER_INT:
      case E_SAVE_RESTORE_C_HEADER_FLOAT:
         SaveCHeader(plotGui, curve, type);
      break;
   }
}

void SaveCurve::getPackedData(PackedCurveData& packedDataReturn)
{
   packedDataReturn.resize(0);
   packedDataReturn.insert(packedDataReturn.end(), packedCurveHead.begin(), packedCurveHead.end());
   packedDataReturn.insert(packedDataReturn.end(), packedCurveData.begin(), packedCurveData.end());
}

void SaveCurve::SaveRaw(CurveData* curve)
{
    tSaveRestoreCurveParams params;
    params.curveName = curve->getCurveTitle();
    params.plotDim = curve->getPlotDim();
    params.plotType = curve->getPlotType();
    params.numPoints = curve->getNumPoints();
    params.mathProps.sampleRate = curve->getSampleRate();
    params.mathProps.mathOpsXAxis = curve->getMathOps(E_X_AXIS);
    params.numXMapOps = params.mathProps.mathOpsXAxis.size();
    params.mathProps.mathOpsYAxis = curve->getMathOps(E_Y_AXIS);
    params.numYMapOps = params.mathProps.mathOpsYAxis.size();

    unsigned long dataPointsSize1Axis = (params.numPoints * sizeof(double));
    unsigned long dataPointsSize = (params.plotDim == E_PLOT_DIM_2D) ? (2*dataPointsSize1Axis) : dataPointsSize1Axis;

    if(params.curveName.size() > MAX_STORE_PLOT_CURVE_NAME_SIZE)
    {
       params.curveName.resize(MAX_STORE_PLOT_CURVE_NAME_SIZE);
    }

    unsigned long finalSize =
          params.curveName.size() + 1 +
          sizeof(params.plotDim) +
          sizeof(params.plotType) +
          sizeof(params.numPoints) +
          sizeof(params.mathProps.sampleRate) +
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
    pack(&packArray, &params.mathProps.sampleRate, sizeof(params.mathProps.sampleRate));

    // Pack X Axis Math Operations
    pack(&packArray, &params.numXMapOps, sizeof(params.numXMapOps));
    for(tMathOpList::iterator iter = params.mathProps.mathOpsXAxis.begin(); iter != params.mathProps.mathOpsXAxis.end(); ++iter)
       pack(&packArray, &(*iter), sizeof(*iter));

    // Pack Y Axis Math Operations
    pack(&packArray, &params.numYMapOps, sizeof(params.numYMapOps));
    for(tMathOpList::iterator iter = params.mathProps.mathOpsYAxis.begin(); iter != params.mathProps.mathOpsYAxis.end(); ++iter)
       pack(&packArray, &(*iter), sizeof(*iter));

    // Pack Y Axis Data
    pack(&packArray, curve->getOrigYPoints(), dataPointsSize1Axis);

    // Pack X Axis Data if 2D plot
    if(params.plotDim == E_PLOT_DIM_2D)
       pack(&packArray, curve->getOrigXPoints(), dataPointsSize1Axis);
}

void SaveCurve::SaveExcel(MainWindow* plotGui, CurveData* curve, std::string delim)
{
   std::stringstream csvFile;
   if(curve->getPlotDim() == E_PLOT_DIM_2D)
   {
      csvFile << curve->getCurveTitle().toStdString() << " - X Axis" << delim;
      csvFile << curve->getCurveTitle().toStdString() << " - Y Axis\r\n";
      
      for(unsigned int i = 0; i < curve->getNumPoints(); ++i)
      {
         plotGui->setDisplayIoMapipXAxis(csvFile, curve);
         csvFile << curve->getXPoints()[i] << delim;
         plotGui->setDisplayIoMapipYAxis(csvFile);
         csvFile << curve->getYPoints()[i] << EXCEL_LINE_DELIM;
      }
      plotGui->clearDisplayIoMapIp(csvFile);
   }
   else
   {
      unsigned int startInclusive = 0;
      unsigned int stopExclusive = curve->getNumPoints();

      if(m_limitToZoom)
      {
         maxMinXY displayed = curve->get1dDisplayedIndexes();
         displayed.minX += 1; // Min is the sample before the first sample displayed.

         if(displayed.minX > startInclusive)
            startInclusive = displayed.minX;
         if(displayed.maxX < stopExclusive)
            stopExclusive = displayed.maxX;
      }
      
      csvFile << curve->getCurveTitle().toStdString() << EXCEL_LINE_DELIM;

      plotGui->setDisplayIoMapipYAxis(csvFile);
      for(unsigned int i = startInclusive; i < stopExclusive; ++i)
      {
         csvFile << curve->getYPoints()[i] << EXCEL_LINE_DELIM;
      }
      plotGui->clearDisplayIoMapIp(csvFile);
   }

   packedCurveData.resize(csvFile.str().size());
   memcpy(&packedCurveData[0], csvFile.str().c_str(), csvFile.str().size());
}

void SaveCurve::SaveCHeader(MainWindow* plotGui, CurveData* curve, eSaveRestorePlotCurveType type)
{
   std::stringstream headerFile;
   std::stringstream outFile;
   static const int MAX_SAMP_PER_LINE = 10;
   int sampPerLineCount = 0;

   unsigned int numSamplesToWrite = curve->getNumPoints();

   // Determine the data type string (i.e. long, double, unsigned char, etc)
   bool dataType_isInt = false;
   std::string dataType_str = getCHeaderDataType(
      type,
      curve->getPlotDim() == E_PLOT_DIM_1D ? E_INVALID_DATA_TYPE : curve->getLastMsgDataType(E_X_AXIS),
      curve->getLastMsgDataType(E_Y_AXIS),
      dataType_isInt);

   headerFile << getCHeaderTypedefStr(plotGui->getPlotName(), curve->getCurveTitle(), dataType_str) << C_HEADER_LINE_DELIM;

   if(curve->getPlotDim() != E_PLOT_DIM_1D)
   {
      const double* xPoints = curve->getXPoints();
      const double* yPoints = curve->getYPoints();
      outFile << getPlotNameCHeaderTypeName(plotGui->getPlotName(), curve->getCurveTitle()) << " " <<
                 getPlotNameCHeaderVariableName(curve->getCurveTitle()) <<
                 "[" << curve->getNumPoints() << "][2] = {" << C_HEADER_LINE_DELIM;

      if(dataType_isInt)
      {
         for(unsigned int i = 0; i < numSamplesToWrite; ++i)
         {
            outFile << "{" << (INT_64)xPoints[i] << "," << (INT_64)yPoints[i] << "}";
            if(i < (numSamplesToWrite-1))
               outFile << ", ";
            if(++sampPerLineCount >= MAX_SAMP_PER_LINE)
            {
               outFile << C_HEADER_LINE_DELIM;
               sampPerLineCount = 0;
            }
         }
      }
      else
      {
         for(unsigned int i = 0; i < numSamplesToWrite; ++i)
         {
            plotGui->setDisplayIoMapipXAxis(outFile, curve);
            outFile << "{" << xPoints[i] << ",";
            plotGui->setDisplayIoMapipYAxis(outFile);
            outFile << yPoints[i] << "}";
            if(i < (numSamplesToWrite-1))
               outFile << ", ";
            if(++sampPerLineCount >= MAX_SAMP_PER_LINE)
            {
               outFile << C_HEADER_LINE_DELIM;
               sampPerLineCount = 0;
            }
         }
         plotGui->clearDisplayIoMapIp(outFile);
      }
      outFile << "};" << C_HEADER_LINE_DELIM << C_HEADER_LINE_DELIM;
   }
   else
   {
      const double* yPoints = curve->getYPoints();
      outFile << getPlotNameCHeaderTypeName(plotGui->getPlotName(), curve->getCurveTitle()) << " " <<
                 getPlotNameCHeaderVariableName(curve->getCurveTitle()) <<
                 "[" << curve->getNumPoints() << "] = {" << C_HEADER_LINE_DELIM;

      if(dataType_isInt)
      {
         for(unsigned int i = 0; i < numSamplesToWrite; ++i)
         {
            outFile << (INT_64)yPoints[i];
            if(i < (numSamplesToWrite-1))
               outFile << ", ";
            if(++sampPerLineCount >= MAX_SAMP_PER_LINE)
            {
               outFile << C_HEADER_LINE_DELIM;
               sampPerLineCount = 0;
            }
         }
      }
      else
      {
         plotGui->setDisplayIoMapipYAxis(outFile);
         for(unsigned int i = 0; i < numSamplesToWrite; ++i)
         {
            outFile << yPoints[i];
            if(i < (numSamplesToWrite-1))
               outFile << ", ";
            if(++sampPerLineCount >= MAX_SAMP_PER_LINE)
            {
               outFile << C_HEADER_LINE_DELIM;
               sampPerLineCount = 0;
            }
         }
         plotGui->clearDisplayIoMapIp(outFile);
      }
      outFile << "};" << C_HEADER_LINE_DELIM << C_HEADER_LINE_DELIM;
   }


   packedCurveHead.resize(headerFile.str().size());
   memcpy(&packedCurveHead[0], headerFile.str().c_str(), headerFile.str().size());

   packedCurveData.resize(outFile.str().size());
   memcpy(&packedCurveData[0], outFile.str().c_str(), outFile.str().size());
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
   for(unsigned int i = 0; i <= MAX_STORE_PLOT_CURVE_NAME_SIZE && i < m_packedSize; ++i)
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

      unpack(&params.mathProps.sampleRate, sizeof(params.mathProps.sampleRate));

      // Get X Axis Math Operations
      unpack(&params.numXMapOps, sizeof(params.numXMapOps));
      params.mathProps.mathOpsXAxis.clear();
      for(unsigned int i = 0; i < params.numXMapOps; ++i)
      {
         tOperation newOp;
         unpack(&newOp, sizeof(newOp));

         // Validate new operation... return if invalid.
         if(valid_eMathOp(newOp.op))
            params.mathProps.mathOpsXAxis.push_back(newOp);
         else
            return;
      }

      // Get Y Axis Math Operations
      unpack(&params.numYMapOps, sizeof(params.numYMapOps));
      params.mathProps.mathOpsYAxis.clear();
      for(unsigned int i = 0; i < params.numYMapOps; ++i)
      {
         tOperation newOp;
         unpack(&newOp, sizeof(newOp));

         // Validate new operation... return if invalid.
         if(valid_eMathOp(newOp.op))
            params.mathProps.mathOpsYAxis.push_back(newOp);
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


SavePlot::SavePlot(MainWindow* plotGui, QString plotName, QVector<CurveData*>& plotInfo, eSaveRestorePlotCurveType type, bool limitToZoom):
   m_limitToZoom(limitToZoom)
{
   switch(type)
   {
      default:
      case E_SAVE_RESTORE_RAW:
         SaveRaw(plotGui, plotName, plotInfo);
      break;
      case E_SAVE_RESTORE_CSV:
      case E_SAVE_RESTORE_CLIPBOARD_EXCEL:
         SaveExcel(plotGui, plotInfo, type);
      break;
      case E_SAVE_RESTORE_C_HEADER_AUTO_TYPE:
      case E_SAVE_RESTORE_C_HEADER_INT:
      case E_SAVE_RESTORE_C_HEADER_FLOAT:
         SaveCHeader(plotGui, plotInfo, type);
      break;
   }
}

void SavePlot::getPackedData(PackedCurveData& packedDataReturn)
{
   packedDataReturn.resize(0);
   packedDataReturn.insert(packedDataReturn.end(), packedCurveHead.begin(), packedCurveHead.end());
   packedDataReturn.insert(packedDataReturn.end(), packedCurveData.begin(), packedCurveData.end());
}

void SavePlot::SaveRaw(MainWindow* plotGui, QString plotName, QVector<CurveData*>& plotInfo)
{
   QVector<PackedCurveData> curveRawFiles;
   UINT_32 fileSize = 0;
   for(int i = 0; i < plotInfo.size(); ++i)
   {
      SaveCurve curveFile(plotGui, plotInfo[i], E_SAVE_RESTORE_RAW, m_limitToZoom);
      curveRawFiles.push_back(curveFile.packedCurveData);
      fileSize += curveFile.packedCurveData.size();
   }

   UINT_32 numCurves = curveRawFiles.size();

   int plotNamePackSize = std::min(MAX_STORE_PLOT_CURVE_NAME_SIZE, (int)plotName.size());

   fileSize += (plotNamePackSize + 1); // Plot Name plus null term.
   fileSize += sizeof(numCurves);     // Storing the number of curves.
   fileSize += (numCurves * sizeof(UINT_32)); // Storing the number of bytes for each packed curve.

   packedCurveData.resize(fileSize);
   char* packPtr = &packedCurveData[0];
   char* packEnd = packPtr + fileSize;
   char nullTerm = '\0';

   //static void pack(char** packArray, const void* toPack, size_t packSize)
   pack(&packPtr, plotName.toStdString().c_str(), plotNamePackSize);
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

void SavePlot::SaveExcel(MainWindow* plotGui, QVector<CurveData*> &plotInfo, eSaveRestorePlotCurveType type)
{
   std::string delim = type == E_SAVE_RESTORE_CLIPBOARD_EXCEL ? CLIPBOARD_EXCEL_CELL_DELIM : CSV_CELL_DELIM;
   QVector<QStringList> curveCsvFiles;
   for(int i = 0; i < plotInfo.size(); ++i)
   {
      SaveCurve curveFile(plotGui, plotInfo[i], type, m_limitToZoom);
      curveFile.packedCurveData.push_back('\0'); // Null Terminate to make the char array a string.
      curveCsvFiles.push_back(QString(&curveFile.packedCurveData[0]).split(EXCEL_LINE_DELIM.c_str(), Qt::SkipEmptyParts));
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
            packed.append(delim.c_str());
         }
         else
         {
            packed.append(EXCEL_LINE_DELIM.c_str());
         }
      }
   }

   packedCurveData.resize(packed.size());
   memcpy(&packedCurveData[0], packed.toStdString().c_str(), packed.size());
}

void SavePlot::SaveCHeader(MainWindow* plotGui, QVector<CurveData*>& plotInfo, eSaveRestorePlotCurveType type)
{
   for(int i = 0; i < plotInfo.size(); ++i)
   {
      SaveCurve curveFile(plotGui, plotInfo[i], type, m_limitToZoom);
      packedCurveHead.insert(packedCurveHead.end(), curveFile.packedCurveHead.begin(), curveFile.packedCurveHead.end());
      packedCurveData.insert(packedCurveData.end(), curveFile.packedCurveData.begin(), curveFile.packedCurveData.end());
   }
}

RestorePlot::RestorePlot(PackedCurveData &packedPlot)
{
   isValid = false;

   UINT_32 packedSize = packedPlot.size();
   if(packedSize <= 0)
      return; // Invalid, early return.

   // Find null termination of plot name.
   int nullTermSearchLen = std::min(MAX_STORE_PLOT_CURVE_NAME_SIZE, (int)packedSize);
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

   UINT_32 totalSamplesInCsv = 0;

   if(packedPlot.size() <= 0)
      return; // Invalid, early return.

   std::string csvFile(&packedPlot[0], packedPlot.size());

   // Determine the line ending
   std::string lineEnding;
   int numDosEnd, numUnxEnd;

   // Limit the number of characters to search over.
   if(csvFile.size() <= 500000)
   {
      numDosEnd = dString::Count(csvFile, "\r\n");
      numUnxEnd = dString::Count(csvFile, "\n");
   }
   else
   {
      std::string csvFileSmall = csvFile.substr(0, 500000);
      numDosEnd = dString::Count(csvFileSmall, "\r\n");
      numUnxEnd = dString::Count(csvFileSmall, "\n");
   }

   if(numDosEnd == 0 || numUnxEnd > (2*numDosEnd))
   {
      lineEnding = "\n";
   }
   else
   {
      lineEnding = "\r\n";
   }

   // Split file into rows.
   std::vector<std::string> csvRows;
   dString::SplitV(csvFile, lineEnding, csvRows);
   while(csvRows[csvRows.size()-1] == "") // Remove empty lines from the bottom.
   {
      csvRows.pop_back();
   }

   try
   {
      std::vector<std::string> csvCells;
      dString::SplitV(csvRows[0], CSV_CELL_DELIM, csvCells);

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
      int numRows = (int)csvRows.size();

      for(int rowIndex = rowStart; rowIndex < numRows; ++rowIndex)
      {
         int colIndex = 0;
         const char* colPtr = csvRows[rowIndex].c_str();
         const char* endPtr = colPtr + csvRows[rowIndex].size();
         const char* delimPos = strchr(colPtr, CSV_CELL_DELIM_CHAR); // Returns NULL if no match is found.

         // Keep looping until Num Columns is hit or no more delimiters exist.
         while(colIndex < numCol && delimPos)
         {
            double newValue = colPtr != delimPos ? strtod(colPtr, NULL) : NAN; // If cell is empty, set point to "Not A Number"
            params[colIndex].yOrigPoints.push_back(newValue);

            colPtr = delimPos + 1;
            delimPos = strchr(colPtr, CSV_CELL_DELIM_CHAR);
            ++colIndex;
         }

         // Handle last column that has no delmiter after it.
         if(colIndex < numCol)
         {
            double newValue = colPtr != endPtr ? strtod(colPtr, NULL) : NAN; // If cell is empty, set point to "Not A Number"
            params[colIndex].yOrigPoints.push_back(newValue);
         }
      }

      // Get curve names from first row.
      if(firstRowIsAllNums == false)
      {
         dString::SplitV(csvRows[0], CSV_CELL_DELIM, csvCells);
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
         params[i].mathProps.sampleRate = 0.0;
         params[i].numXMapOps = 0;
         params[i].mathProps.mathOpsXAxis.clear();
         params[i].numYMapOps = 0;
         params[i].mathProps.mathOpsYAxis.clear();
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







