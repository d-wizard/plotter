/* Copyright 2014 Dan Williams. All Rights Reserved.
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
#ifndef AmFmPmDemod_h
#define AmFmPmDemod_h

#include "DataTypes.h"
#include "PlotHelperTypes.h"

#include <math.h>

#define M_NEG_PI -3.14159265358979323846
#define M_2X_PI 6.28318530717958647692528676655900

inline void AmDemod(dubVect& reInput, dubVect& imInput, dubVect& demodOutput)
{
   unsigned int outSize = std::min(reInput.size(), imInput.size());
   demodOutput.resize(outSize);
   for(unsigned int i = 0; i < outSize; ++i)
   {
      demodOutput[i] = sqrt((reInput[i]*reInput[i]) + (imInput[i]*imInput[i]));
   }
}

inline void FmDemod(dubVect& reInput, dubVect& imInput, dubVect& demodOutput)
{
   unsigned int outSize = std::min(reInput.size(), imInput.size());
   if(outSize > 0)
   {
      demodOutput.resize(outSize);

      double prevPhase = atan2(reInput[outSize-1], imInput[outSize-1]);
      double curPhase = 0.0;

      for(unsigned int i = 0; i < outSize; ++i)
      {
         curPhase = atan2(reInput[i], imInput[i]);
         demodOutput[i] = curPhase - prevPhase;
         if(demodOutput[i] < M_NEG_PI)
            demodOutput[i] += M_2X_PI;
         else if(demodOutput[i] > M_PI)
            demodOutput[i] -= M_2X_PI;

         prevPhase = curPhase;
      }
   }
}

inline void PmDemod(dubVect& reInput, dubVect& imInput, dubVect& demodOutput)
{
   unsigned int outSize = std::min(reInput.size(), imInput.size());
   demodOutput.resize(outSize);
   for(unsigned int i = 0; i < outSize; ++i)
   {
      demodOutput[i] = atan2(reInput[i], imInput[i]);
   }
}



#endif


