/* Copyright 2016 - 2017, 2019 - 2020 Dan Williams. All Rights Reserved.
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
#ifndef PLOTSNRCALC_H
#define PLOTSNRCALC_H

#include <QLabel>
#include "plotBar.h"
#include "CurveData.h"


class plotSnrCalc
{
public:

   plotSnrCalc(QwtPlot* parentPlot, QLabel* snrLabel);
   ~plotSnrCalc();


   void show(const tMaxMinXY& plotDimensions, const tMaxMinXY& zoomDimensions);
   void hide();
   bool isVisable();

   void updateZoomDim(const maxMinXY& zoomDim);
   void updatePlotDim(const maxMinXY& plotDim);

   bool isSelectionCloseToBar( const QPointF& pos,
                               const maxMinXY& zoomDim,
                               const int canvasWidth_pixels,
                               const int canvasHeight_pixels);

   void moveBar(const QPointF& pos);

   void moveToFront();

   void setCurve(CurveData* curve, QList<CurveData*>& allCurves);

   bool curveUpdated(CurveData* curve, QList<CurveData*>& allCurves);

   void updateSampleRate();

   double getMeasurement(eFftSigNoiseMeasurements type);

   QPointF selectedBarPos();
   void selectedBarPos(QPointF val);

private:

   typedef struct curveDataIndexes
   {
      int startIndex; // Inclusive
      int stopIndex;  // Inclusive

      bool operator==(const struct curveDataIndexes& rhs)
      {
          return (startIndex == rhs.startIndex) &&
                 (stopIndex == rhs.stopIndex);
      }

      bool operator!=(const struct curveDataIndexes& rhs)
      {
          return (startIndex != rhs.startIndex) ||
                 (stopIndex != rhs.stopIndex);
      }

      // Since both start and stop indexes are inclusive, set the default stop index
      // to a lower value than the default start index. This way the default values
      // don't look like there are any valid samples.
      curveDataIndexes(): startIndex(0), stopIndex(-1){}
   }tCurveDataIndexes;

   typedef struct fftBinChunk
   {
      tCurveDataIndexes indexes;
      double powerLinear;
      double bandwidth;
      fftBinChunk(): powerLinear(0.0), bandwidth(0.0){}
   }tFftBinChunk;


   // Eliminate default, copy, assign
   plotSnrCalc();
   plotSnrCalc(plotSnrCalc const&);
   void operator=(plotSnrCalc const&);

   CurveData* getComplexParentCurve(QList<CurveData*>& allCurves);

   void autoSetBars();

   void calcSnr();
   void calcSnrSlow();
   void calcSnrFast();
   void calcPower(
         tFftBinChunk* fftChunk,
         ePlotType plotType,
         const double *xPoints,
         const double *yPoints,
         const double *yPoints_complex,
         int numPoints,
         double hzPerBin );

   bool findDcBinIndex(unsigned int numPoints, const double* xPoints);
   void findIndexes(double start, double stop, tCurveDataIndexes* indexes, int numPoints, const double* xPoints, double hzPerBin);
   int findIndex(double barPoint, int numPoints, const double* xPoints, double hzPerBin, bool highSide);

   void determineNoiseChunks(const tCurveDataIndexes& noise, const tCurveDataIndexes& signal, tCurveDataIndexes& noiseLeftRet, tCurveDataIndexes& noiseRightRet);

   bool calcBinDelta(const tCurveDataIndexes& oldIndexes, const tCurveDataIndexes& newIndexes, tFftBinChunk* retFftBinChunk);

   void calcFftChunk(tFftBinChunk* fftChunk, const tCurveDataIndexes& newIndexes);
   void updateFftChunk(tFftBinChunk* fftChunk, const tCurveDataIndexes& newIndexes);

   QString doubleToStr(double num);
   QString numToStrDb(double num, QString units);
   QString numToHz(double num);
   void setLabel();

   bool m_barsAreItialized;
   QwtPlot* m_parentPlot;
   QLabel* m_snrLabel;
   CurveData* m_parentCurve;
   CurveData* m_parentCurve_complex;
   bool m_multipleParentCurves;
   double m_curveSampleRate;
   bool m_isVisable;

   plotBar* m_noiseBars[2];
   plotBar* m_signalBars[2];

   plotBar* m_allBars[4];

   int m_activeBarIndex;

   int m_dcBinIndex;

   tFftBinChunk m_noiseChunkLeft;
   tFftBinChunk m_noiseChunkRight;
   tFftBinChunk m_signalChunk;

   double m_signalPower;
   double m_signalBandwidth;
   double m_noisePower;
   double m_noiseBandwidth;
   double m_noiseBwPerHz;
   double m_snrDb;
   double m_snrDbHz;

};





#endif // PLOTSNRCALC_H
