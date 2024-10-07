/* Copyright 2014 - 2017, 2019, 2024 Dan Williams. All Rights Reserved.
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
#ifndef saveRestoreCurve_h
#define saveRestoreCurve_h

#include <vector>
#include <QWidget>
#include "DataTypes.h"
#include "CurveData.h"
#include "mainwindow.h"
#include "CurveCommander.h"

#define MAX_STORE_PLOT_CURVE_NAME_SIZE (100)

typedef std::vector<char> PackedCurveData;

typedef enum
{
   E_SAVE_RESTORE_RAW,
   E_SAVE_RESTORE_CSV,
   E_SAVE_RESTORE_CLIPBOARD_EXCEL,
   E_SAVE_RESTORE_C_HEADER_AUTO_TYPE,
   E_SAVE_RESTORE_C_HEADER_INT,
   E_SAVE_RESTORE_C_HEADER_FLOAT,
   E_SAVE_RESTORE_BIN_S8,  // E_CHAR,
   E_SAVE_RESTORE_BIN_U8,  // E_UCHAR,
   E_SAVE_RESTORE_BIN_S16, // E_INT_16,
   E_SAVE_RESTORE_BIN_U16, // E_UINT_16,
   E_SAVE_RESTORE_BIN_S32, // E_INT_32,
   E_SAVE_RESTORE_BIN_U32, // E_UINT_32,
   E_SAVE_RESTORE_BIN_S64, // E_INT_64,
   E_SAVE_RESTORE_BIN_U64, // E_UINT_64,
   E_SAVE_RESTORE_BIN_F32, // E_FLOAT_32,
   E_SAVE_RESTORE_BIN_F64, // E_FLOAT_64,
   E_SAVE_RESTORE_BIN_AUTO_TYPE,
   E_SAVE_RESTORE_INVALID
}eSaveRestorePlotCurveType;


typedef struct
{

   QString curveName;
   ePlotDim plotDim;
   ePlotType plotType;
   UINT_32 numPoints;

   UINT_32 numXMapOps;
   UINT_32 numYMapOps;
   tCurveMathProperties mathProps;

   dubVect xOrigPoints;
   dubVect yOrigPoints;

}tSaveRestoreCurveParams;

////////////////////////////////////////////////////////////////////////////////

class SaveCurve
{
public:
   SaveCurve(MainWindow* plotGui, CurveData* curve, eSaveRestorePlotCurveType type, bool limitToZoom = false);
   void getPackedData(PackedCurveData& packedDataReturn);

   PackedCurveData packedCurveHead;
   PackedCurveData packedCurveData;

   // Parameters set when 'type' is one of the Binary save/restore types (i.e. when 'SaveBinary' is used)
   unsigned int binary_dataTypeSize = 0;
   unsigned int binary_numPoints = 0;

   bool hasData() {return m_hasData;}
private:

   SaveCurve();
   SaveCurve(SaveCurve const&);
   void operator=(SaveCurve const&);

   void SaveRaw(CurveData* curve);
   void SaveExcel(MainWindow* plotGui, CurveData* curve, std::string delim);
   void SaveCHeader(MainWindow* plotGui, CurveData* curve, eSaveRestorePlotCurveType type);
   void SaveBinary(CurveData* curve, eSaveRestorePlotCurveType type);

   bool m_limitToZoom;
   bool m_hasData = false;
};

////////////////////////////////////////////////////////////////////////////////

class RestoreCurve
{
public:
   RestoreCurve(PackedCurveData &packedCurve);

   tSaveRestoreCurveParams params;

   bool isValid;

private:
   RestoreCurve();
   RestoreCurve(RestoreCurve const&);
   void operator=(RestoreCurve const&);


   void unpack(void* toUnpack, size_t unpackSize);
   char* m_packedArray;
   size_t m_packedSize;


};

////////////////////////////////////////////////////////////////////////////////

class SavePlot
{
public:
   SavePlot(MainWindow* plotGui, QString plotName, QVector<CurveData*>& plotInfo, eSaveRestorePlotCurveType type, bool limitToZoom = false);
   void getPackedData(PackedCurveData& packedDataReturn);

   PackedCurveData packedCurveHead;
   PackedCurveData packedCurveData;
private:

   SavePlot();
   SavePlot(SavePlot const&);
   void operator=(SavePlot const&);

   void SaveRaw(MainWindow* plotGui, QString plotName, QVector<CurveData*>& plotInfo);
   void SaveExcel(MainWindow* plotGui, QVector<CurveData*>& plotInfo, eSaveRestorePlotCurveType type);
   void SaveCHeader(MainWindow* plotGui, QVector<CurveData*>& plotInfo, eSaveRestorePlotCurveType type);
   void SaveBinary(MainWindow* plotGui, QVector<CurveData*>& plotInfo, eSaveRestorePlotCurveType type);

   bool m_limitToZoom;
};

////////////////////////////////////////////////////////////////////////////////

class RestorePlot
{
public:
   RestorePlot(PackedCurveData &packedPlot);

   QString plotName;
   QVector<tSaveRestoreCurveParams> params;

   bool isValid;

private:
   RestorePlot();
   RestorePlot(RestorePlot const&);
   void operator=(RestorePlot const&);
};

////////////////////////////////////////////////////////////////////////////////

class RestoreCsv
{
public:
   RestoreCsv(PackedCurveData &packedPlot);

   QVector<tSaveRestoreCurveParams> params;

   bool isValid;
   bool hasBadCells;

private:
   RestoreCsv();
   RestoreCsv(RestoreCsv const&);
   void operator=(RestoreCsv const&);
};

////////////////////////////////////////////////////////////////////////////////

class CurveCommander;

class SavePlotCurveDialog
{
public:
   SavePlotCurveDialog(QWidget* widgetParent, CurveCommander* curveCmdr, bool limitToZoom)
      : m_widgetParent(widgetParent), m_curveCmdr(curveCmdr), m_limitToZoom(limitToZoom){}

   bool saveCurve(const QString& plotName, const QString& curveName); // Returns whether the file was saved or not.
   bool savePlot(const QString& plotName); // Returns whether the file was saved or not.

private:
   SavePlotCurveDialog();
   SavePlotCurveDialog(SavePlotCurveDialog const&);
   void operator=(SavePlotCurveDialog const&);

private:
   QWidget* m_widgetParent;
   CurveCommander* m_curveCmdr;
   bool m_limitToZoom;

   eSaveRestorePlotCurveType parseSaveFileName(QString& pathInOut, const QString& selectedFilter);
};

#endif
