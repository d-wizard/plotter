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


const QString PLOT_CURVE_SEP = "->";

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define STRING_STREAM_CLEAR_PRECISION_VAL (-1)

typedef std::vector<double> dubVect;


typedef struct
{
   QString plot;
   QString curve;
}tPlotCurveName;



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
inline bool valid_ePlotDim(ePlotDim in)
{
   switch(in)
   {
   case E_PLOT_DIM_1D:
   case E_PLOT_DIM_2D:
      return true;
      break;
   }
   return false;
}

typedef enum
{
   E_PLOT_TYPE_1D,
   E_PLOT_TYPE_2D,
   E_PLOT_TYPE_REAL_FFT,
   E_PLOT_TYPE_COMPLEX_FFT,
   E_PLOT_TYPE_AM_DEMOD,
   E_PLOT_TYPE_FM_DEMOD,
   E_PLOT_TYPE_PM_DEMOD,
   E_PLOT_TYPE_AVERAGE,
   E_PLOT_TYPE_DB_POWER_FFT_REAL,
   E_PLOT_TYPE_DB_POWER_FFT_COMPLEX
}ePlotType;

inline bool valid_ePlotType(ePlotType in)
{
   switch(in)
   {
   case E_PLOT_TYPE_1D:
   case E_PLOT_TYPE_2D:
   case E_PLOT_TYPE_REAL_FFT:
   case E_PLOT_TYPE_COMPLEX_FFT:
   case E_PLOT_TYPE_AM_DEMOD:
   case E_PLOT_TYPE_FM_DEMOD:
   case E_PLOT_TYPE_PM_DEMOD:
   case E_PLOT_TYPE_AVERAGE:
   case E_PLOT_TYPE_DB_POWER_FFT_REAL:
   case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
      return true;
      break;
   }
   return false;
}

inline bool plotTypeHas2DInput(ePlotType in)
{
   bool twoDInput = false;
   switch(in)
   {
   case E_PLOT_TYPE_2D:
   case E_PLOT_TYPE_COMPLEX_FFT:
   case E_PLOT_TYPE_AM_DEMOD:
   case E_PLOT_TYPE_FM_DEMOD:
   case E_PLOT_TYPE_PM_DEMOD:
   case E_PLOT_TYPE_DB_POWER_FFT_COMPLEX:
      twoDInput = true;
      break;

   case E_PLOT_TYPE_1D:
   case E_PLOT_TYPE_REAL_FFT:
   case E_PLOT_TYPE_AVERAGE:
   case E_PLOT_TYPE_DB_POWER_FFT_REAL:
   default:
      twoDInput = false;
      break;
   }
   return twoDInput;
}

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
   double avgAmount;
}tParentCurveInfo;

typedef enum
{
   E_ADD,
   E_MULTIPLY,
   E_DIVIDE,
   E_SHIFT_UP,
   E_SHIFT_DOWN,
   E_POWER,
   E_LOG,
   E_MOD,
   E_ABS
}eMathOp;
inline bool valid_eMathOp(eMathOp in)
{
   switch(in)
   {
   case E_ADD:
   case E_MULTIPLY:
   case E_DIVIDE:
   case E_SHIFT_UP:
   case E_SHIFT_DOWN:
   case E_POWER:
   case E_LOG:
   case E_MOD:
   case E_ABS:
      return true;
      break;
   }
   return false;
}
inline bool needsValue_eMathOp(eMathOp in)
{
   switch(in)
   {
   case E_ADD:
   case E_MULTIPLY:
   case E_DIVIDE:
   case E_SHIFT_UP:
   case E_SHIFT_DOWN:
   case E_POWER:
   case E_LOG:
   case E_MOD:
      return true;
      break;
   case E_ABS:
      return false;
      break;
   }
   return false;
}

typedef struct
{
   eMathOp op;
   double num;

   // To be used for math ops that can generate a number when the math operation
   // is specified that will be used each time the math operation is applied to a sample.
   double helperNum;
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
   tPlotCurveName name;
   const char* msgPtr;
   unsigned int msgSize;
   QDateTime msgTime;
}tStoredMsg;


//////////////////////////////////////////////
////////////// Type Operators ////////////////
//////////////////////////////////////////////

// tPlotCurveName operators.
inline bool operator==(const tPlotCurveName& lhs, const tPlotCurveName& rhs)
{
   return (lhs.plot == rhs.plot) &&
          (lhs.curve == rhs.curve);
}
inline bool operator!=(const tPlotCurveName& lhs, const tPlotCurveName& rhs){return !(operator==(lhs, rhs));}

inline bool operator<(const tPlotCurveName& lhs, const tPlotCurveName& rhs)
{
   if(lhs.plot != rhs.plot)
   {
      int diffVal = memcmp( lhs.plot.toStdString().c_str(),
                            rhs.plot.toStdString().c_str(),
                            std::min(lhs.plot.size(), rhs.plot.size()) );
      if(diffVal != 0)
         return diffVal < 0;
      else
         return lhs.plot.size() < rhs.plot.size();
   }
   else
   {
      int diffVal =  memcmp( lhs.curve.toStdString().c_str(),
                             rhs.curve.toStdString().c_str(),
                             std::min(lhs.curve.size(), rhs.curve.size()) );
      if(diffVal != 0)
         return diffVal < 0;
      else
         return lhs.curve.size() < rhs.curve.size();

   }
}
inline bool operator<=(const tPlotCurveName& lhs, const tPlotCurveName& rhs){return   operator< (rhs, lhs);}
inline bool operator> (const tPlotCurveName& lhs, const tPlotCurveName& rhs){return !(operator<=(lhs, rhs));}
inline bool operator>=(const tPlotCurveName& lhs, const tPlotCurveName& rhs){return !(operator< (lhs, rhs));}









#endif

