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
#include <fftw3.h>
#include <math.h>
#include "PlotHelperTypes.h"
#include "fftHelper.h"

void complexFFT(const dubVect& inRe, const dubVect& inIm, dubVect& outRe, dubVect& outIm, double *windowCoef)
{
   fftw_complex *in, *out;
   fftw_plan p;

   unsigned int N = std::min(inRe.size(), inIm.size());

   if(N > 0)
   {
       in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
       out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

       if(windowCoef == NULL)
       {
          for(unsigned int i = 0; i < N; ++i)
          {
             in[i][0] = inRe[i];
             in[i][1] = inIm[i];
          }
       }
       else
       {
          for(unsigned int i = 0; i < N; ++i)
          {
             in[i][0] = inRe[i] * windowCoef[i];
             in[i][1] = inIm[i] * windowCoef[i];
          }
       }

       p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

       fftw_execute(p);

       fftw_destroy_plan(p);

       outRe.resize(N);
       outIm.resize(N);
       for(unsigned int i = 0; i < N; ++i)
       {
          outRe[i] = out[i][0] / (double)N;
          outIm[i] = out[i][1] / (double)N;
       }

       fftw_free(in);
       fftw_free(out);
    }
   else
   {
       outRe.clear();
       outIm.clear();
   }
}

void realFFT(const dubVect& inRe, dubVect& outRe, double* windowCoef)
{
   fftw_complex *in, *out;
   fftw_plan p;

   unsigned int N = inRe.size();

   if(N > 0)
   {
       unsigned int halfN = N >> 1;

       in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
       out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

       if(windowCoef == NULL)
       {
          for(unsigned int i = 0; i < N; ++i)
          {
             in[i][0] = inRe[i];
             in[i][1] = inRe[i];
          }
       }
       else
       {
          for(unsigned int i = 0; i < N; ++i)
          {
             in[i][0] = inRe[i] * windowCoef[i];
             in[i][1] = inRe[i] * windowCoef[i];
          }
       }

       p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

       fftw_execute(p);

       fftw_destroy_plan(p);

       outRe.resize(halfN);
       outRe[0] = fabs(out[0][0] / (double)N);
       for(unsigned int i = 1; i < halfN; ++i)
       {
          outRe[i] = (fabs(out[i][0]) + fabs(out[N-i][0])) / (double)N;
       }

       fftw_free(in);
       fftw_free(out);
   }
   else
   {
       outRe.clear();
   }
}

void getFFTXAxisValues_real(dubVect& xAxis, unsigned int numPoints, double sampleRate)
{
   if(numPoints > 0)
   {
      if(xAxis.size() != numPoints)
      {
         xAxis.resize(numPoints);
      }
      if(sampleRate == 0.0)
      {
         for(unsigned int i = 0; i < numPoints; ++i)
         {
            xAxis[i] = (double)i;
         }
      }
      else
      {
         // Real FFTs go from 0 to Fs/2.
         double hzPerBin = sampleRate / ((double)numPoints * (double)2.0);
         for(unsigned int i = 0; i < numPoints; ++i)
         {
            xAxis[i] = (double)i * hzPerBin;
         }

      }
   }
   else
   {
       xAxis.clear();
   }

}

void getFFTXAxisValues_complex(dubVect& xAxis, unsigned int numPoints, double sampleRate)
{
    if(numPoints > 0)
    {
       // Complex FFTs go from -Fs/2 to Fs/2.
        int halfPoints = (int)(numPoints >> 1);
        int start = -halfPoints;
        unsigned int end = halfPoints;
        if((numPoints & 1) == 0)
        {
           --end;
        }

        if(xAxis.size() != numPoints)
        {
           xAxis.resize(numPoints);
        }

        if(sampleRate == 0.0)
        {
           for(unsigned int i = 0; i <= end; ++i)
           {
              xAxis[i] = i;
           }
           for(unsigned int i = (end+1); i < numPoints; ++i)
           {
              xAxis[i] = start++;
           }
        }
        else
        {
           double hzPerBin = sampleRate / (double)numPoints;
           for(unsigned int i = 0; i <= end; ++i)
           {
              xAxis[i] = (double)i * hzPerBin;
           }
           for(unsigned int i = (end+1); i < numPoints; ++i)
           {
              xAxis[i] = ((double)start++) * hzPerBin;
           }
        }
    }
    else
    {
        xAxis.clear();
    }
}

void genBlackmanWindowCoef(double* outSamp, unsigned int numSamp)
{
   for(unsigned int i = 0; i < numSamp; ++i)
   {
      outSamp[i] = ( 0.42 - 0.5 * cos (2.0*M_PI*(double)i/(double)(numSamp-1))
         + 0.08 * cos (4.0*M_PI*(double)i/(double)(numSamp-1)) );
   }
}
