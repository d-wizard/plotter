/* Copyright 2013, 2026 Dan Williams. All Rights Reserved.
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
#ifndef DataTypes_h
#define DataTypes_h

#include <vector>
#include <list>
#include "float16Helpers.h"

typedef signed char SCHAR;
typedef unsigned char UCHAR;
typedef signed short INT_16;
typedef unsigned short UINT_16;
typedef signed int INT_32;
typedef unsigned int UINT_32;
typedef signed long long INT_64;
typedef unsigned long long UINT_64;
typedef PlotFloat16 FLOAT_16;
typedef float FLOAT_32;
typedef double FLOAT_64;
typedef long double FLOAT_96;

typedef unsigned char BIT;
#define TO_BIT(x) (x&1)

typedef signed long long INT_SMAX;
typedef unsigned long long INT_UMAX;

typedef std::list<INT_UMAX> ioData;
typedef std::list<BIT> bitData;

#define BITS_PER_BYTE (8)
#define BITS_PER_BYTE_MASK (7)

#define MAX_INT_NEGATIVE_MASK ((INT_UMAX)( (INT_UMAX) ((INT_UMAX)1 << ((sizeof(INT_UMAX)*BITS_PER_BYTE) - 1)) ))

#endif
