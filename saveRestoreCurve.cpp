/* Copyright 2014 - 2017, 2019, 2021 - 2024 Dan Williams. All Rights Reserved.
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
#include <memory>
#include <QString>
#include <QStringList>
#include <QFileDialog>
#include <QMessageBox>
#include <algorithm>
#include "saveRestoreCurve.h"
#include "persistentParameters.h"
#include "dString.h"
#include "FileSystemOperations.h"

static const std::string EXCEL_LINE_DELIM = "\r\n";
static const std::string CSV_CELL_DELIM = ",";
static const char        CSV_CELL_DELIM_CHAR = ',';
static const std::string CLIPBOARD_EXCEL_CELL_DELIM = "\t";

static const std::string C_HEADER_LINE_DELIM = "\r\n";

const QString OPEN_SAVE_FILTER_PLOT_STR = "Plots (*.plot)";
const QString OPEN_SAVE_FILTER_CURVE_STR = "Curves (*.curve)";
const QString OPEN_SAVE_FILTER_CSV_STR = "Comma Separated Values (*.csv)";
const QString OPEN_SAVE_FILTER_C_HEADER_AUTO_TYPE_STR = "C Header - Auto Type (*.h)";
const QString OPEN_SAVE_FILTER_C_HEADER_INT_STR = "C Header - Integer (*.h)";
const QString OPEN_SAVE_FILTER_C_HEADER_FLOAT_STR = "C Header - Float (*.h)";
const QString OPEN_SAVE_FILTER_BINARY_S8_STR   = "Raw Binary - Signed Int 8 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_S16_STR  = "Raw Binary - Signed Int 16 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_S32_STR  = "Raw Binary - Signed Int 32 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_S64_STR  = "Raw Binary - Signed Int 64 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_U64_STR  = "Raw Binary - Unsigned Int 64 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_U32_STR  = "Raw Binary - Unsigned Int 32 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_U16_STR  = "Raw Binary - Unsigned Int 16 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_U8_STR   = "Raw Binary - Unsigned Int 8 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_F32_STR  = "Raw Binary - Float 32 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_F64_STR  = "Raw Binary - Float 64 Bit (*.*)";
const QString OPEN_SAVE_FILTER_BINARY_AUTO_STR = "Raw Binary - Auto Type (*.*)";

const QString OPEN_SAVE_FILTER_DELIM = ";;";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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

static ePlotDataTypes getSaveDataType(eSaveRestorePlotCurveType type, ePlotDataTypes xAxis, ePlotDataTypes yAxis)
{
   ePlotDataTypes retVal = E_INVALID_DATA_TYPE;
   switch(type)
   {
      case E_SAVE_RESTORE_C_HEADER_INT: // Specific data type.
         retVal = E_INT_64;
      break;
      case E_SAVE_RESTORE_C_HEADER_FLOAT: // Specific data type.
         retVal = E_FLOAT_64;
      break;
      case E_SAVE_RESTORE_BIN_S8:  // E_CHAR,
         retVal = E_CHAR;
      break;
      case E_SAVE_RESTORE_BIN_U8:  // E_UCHAR,
         retVal = E_UCHAR;
      break;
      case E_SAVE_RESTORE_BIN_S16: // E_INT_16,
         retVal = E_INT_16;
      break;
      case E_SAVE_RESTORE_BIN_U16: // E_UINT_16,
         retVal = E_UINT_16;
      break;
      case E_SAVE_RESTORE_BIN_S32: // E_INT_32,
         retVal = E_INT_32;
      break;
      case E_SAVE_RESTORE_BIN_U32: // E_UINT_32,
         retVal = E_UINT_32;
      break;
      case E_SAVE_RESTORE_BIN_S64: // E_INT_64,
         retVal = E_INT_64;
      break;
      case E_SAVE_RESTORE_BIN_U64: // E_UINT_64,
         retVal = E_UINT_64;
      break;
      case E_SAVE_RESTORE_BIN_F32: // E_FLOAT_32,
         retVal = E_FLOAT_32;
      break;
      case E_SAVE_RESTORE_BIN_F64: // E_FLOAT_64,
         retVal = E_FLOAT_64;
      break;
      default: // Auto-data type.
      {
         if(xAxis != yAxis && xAxis != E_INVALID_DATA_TYPE)
         {
            // 2D plot, but X axis type doesn't match Y axis type. Just use double to be sure.
            retVal = E_FLOAT_64;
         }
         else
         {
            retVal = yAxis; // Auto-data type. Use y-axis type.
         }
      }
      break;
   }
   return retVal;
}

static std::string getDataStrName(ePlotDataTypes dataType, bool& isInt)
{
   std::string typeStr;
   isInt = true;
   switch(dataType)
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
   return typeStr;
}

static std::string getCHeaderTypedefStr(QString plotName, QString curveName, std::string dataTypeStr)
{
   std::string typeNameStr = getPlotNameCHeaderTypeName(plotName, curveName);
   return "#ifndef " + typeNameStr + C_HEADER_LINE_DELIM +
          "#define " + typeNameStr + " " + dataTypeStr + C_HEADER_LINE_DELIM +
          "#endif" + C_HEADER_LINE_DELIM;
}

template <typename T>
unsigned getRawCurveBytes(const double* inX, const double* inY, size_t numPoints, PackedCurveData& out)
{
   unsigned dataTypeSize = 0;
   if(inX != nullptr && inY != nullptr)
   {
      // 2D (i.e. both X an Y axes have data)
      dataTypeSize = 2 * sizeof(T);
      out.resize(dataTypeSize*numPoints);
      T* ptr = (T*)out.data();
      for(size_t i = 0; i < numPoints; ++i)
      {
         ptr[2*i+0] = (T)inX[i];
         ptr[2*i+1] = (T)inY[i];
      }
   }
   else if(inY != nullptr)
   {
      // 1D (i.e. only Y axis has data)
      dataTypeSize = sizeof(T);
      out.resize(dataTypeSize*numPoints);
      T* ptr = (T*)out.data();
      for(size_t i = 0; i < numPoints; ++i)
      {
         ptr[i] = (T)inY[i];
      }
   }
   return dataTypeSize;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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
      case E_SAVE_RESTORE_BIN_AUTO_TYPE:
      case E_SAVE_RESTORE_BIN_S8:  // E_CHAR,
      case E_SAVE_RESTORE_BIN_U8:  // E_UCHAR,
      case E_SAVE_RESTORE_BIN_S16: // E_INT_16,
      case E_SAVE_RESTORE_BIN_U16: // E_UINT_16,
      case E_SAVE_RESTORE_BIN_S32: // E_INT_32,
      case E_SAVE_RESTORE_BIN_U32: // E_UINT_32,
      case E_SAVE_RESTORE_BIN_S64: // E_INT_64,
      case E_SAVE_RESTORE_BIN_U64: // E_UINT_64,
      case E_SAVE_RESTORE_BIN_F32: // E_FLOAT_32,
      case E_SAVE_RESTORE_BIN_F64: // E_FLOAT_64,
         SaveBinary(curve, type);
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
    m_hasData = (params.numPoints > 0);

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
   if(curve->getPlotDim() == E_PLOT_DIM_2D) ////// 2D --------------------------
   {
      // Determine the points to use
      const double* xPoints = nullptr;
      const double* yPoints = nullptr;
      unsigned int numPoints = 0;
      dubVect xPoints_zoom;
      dubVect yPoints_zoom;
      
      if(!m_limitToZoom)
      {
         // Not limiting the points to the zoom, so get all the samples.
         xPoints = curve->getXPoints();
         yPoints = curve->getYPoints();
         numPoints = curve->getNumPoints();
      }
      else
      {
         // Limit the samples based on the current zoom. 
         curve->get2dDisplayedPoints(xPoints_zoom, yPoints_zoom);
         xPoints = xPoints_zoom.data();
         yPoints = yPoints_zoom.data();
         numPoints = (unsigned int)std::min(xPoints_zoom.size(), yPoints_zoom.size());
      }

      // Detemine if this curve has data to save.
      m_hasData = (numPoints > 0);

      // Build the CSV File
      csvFile << curve->getCurveTitle().toStdString() << " - X Axis" << delim;
      csvFile << curve->getCurveTitle().toStdString() << " - Y Axis\r\n";
      for(unsigned int i = 0; i < numPoints; ++i)
      {
         plotGui->setDisplayIoMapipXAxis(csvFile, curve);
         csvFile << xPoints[i] << delim;
         plotGui->setDisplayIoMapipYAxis(csvFile);
         csvFile << yPoints[i] << EXCEL_LINE_DELIM;
      }
      plotGui->clearDisplayIoMapIp(csvFile);
   }
   else ////// 1D --------------------------
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

      // Detemine if this curve has data to save.
      m_hasData = (stopExclusive > startInclusive);
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
   m_hasData = (numSamplesToWrite > 0);

   // Determine the data type string (i.e. long, double, unsigned char, etc)
   bool dataType_isInt = false;
   ePlotDataTypes dataType_type = getSaveDataType(
      type,
      curve->getPlotDim() == E_PLOT_DIM_1D ? E_INVALID_DATA_TYPE : curve->getLastMsgDataType(E_X_AXIS),
      curve->getLastMsgDataType(E_Y_AXIS));
   std::string dataType_str = getDataStrName(dataType_type, dataType_isInt);

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

void SaveCurve::SaveBinary(CurveData* curve, eSaveRestorePlotCurveType type)
{
   bool is2D = (curve->getPlotDim() == E_PLOT_DIM_2D);

   // Determine the data type string (i.e. long, double, unsigned char, etc)
   ePlotDataTypes dataType_type = getSaveDataType(
      type,
      is2D ? curve->getLastMsgDataType(E_X_AXIS) : E_INVALID_DATA_TYPE,
      curve->getLastMsgDataType(E_Y_AXIS));

   // Parameters for keeping track of the sample to save.
   unsigned int numPoints = 0;
   const double* xPoints = nullptr;
   const double* yPoints = nullptr;
   dubVect xPoints_zoom; // needed if m_limitToZoom is true
   dubVect yPoints_zoom; // needed if m_limitToZoom is true

   // Determine which samples to save.
   if(!m_limitToZoom)
   {
      // Not limiting the points to the zoom, so get all the samples.
      if(is2D) // xPoints only needs to be set for 2D plots. It should stay nullptr for 1D plots.
         xPoints = curve->getXPoints();
      yPoints = curve->getYPoints();
      numPoints = curve->getNumPoints();
   }
   else if(is2D)
   {
      // 2D - Limit the samples based on the current zoom. 
      curve->get2dDisplayedPoints(xPoints_zoom, yPoints_zoom); // 2D samples are scattered around, so new memory needs to be used to fill in the samples that are in the current zoom.
      xPoints = xPoints_zoom.data();
      yPoints = yPoints_zoom.data();
      numPoints = (unsigned int)std::min(xPoints_zoom.size(), yPoints_zoom.size());
   }
   else
   {
      // 1D - Limit the samples based on the current zoom.
      unsigned int startInclusive = 0;
      unsigned int stopExclusive = curve->getNumPoints();
      maxMinXY displayed = curve->get1dDisplayedIndexes();
      displayed.minX += 1; // Min is the sample before the first sample displayed.

      if(displayed.minX > startInclusive)
         startInclusive = displayed.minX;
      if(displayed.maxX < stopExclusive)
         stopExclusive = displayed.maxX;
      numPoints = (stopExclusive >= startInclusive) ? (stopExclusive - startInclusive) : 0;
      yPoints = (numPoints > 0) ? curve->getYPoints()+startInclusive : nullptr;
   }

   // Store the samples as binary data in the data format specified by 'dataType_type'
   switch(dataType_type)
   {
      case E_CHAR:
         binary_dataTypeSize = getRawCurveBytes<int8_t>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      case E_UCHAR:
         binary_dataTypeSize = getRawCurveBytes<uint8_t>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      case E_INT_16:
         binary_dataTypeSize = getRawCurveBytes<int16_t>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      case E_UINT_16:
         binary_dataTypeSize = getRawCurveBytes<uint16_t>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      case E_INT_32:
         binary_dataTypeSize = getRawCurveBytes<int32_t>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      case E_UINT_32:
         binary_dataTypeSize = getRawCurveBytes<uint32_t>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      case E_INT_64:
         binary_dataTypeSize = getRawCurveBytes<int64_t>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      case E_UINT_64:
         binary_dataTypeSize = getRawCurveBytes<uint64_t>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      case E_FLOAT_32:
         binary_dataTypeSize = getRawCurveBytes<float>(xPoints, yPoints, numPoints, packedCurveData);
      break;
      default:
         binary_dataTypeSize = getRawCurveBytes<double>(xPoints, yPoints, numPoints, packedCurveData);
      break;
   }

   // Save info about the number of points.
   m_hasData = (numPoints > 0);
   binary_numPoints = numPoints;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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
      case E_SAVE_RESTORE_BIN_AUTO_TYPE:
      case E_SAVE_RESTORE_BIN_S8:  // E_CHAR,
      case E_SAVE_RESTORE_BIN_U8:  // E_UCHAR,
      case E_SAVE_RESTORE_BIN_S16: // E_INT_16,
      case E_SAVE_RESTORE_BIN_U16: // E_UINT_16,
      case E_SAVE_RESTORE_BIN_S32: // E_INT_32,
      case E_SAVE_RESTORE_BIN_U32: // E_UINT_32,
      case E_SAVE_RESTORE_BIN_S64: // E_INT_64,
      case E_SAVE_RESTORE_BIN_U64: // E_UINT_64,
      case E_SAVE_RESTORE_BIN_F32: // E_FLOAT_32,
      case E_SAVE_RESTORE_BIN_F64: // E_FLOAT_64,
         SaveBinary(plotGui, plotInfo, type);
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
   QVector<ePlotDim> curvePlotDim;
   for(int i = 0; i < plotInfo.size(); ++i)
   {
      SaveCurve curveFile(plotGui, plotInfo[i], type, m_limitToZoom);
      if(curveFile.hasData())
      {
         curveFile.packedCurveData.push_back('\0'); // Null Terminate to make the char array a string.
         curveCsvFiles.push_back(QString(&curveFile.packedCurveData[0]).split(EXCEL_LINE_DELIM.c_str(), Qt::SkipEmptyParts));
         curvePlotDim.push_back(plotInfo[i]->getPlotDim());
      }
   }

   int maxNumLines = 0;
   for(int curveIndex = 0; curveIndex < curveCsvFiles.size(); ++curveIndex)
   {
      if(curveCsvFiles[curveIndex].size() > maxNumLines)
      {
         maxNumLines = curveCsvFiles[curveIndex].size();
      }
   }

   QString packed;
   for(int lineIndex = 0; lineIndex < maxNumLines; ++lineIndex)
   {
      int numCurves = curveCsvFiles.size();
      for(int curveIndex = 0; curveIndex < numCurves; ++curveIndex)
      {
         bool lastColumn = (curveIndex >= (numCurves-1));
         if(curveCsvFiles[curveIndex].size() > lineIndex)
         {
            packed.append(curveCsvFiles[curveIndex][lineIndex]);
         }
         else if((curvePlotDim[curveIndex] == E_PLOT_DIM_2D) && !lastColumn)
         {
            packed.append(delim.c_str()); // No data for this 2D curve, but more curves are available for this row. Add extra demin to make this 2D.
         }

         if(!lastColumn)
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

void SavePlot::SaveBinary(MainWindow* plotGui, QVector<CurveData*>& plotInfo, eSaveRestorePlotCurveType type)
{
   packedCurveHead.clear(); // No Header. This is a raw binary file.
   packedCurveData.clear(); // Clear data (it will be modified later in this function)

   // Generate all the raw bytes for each curve.
   std::vector<std::shared_ptr<SaveCurve>> saveCurves;
   std::vector<char*> saveCurvesRawBytesPtr;
   std::vector<unsigned> saveCurvesTypeSize;
   unsigned numPoints = 0;
   unsigned numBytesPerPoint = 0;
   for(int i = 0; i < plotInfo.size(); ++i)
   {
      saveCurves.emplace_back(std::make_shared<SaveCurve>(plotGui, plotInfo[i], type, m_limitToZoom));
      saveCurvesRawBytesPtr.emplace_back(saveCurves[i]->hasData() ? saveCurves[i]->packedCurveData.data() : nullptr);
      saveCurvesTypeSize.emplace_back(saveCurves[i]->hasData() ? saveCurves[i]->binary_dataTypeSize : 0); // If no data, this should be zero.
      if(saveCurves[i]->hasData())
      {
         if(numPoints == 0)
            numPoints = saveCurves[i]->binary_numPoints; // Initialize with the first curve that has some points.
         else
            numPoints = std::min(numPoints, saveCurves[i]->binary_numPoints); // Save the number of points that are available in all curves.
         numBytesPerPoint += saveCurvesTypeSize[i];
      }
   }

   // Interleave all the curve data points.
   packedCurveData.resize(numBytesPerPoint*numPoints);
   auto packedDataPtr = packedCurveData.data();
   unsigned writeByte = 0;
   for(unsigned pointIndex = 0; pointIndex < numPoints; ++pointIndex)
   {
      for(int curveIndex = 0; curveIndex < plotInfo.size(); ++curveIndex)
      {
         auto curveMemberSize = saveCurvesTypeSize[curveIndex];
         memcpy(&packedDataPtr[writeByte], saveCurvesRawBytesPtr[curveIndex], curveMemberSize);
         writeByte += curveMemberSize;
         saveCurvesRawBytesPtr[curveIndex] += curveMemberSize;
      }
   }
   assert(writeByte == (numBytesPerPoint*numPoints));

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


bool SavePlotCurveDialog::saveCurve(const QString& plotName, const QString& curveName)
{
   bool fileWritten = false;
   CurveData* toSaveCurveData = m_curveCmdr->getCurveData(plotName, curveName);
   MainWindow* plotGui = m_curveCmdr->getMainPlot(plotName);

   if(toSaveCurveData != NULL)
   {
      // Use the last saved location to determine the folder to save the curve to.
      QString suggestedSavePath = m_curveCmdr->getOpenSavePath(curveName);

      // Read the last used filter from persistent memory.
      std::string persistentSaveStr = PERSIST_PARAM_CURVE_SAVE_PREV_SAVE_SELECTION_STR;
      std::string persistentReadValue;
      persistentParam_getParam_str(persistentSaveStr, persistentReadValue);

      // Generate Filter List string (i.e. the File Save types)
      QStringList filterList;
      if(!m_limitToZoom) // OPEN_SAVE_FILTER_CURVE_STR doesn't support "Limit to Zoom"
         filterList.append(OPEN_SAVE_FILTER_CURVE_STR);
      filterList.append(OPEN_SAVE_FILTER_CSV_STR);
      if(!m_limitToZoom) // OPEN_SAVE_FILTER_C_HEADER_* doesn't support "Limit to Zoom"
      {
         filterList.append(OPEN_SAVE_FILTER_C_HEADER_AUTO_TYPE_STR);
         filterList.append(OPEN_SAVE_FILTER_C_HEADER_INT_STR);
         filterList.append(OPEN_SAVE_FILTER_C_HEADER_FLOAT_STR);
      }
      filterList.append(OPEN_SAVE_FILTER_BINARY_S8_STR  );
      filterList.append(OPEN_SAVE_FILTER_BINARY_S16_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_S32_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_S64_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_U8_STR  );
      filterList.append(OPEN_SAVE_FILTER_BINARY_U16_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_U32_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_U64_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_F32_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_F64_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_AUTO_STR);
      QString filterString = filterList.join(OPEN_SAVE_FILTER_DELIM);

      // Initialize selection with stored value (if there was one).
      QString selectedFilter;
      if(filterList.contains(persistentReadValue.c_str()))
         selectedFilter = persistentReadValue.c_str();

      // Open the save file dialog.
      QString dialogCaption = QString("Save Curve To File") + QString(m_limitToZoom ? " (Only Points in Current Zoom)" : "");
      QString fileName = QFileDialog::getSaveFileName(m_widgetParent,
                                                      QObject::tr(dialogCaption.toStdString().c_str()),
                                                      suggestedSavePath,
                                                      filterString,
                                                      &selectedFilter,
                                                      QFileDialog::DontConfirmOverwrite);

      eSaveRestorePlotCurveType saveType = parseSaveFileName(fileName, selectedFilter);

      if(fileName != "") // Check for 'Cancel' case.
      {
         // Write user selections to persisent memory.
         m_curveCmdr->setOpenSavePath(fileName);
         persistentParam_setParam_str(persistentSaveStr, selectedFilter.toStdString());
         persistentParam_setParam_f64(PERSIST_PARAM_CURVE_SAVE_PREV_SAVE_SELECTION_INDEX, saveType);

         if(saveType != E_SAVE_RESTORE_INVALID)
         {
            SaveCurve packedCurve(plotGui, toSaveCurveData, saveType);
            PackedCurveData dataToWriteToFile;
            packedCurve.getPackedData(dataToWriteToFile);
            fso::WriteFile(fileName.toStdString(), &dataToWriteToFile[0], dataToWriteToFile.size());

            fileWritten = true;
         }
      }
   }
   return fileWritten;
}

bool SavePlotCurveDialog::savePlot(const QString& plotName)
{
   bool fileWritten = false;
   tCurveCommanderInfo allPlots = m_curveCmdr->getCurveCommanderInfo();

   if(allPlots.find(plotName) != allPlots.end())
   {
      // Use the last saved location to determine the folder to save the curve to.
      QString suggestedSavePath = m_curveCmdr->getOpenSavePath(plotName);

      // Read the last used filter from persistent memory.
      std::string persistentSaveStr = PERSIST_PARAM_PLOT_SAVE_PREV_SAVE_SELECTION_STR;
      std::string persistentReadValue;
      persistentParam_getParam_str(persistentSaveStr, persistentReadValue);

      // Generate Filter List string (i.e. the File Save types)
      QStringList filterList;
      if(!m_limitToZoom) // OPEN_SAVE_FILTER_PLOT_STR doesn't support "Limit to Zoom"
         filterList.append(OPEN_SAVE_FILTER_PLOT_STR);
      filterList.append(OPEN_SAVE_FILTER_CSV_STR);
      if(!m_limitToZoom) // OPEN_SAVE_FILTER_C_HEADER_* doesn't support "Limit to Zoom"
      {
         filterList.append(OPEN_SAVE_FILTER_C_HEADER_AUTO_TYPE_STR);
         filterList.append(OPEN_SAVE_FILTER_C_HEADER_INT_STR);
         filterList.append(OPEN_SAVE_FILTER_C_HEADER_FLOAT_STR);
      }
      filterList.append(OPEN_SAVE_FILTER_BINARY_S8_STR  );
      filterList.append(OPEN_SAVE_FILTER_BINARY_S16_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_S32_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_S64_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_U8_STR  );
      filterList.append(OPEN_SAVE_FILTER_BINARY_U16_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_U32_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_U64_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_F32_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_F64_STR );
      filterList.append(OPEN_SAVE_FILTER_BINARY_AUTO_STR);
      QString filterString = filterList.join(OPEN_SAVE_FILTER_DELIM);

      // Initialize selection with stored value (if there was one).
      QString selectedFilter;
      if(filterList.contains(persistentReadValue.c_str()))
         selectedFilter = persistentReadValue.c_str();

      // Open the save file dialog.
      QString dialogCaption = QString("Save Plot To File") + QString(m_limitToZoom ? " (Only Points in Current Zoom)" : "");
      QString fileName = QFileDialog::getSaveFileName(m_widgetParent, 
                                                      QObject::tr(dialogCaption.toStdString().c_str()),
                                                      suggestedSavePath,
                                                      filterString,
                                                      &selectedFilter,
                                                      QFileDialog::DontConfirmOverwrite);

      eSaveRestorePlotCurveType saveType = parseSaveFileName(fileName, selectedFilter);

      if(fileName != "") // Check for 'Cancel' case.
      {
         // Fill in vector of curve data in the correct order.
         QVector<CurveData*> curves;
         curves.resize(allPlots[plotName].curves.size());
         foreach( QString key, allPlots[plotName].curves.keys() )
         {
            int index = allPlots[plotName].plotGui->getCurveIndex(key);
            curves[index] = allPlots[plotName].curves[key];
         }

         // Write user selections to persisent memory.
         m_curveCmdr->setOpenSavePath(fileName);
         persistentParam_setParam_str(persistentSaveStr, selectedFilter.toStdString());
         persistentParam_setParam_f64(PERSIST_PARAM_PLOT_SAVE_PREV_SAVE_SELECTION_INDEX, saveType);

         if(saveType != E_SAVE_RESTORE_INVALID)
         {
            SavePlot savePlot(allPlots[plotName].plotGui, plotName, curves, saveType, true);
            PackedCurveData dataToWriteToFile;
            savePlot.getPackedData(dataToWriteToFile);
            fso::WriteFile(fileName.toStdString(), &dataToWriteToFile[0], dataToWriteToFile.size());

            fileWritten = true;
         }
      }
   }
   return fileWritten;
}

eSaveRestorePlotCurveType SavePlotCurveDialog::parseSaveFileName(QString& pathInOut, const QString& selectedFilter)
{
   // If path is empty, cancel or some equivalent was pressed. Return right away in that case.
   if(pathInOut == "")
      return E_SAVE_RESTORE_INVALID;

   // Use selectedFilter to determine the file format to save.
   eSaveRestorePlotCurveType saveType = E_SAVE_RESTORE_INVALID;
   QString ext = "";
   if(selectedFilter == OPEN_SAVE_FILTER_PLOT_STR)
   {
      saveType = E_SAVE_RESTORE_RAW;
      ext = ".plot";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_CURVE_STR)
   {
      saveType = E_SAVE_RESTORE_RAW;
      ext = ".curve";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_CSV_STR)
   {
      saveType = E_SAVE_RESTORE_CSV;
      ext = ".csv";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_C_HEADER_AUTO_TYPE_STR)
   {
      saveType = E_SAVE_RESTORE_C_HEADER_AUTO_TYPE;
      ext = ".h";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_C_HEADER_INT_STR)
   {
      saveType = E_SAVE_RESTORE_C_HEADER_INT;
      ext = ".h";
   }
   else if(selectedFilter == OPEN_SAVE_FILTER_C_HEADER_FLOAT_STR)
   {
      saveType = E_SAVE_RESTORE_C_HEADER_FLOAT;
      ext = ".h";
   }
   // Raw Binary Save types - No Extensions
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_S8_STR)
      saveType = E_SAVE_RESTORE_BIN_S8;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_U8_STR)
      saveType = E_SAVE_RESTORE_BIN_U8;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_S16_STR)
      saveType = E_SAVE_RESTORE_BIN_S16;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_U16_STR)
      saveType = E_SAVE_RESTORE_BIN_U16;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_S32_STR)
      saveType = E_SAVE_RESTORE_BIN_S32;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_U32_STR)
      saveType = E_SAVE_RESTORE_BIN_U32;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_S64_STR)
      saveType = E_SAVE_RESTORE_BIN_S64;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_U64_STR)
      saveType = E_SAVE_RESTORE_BIN_U64;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_F32_STR)
      saveType = E_SAVE_RESTORE_BIN_F32;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_F64_STR)
      saveType = E_SAVE_RESTORE_BIN_F64;
   else if(selectedFilter == OPEN_SAVE_FILTER_BINARY_AUTO_STR)
      saveType = E_SAVE_RESTORE_BIN_AUTO_TYPE;

   if(saveType != E_SAVE_RESTORE_INVALID) // Attempt to update pathInOut, but only if saveType is a valid File Format.
   {
      // Check if the correct file extension needs to be added to the save path (this seems to be needed in Linux)
      if(pathInOut.size() < ext.size())
      {
         pathInOut += ext; // path is too short to be the extension, so just add the extension to the end
      }
      else
      {
         // Check if the correct extension is at the end of the save path, if not fix the save path.
         QString pathEnd = pathInOut.right(ext.size());
         if(pathEnd != ext)
         {
            if(pathEnd.toLower() == ext)
            {
               pathInOut = pathInOut.left(pathInOut.size()-ext.size()); // Extension matches but is the wrong case, remove extension (it will be added back in the line below).
            }
            pathInOut += ext;
         }
      }
   }
   
   if(fso::FileExists(pathInOut.toStdString()))
   {
      // A file with this path name already exists. Ask the user if it should be overwritten.
      QMessageBox::StandardButton reply;
      reply = QMessageBox::question( m_widgetParent,
                                     "Confirm Save As!",
                                     pathInOut + " already exists.\nDo you want to replace it?",
                                     QMessageBox::Yes|QMessageBox::No );
      if (reply != QMessageBox::Yes)
      {
         // User doesn't want to replace the existing file.
         pathInOut = "";
         saveType = E_SAVE_RESTORE_INVALID;
      }
   }

   return saveType;
}





