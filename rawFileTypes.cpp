/* Copyright 2024, 2026 Dan Williams. All Rights Reserved.
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
#include "rawFileTypes.h"

const QString RAW_TYPE_DROPDOWN[E_RAW_TYPE_SIZE] =
{
   "Signed Int 8 Bit",
   "Signed Int 16 Bit",
   "Signed Int 32 Bit",
   "Signed Int 64 Bit",
   "Unsigned Int 8 Bit",
   "Unsigned Int 16 Bit",
   "Unsigned Int 32 Bit",
   "Unsigned Int 64 Bit",
   "Float 16 Bit",
   "Float 32 Bit",
   "Float 64 Bit",
   "Interleaved Signed Int 8 Bit",
   "Interleaved Signed Int 16 Bit",
   "Interleaved Signed Int 32 Bit",
   "Interleaved Signed Int 64 Bit",
   "Interleaved Unsigned Int 8 Bit",
   "Interleaved Unsigned Int 16 Bit",
   "Interleaved Unsigned Int 32 Bit",
   "Interleaved Unsigned Int 64 Bit",
   "Interleaved Float 16 Bit",
   "Interleaved Float 32 Bit",
   "Interleaved Float 64 Bit"
};

const size_t RAW_TYPE_BLOCK_SIZE[E_RAW_TYPE_SIZE] =
{
    1, //E_RAW_TYPE_SIGNED_INT_8,
    2, //E_RAW_TYPE_SIGNED_INT_16,
    4, //E_RAW_TYPE_SIGNED_INT_32,
    8, //E_RAW_TYPE_SIGNED_INT_64,
    1, //E_RAW_TYPE_UNSIGNED_INT_8,
    2, //E_RAW_TYPE_UNSIGNED_INT_16,
    4, //E_RAW_TYPE_UNSIGNED_INT_32,
    8, //E_RAW_TYPE_UNSIGNED_INT_64,
    2, //E_RAW_TYPE_FLOAT_16,
    4, //E_RAW_TYPE_FLOAT_32,
    8, //E_RAW_TYPE_FLOAT_64,
    2, //E_RAW_TYPE_INTERLEAVED_SIGNED_INT_8,
    4, //E_RAW_TYPE_INTERLEAVED_SIGNED_INT_16,
    8, //E_RAW_TYPE_INTERLEAVED_SIGNED_INT_32,
   16, //E_RAW_TYPE_INTERLEAVED_SIGNED_INT_64,
    2, //E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_8,
    4, //E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_16,
    8, //E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_32,
   16, //E_RAW_TYPE_INTERLEAVED_UNSIGNED_INT_64,
    4, //E_RAW_TYPE_INTERLEAVED_FLOAT_16,
    8, //E_RAW_TYPE_INTERLEAVED_FLOAT_32,
   16  //E_RAW_TYPE_INTERLEAVED_FLOAT_64
};