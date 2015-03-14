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
#ifndef saveRestoreCurve_h
#define saveRestoreCurve_h

#include <vector>
#include "DataTypes.h"
#include "CurveData.h"
#include "mainwindow.h"
#include "CurveCommander.h"

#define MAX_STORE_CURVE_NAME_SIZE (100)

typedef std::vector<char> PackedCurveData;

typedef enum
{
    E_SAVE_RESTORE_RAW,
    E_SAVE_RESTORE_CSV
}eSaveRestorePlotCurveType;

typedef struct
{

   QString curveName;
   ePlotDim plotDim;
   ePlotType plotType;
   UINT_32 numPoints;
   double sampleRate;

   UINT_32 numXMapOps;
   tMathOpList mathOpsXAxis;

   UINT_32 numYMapOps;
   tMathOpList mathOpsYAxis;

   dubVect xOrigPoints;
   dubVect yOrigPoints;

}tSaveRestoreCurveParams;

class SaveCurve
{
public:
   SaveCurve(MainWindow* plotGui, CurveData* curve, eSaveRestorePlotCurveType type);

   PackedCurveData packedCurveData;
private:

   tSaveRestoreCurveParams params;

   SaveCurve();
   SaveCurve(SaveCurve const&);
   void operator=(SaveCurve const&);

   void SaveRaw(CurveData* curve);
   void SaveCsv(MainWindow* plotGui, CurveData* curve);
};

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

class SavePlot
{
public:
   SavePlot(QString plotName, tPlotGuiCurveInfo& plotInfo, eSaveRestorePlotCurveType type);

   PackedCurveData packedCurveData;
private:

   SavePlot();
   SavePlot(SavePlot const&);
   void operator=(SavePlot const&);

   void SaveRaw(QString plotName, tPlotGuiCurveInfo& plotInfo);
   void SaveCsv(tPlotGuiCurveInfo& plotInfo);
};

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


#endif
