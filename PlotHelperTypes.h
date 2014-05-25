/* Copyright 2013 - 2014 Dan Williams. All Rights Reserved.
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
#ifndef PlotHelperTypes_h
#define PlotHelperTypes_h

#include <vector>


#include <QString>
#include <QList>
#include <QVector>
#include <QString>
#include <QDateTime>


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define STRING_STREAM_CLEAR_PRECISION_VAL (-1)

typedef std::vector<double> dubVect;

typedef struct tMaxMinXY
{
    double minX;
    double minY;
    double maxX;
    double maxY;

    bool operator==(const struct tMaxMinXY& rhs)
    {
        return (minX == rhs.minX) &&
               (minY == rhs.minY) &&
               (maxX == rhs.maxX) &&
               (maxY == rhs.maxY);
    }

}maxMinXY;

typedef enum
{
    E_PLOT_DIM_1D,
    E_PLOT_DIM_2D
}ePlotDim;

typedef enum
{
    E_PLOT_TYPE_1D          = 0,
    E_PLOT_TYPE_2D          = 1,
    E_PLOT_TYPE_REAL_FFT    = 2,
    E_PLOT_TYPE_COMPLEX_FFT = 3
}ePlotType;

typedef struct
{
    double m;
    double b;
}tLinear;

typedef struct
{
    tLinear xAxis;
    tLinear yAxis;
}tLinearXYAxis;

typedef enum
{
    E_FFT_REAL,
    E_FFT_COMPLEX
}eFFTType;

typedef enum
{
    E_X_AXIS,
    E_Y_AXIS
}eAxis;


typedef struct
{
    QString plotName;
    QString curveName;

    eFFTType fftType;

    QString srcRePlotName;
    QString srcReCurveName;
    eAxis   srcReAxis;

    QString srcImPlotName;
    QString srcImCurveName;
    eAxis   srcImAxis;
}tFFTCurve;

typedef struct
{
   QString plotName;
   QString curveName;
   eAxis axis;
}tPlotCurveAxis;

typedef struct
{
   tPlotCurveAxis dataSrc;
   int startIndex;
   int stopIndex;
}tParentCurveInfo;

typedef enum
{
   E_ADD,
   E_SUBTRACT,
   E_MULTIPLY,
   E_DIVIDE,
   E_SHIFT_UP,
   E_SHIFT_DOWN,
   E_LOG
}eMathOp;

typedef struct
{
   eMathOp op;
   double num;
}tOperation;

typedef std::list<tOperation> tMathOpList;

typedef enum
{
    E_DISPLAY_POINT_AUTO,
    E_DISPLAY_POINT_FIXED,
    E_DISPLAY_POINT_SCIENTIFIC
}eDisplayPointType;

typedef struct
{
   QString plotName;
   QString curveName;
   const char* msgPtr;
   unsigned int msgSize;
   QDateTime msgTime;
}tStoredMsg;

#endif

