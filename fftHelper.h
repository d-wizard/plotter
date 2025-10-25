/* Copyright 2013 - 2015, 2017, 2025 Dan Williams. All Rights Reserved.
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
#ifndef fftHelper_h
#define fftHelper_h

#include <fftw3.h>

////////////////////////////////////////////////////////////////////////////////

class complexFFT
{
public:
   complexFFT(){}
   virtual ~complexFFT(){removePlan();}
   void run(const dubVect& inRe, const dubVect& inIm, dubVect& outRe, dubVect& outIm, double* windowCoef = NULL);
private:
   // copy, assignment constructors.
   complexFFT (const complexFFT&) = delete;
   complexFFT& operator= (const complexFFT&) = delete;

private:
   void removePlan();

   fftw_complex* in = nullptr;
   fftw_complex* out = nullptr;
   fftw_plan p;
   unsigned int N = 0;
};

////////////////////////////////////////////////////////////////////////////////

class realFFT
{
public:
   realFFT(){}
   virtual ~realFFT(){removePlan();}
   void run(const dubVect& inRe, dubVect& outRe, double* windowCoef = NULL);
private:
   // copy, assignment constructors.
   realFFT (const realFFT&) = delete;
   realFFT& operator= (const realFFT&) = delete;

private:
   void removePlan();

   fftw_complex* in = nullptr;
   fftw_complex* out = nullptr;
   fftw_plan p;
   unsigned int N = 0;
};

////////////////////////////////////////////////////////////////////////////////

void getFFTXAxisValues_real(dubVect& xAxis, unsigned int numPoints, double& min, double& max, double sampleRate = 0.0);
void getFFTXAxisValues_complex(dubVect& xAxis, unsigned int numPoints, double& min, double& max, double sampleRate = 0.0);

void genWindowCoef(double* outSamp, unsigned int numSamp, bool scale);
#endif
