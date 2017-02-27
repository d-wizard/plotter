/* Copyright 2013 - 2017 Dan Williams. All Rights Reserved.
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
#include <QStringList>
#include <QList>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <limits>       // std::numeric_limits


#include "plotMsgPack.h"

const QString PLOT_CURVE_SEP = "->";
const QString GUI_ALL_VALUES = "*";
const QString X_AXIS_APPEND = ".xAxis";
const QString Y_AXIS_APPEND = ".yAxis";


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define STRING_STREAM_CLEAR_PRECISION_VAL (-1)

typedef std::vector<double> dubVect;

typedef signed long long PlotMsgIdType; // Each plot message needs a unique number. 64 bits should ensure no number is ever used twice.
#define PLOT_MSG_ID_TYPE_NO_PARENT_MSG (-1) // This is used to indicate a change was made to curve data that did not come from a plot message.

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
    bool realX; // If true at least one of the X samples is real. If false all the X samples are not real (i.e. NaN, -inf, +inf, etc)
    bool realY; // If true at least one of the Y samples is real. If false all the Y samples are not real (i.e. NaN, -inf, +inf, etc)

    bool operator==(const struct tMaxMinXY& rhs)
    {
        return (minX == rhs.minX) &&
               (minY == rhs.minY) &&
               (maxX == rhs.maxX) &&
               (maxY == rhs.maxY);
    }

    bool operator!=(const struct tMaxMinXY& rhs)
    {
        return (minX != rhs.minX) ||
               (minY != rhs.minY) ||
               (maxX != rhs.maxX) ||
               (maxY != rhs.maxY);
    }

}maxMinXY;

typedef enum
{
    E_PLOT_DIM_1D,
    E_PLOT_DIM_2D,
    E_PLOT_DIM_INVALID
}ePlotDim;
inline bool valid_ePlotDim(ePlotDim in)
{
   switch(in)
   {
      case E_PLOT_DIM_1D:
      case E_PLOT_DIM_2D:
         return true;
      break;
      default:
         return false;
      break;
   }
   return false;
}
inline ePlotDim plotActionToPlotDim(ePlotAction action)
{
   switch(action)
   {
      case E_CREATE_1D_PLOT:
      case E_UPDATE_1D_PLOT:
         return E_PLOT_DIM_1D;
      break;
      case E_CREATE_2D_PLOT:
      case E_UPDATE_2D_PLOT:
         return E_PLOT_DIM_2D;
      break;
      default:
         return E_PLOT_DIM_INVALID;
      break;
   }
   return E_PLOT_DIM_INVALID;
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
   E_PLOT_TYPE_DB_POWER_FFT_COMPLEX,
   E_PLOT_TYPE_DELTA,
   E_PLOT_TYPE_SUM,
   E_PLOT_TYPE_MATH_BETWEEN_CURVES,

   // The following values are not represented in the GUI. Since these value
   // don't need to match a GUI value, enumerate from the end.
   E_PLOT_TYPE_RESTORE_PLOT_FROM_FILE = -1
}ePlotType;


typedef enum // Must match cmbChildMathOperators
{
   E_MATH_BETWEEN_CURVES_ADD,
   E_MATH_BETWEEN_CURVES_SUBTRACT,
   E_MATH_BETWEEN_CURVES_MULTILPY,
   E_MATH_BETWEEN_CURVES_DIVIDE
}eMathBetweenCurves_operators;

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
   case E_PLOT_TYPE_DELTA:
   case E_PLOT_TYPE_SUM:
   case E_PLOT_TYPE_MATH_BETWEEN_CURVES:
   case E_PLOT_TYPE_RESTORE_PLOT_FROM_FILE:
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
   case E_PLOT_TYPE_MATH_BETWEEN_CURVES:
      twoDInput = true;
      break;

   case E_PLOT_TYPE_1D:
   case E_PLOT_TYPE_REAL_FFT:
   case E_PLOT_TYPE_AVERAGE:
   case E_PLOT_TYPE_DB_POWER_FFT_REAL:
   case E_PLOT_TYPE_DELTA:
   case E_PLOT_TYPE_SUM:
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
   bool windowFFT;
   bool scaleFftWindow;
   eMathBetweenCurves_operators mathBetweenCurvesOperator;
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

typedef struct mathOperator
{
   eMathOp op;
   double num;

   // To be used for math ops that can generate a number when the math operation
   // is specified that will be used each time the math operation is applied to a sample.
   double helperNum;

   bool operator==(const struct mathOperator& rhs)
   {
       return (op == rhs.op) &&
              (num == rhs.num) &&
              (helperNum == rhs.helperNum);
   }

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

class tPlotterIpAddr
{
public:
   // Types
   typedef unsigned long tIpV4;

   static QString convert(tIpV4 ipV4Addr)
   {
      unsigned char* newIpV4Val_bytes = (unsigned char*)&ipV4Addr;
      QString newIpV4Val = "";
      for(size_t i = 0; i < sizeof(tIpV4); ++i)
      {
         newIpV4Val.append(QString::number((unsigned int)newIpV4Val_bytes[i]));
         if(i < (sizeof(tIpV4)-1))
         {
            newIpV4Val.append(".");
         }
      }
      return newIpV4Val;
   }
   static tIpV4 convert(QString ipV4Addr)
   {
      tIpV4 newIpV4Val = 0;
      QStringList stringOctets = ipV4Addr.split(".");
      if(stringOctets.size() == sizeof(tIpV4))
      {
         char* newIpV4Val_bytes = (char*)&newIpV4Val;
         for(int i = 0; i < stringOctets.size(); ++i)
         {
            int octet = stringOctets[i].toInt();
            char* bytes = (char*)&octet;
            newIpV4Val_bytes[i] = bytes[0];
         }
      }
      return newIpV4Val;
   }

   tPlotterIpAddr(){}
   tPlotterIpAddr(tIpV4 ipV4Addr): m_ipV4Addr(ipV4Addr) {}
   tPlotterIpAddr(QString ipV4Addr){m_ipV4Addr = convert(ipV4Addr);}

   bool operator== (const tPlotterIpAddr &c1)
   {
       return c1.m_ipV4Addr == this->m_ipV4Addr;
   }
   bool operator!= (const tPlotterIpAddr &c1)
   {
       return c1.m_ipV4Addr != this->m_ipV4Addr;
   }
   bool operator> (const tPlotterIpAddr &c1) const
   {
       return c1.m_ipV4Addr > this->m_ipV4Addr;
   }
   bool operator>= (const tPlotterIpAddr &c1) const
   {
       return c1.m_ipV4Addr >= this->m_ipV4Addr;
   }
   bool operator< (const tPlotterIpAddr &c1) const
   {
       return c1.m_ipV4Addr < this->m_ipV4Addr;
   }
   bool operator<= (const tPlotterIpAddr &c1) const
   {
       return c1.m_ipV4Addr <= this->m_ipV4Addr;
   }

   void setIpV4Addr(tIpV4& ipV4Addr){m_ipV4Addr = ipV4Addr;}
   void setIpV4Addr(QString& ipV4Addr){m_ipV4Addr = convert(ipV4Addr);}

   unsigned long m_ipV4Addr;
};

typedef struct
{
   const char* msgPtr;
   unsigned int msgSize;
   tPlotterIpAddr ipAddr;
}tIncomingMsg;


typedef struct
{
   double sampleRate;
   tMathOpList mathOpsXAxis;
   tMathOpList mathOpsYAxis;
}tCurveMathProperties;



//////////////////////////////////////////////
/////////////// Type Helpers /////////////////
//////////////////////////////////////////////
inline bool isDoubleValid(double value)
{
   // According to the IEEE standard, NaN values have the odd property that
   // comparisons involving them are always false. That is, for a float
   // f, f != f will be true only if f is NaN
   if (value != value)
   {
      return false;
   }
   else if (value > std::numeric_limits<double>::max())
   {
      return false;
   }
   else if (value < -std::numeric_limits<double>::max())
   {
      return false;
   }
   else
   {
      return true;
   }
}


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

