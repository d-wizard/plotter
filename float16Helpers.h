/* Copyright 2026 Dan Williams. All Rights Reserved.
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
#pragma once
#include <stdint.h>

// Define a type that can convert to / from 16-bit float completely in software.
class PlotFloat16
{
public:
   PlotFloat16(){}

   PlotFloat16(const float& val):data(float32ToFloat16AsUint16(val)){}
   PlotFloat16(const double& val):data(float32ToFloat16AsUint16((float)val)){}

   operator float() const { return float16AsUint16ToFloat32[data]; }
   operator double() const { return static_cast<double>(float16AsUint16ToFloat32[data]); }
   
   PlotFloat16& operator=(const float& val)
   {
      data = float32ToFloat16AsUint16(val);
      return *this;
   }
   PlotFloat16& operator=(const double& val)
   {
      data = float32ToFloat16AsUint16((float)val);
      return *this;
   }
private:
   uint16_t data;

   // Lookup table for converting from 16-bit float to 32-bit float.
   static float* float16AsUint16ToFloat32; // Takes uint16_t values in (representing the 16-bit float) and returns the equivalent 32-bit float value.

   static uint16_t float32ToFloat16AsUint16(float f); // Function that converts a 32-bit float to 16-bit float, but returns the value as a uint16_t (this is need for platforms that might not support 16-bit float).

};
