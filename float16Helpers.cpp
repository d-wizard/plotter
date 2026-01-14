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
#include <string>
#include "float16Helpers.h"
#include "F16ToF32.h" // Lookup table for converting from 16-bit float to 32-bit float.

// __F16ToF32 stores the float values as uint32_t. Cast pointer to float.
float* F16ToF32 = (float*)&__F16ToF32[0];

////////////////////////////////////////////////////////////////////////////////

uint16_t float32ToFloat16_asUint16(float f)
{
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));

    uint32_t sign = bits >> 31;
    int32_t exp   = ((bits >> 23) & 0xFF) - 127;
    uint32_t mant = bits & 0x7FFFFF;

    uint16_t hbits = sign << 15;

    if (exp == 128)
    {
        if (mant == 0)
            hbits |= 0x7C00; // infinity
        else
            hbits |= 0x7E00; // quiet NaN
    }
    else if (exp > 15)
    {
        hbits |= 0x7C00; // overflow -> infinity
    }
    else if (exp >= -14)
    {
        // Normalized half
        exp += 15;
        mant += 0x1000; // rounding

        if (mant & 0x800000)
        {
            mant = 0;
            exp++;
        }

        if (exp >= 31)
            hbits |= 0x7C00;
        else
            hbits |= (exp << 10) | (mant >> 13);
    }
    else if (exp >= -24)
    {
        // Subnormal half
        mant |= 0x800000; // implicit leading 1

        int shift = -exp - 14;
        uint32_t rounding = 1u << (shift + 12);

        mant += rounding;
        uint32_t sub = mant >> (shift + 13);

        hbits |= static_cast<uint16_t>(sub);
    }
    // else: fully underflow -> signed zero

    uint16_t result;
    memcpy(&result, &hbits, sizeof(hbits));
    return result;
}