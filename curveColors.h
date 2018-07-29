/* Copyright 2013, 2018 Dan Williams. All Rights Reserved.
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
                      
QColor curveColors[] =
{                     
    QColor(0x00FFFF),  // 0  
    QColor(0xFF00FF),  // 1  
    QColor(0xFFFF00),  // 2  
    QColor(0xFF8000),  // 3  
    QColor(0x00FF00),  // 4  
    QColor(0x8080FF),  // 5  
    QColor(0xC0C0C0),  // 6  
    QColor(0xFFC0FF),  // 7  
    QColor(0xFFC040),  // 8  
    QColor(0xC0FF00),  // 9  
    QColor(0x00C080),  // 10 
    QColor(0xC080FF),  // 11 
    QColor(0x40C0FF),  // 12 
    QColor(0xFF0000),  // 13 
    QColor(0x8040C0),  // 14 
    QColor(0xC08040),  // 15 
    QColor(0x606060),  // 16 
    QColor(0x0000FF),  // 17 
    QColor(0xC0C0FF),  // 18 
    QColor(0x80C0C0),  // 19 
    QColor(0x804000),  // 20 
    QColor(0x0080FF),  // 21 
    QColor(0xE0E0E0),  // 22 
    QColor(0xC0FF40),  // 23 
    QColor(0x80FFC0),  // 24 
    QColor(0xC08080),  // 25 
    QColor(0x8000FF),  // 26 
    QColor(0x004080),  // 27 
    QColor(0x40C040),  // 28 
    QColor(0x80FFFF),  // 29 
    QColor(0x4040FF),  // 30 
    QColor(0x4080C0),  // 31 
    QColor(0x8000C0),  // 32 
    QColor(0xC0FFC0),  // 33 
    QColor(0xC0FFFF),  // 34 
    QColor(0xFF8040),  // 35 
    QColor(0x80FF00),  // 36 
    QColor(0xFFC080),  // 37 
    QColor(0x808080),  // 38 
    QColor(0xFF4040),  // 39 
    QColor(0x408080),  // 40 
    QColor(0x00C000),  // 41 
    QColor(0xC04000),  // 42 
    QColor(0x408040),  // 43 
    QColor(0xC04040),  // 44 
    QColor(0x80C0FF),  // 45 
    QColor(0xC080C0),  // 46 
    QColor(0xC0C000),  // 47 
    QColor(0x404080),  // 48 
    QColor(0xFF8080),  // 49 
    QColor(0x0080C0),  // 50 
    QColor(0x80FF40),  // 51 
    QColor(0x4000FF),  // 52 
    QColor(0xFFC0C0),  // 53 
    QColor(0x808000),  // 54 
    QColor(0x8040FF),  // 55 
    QColor(0xC040C0),  // 56 
    QColor(0xFFFFFF),  // 57 
    QColor(0x40FFC0),  // 58 
    QColor(0x40FF00),  // 59 
    QColor(0x404040),  // 60 
    QColor(0x800000),  // 61 
    QColor(0x80C080),  // 62 
    QColor(0x40C080),  // 63 
    QColor(0x00FF40),  // 64 
    QColor(0x40C000),  // 65 
    QColor(0xC08000),  // 66 
    QColor(0xFFFF40),  // 67 
    QColor(0x0040FF),  // 68 
    QColor(0x0000C0),  // 69 
    QColor(0xC000FF),  // 70 
    QColor(0xFFFFC0),  // 71 
    QColor(0x40FFFF),  // 72 
    QColor(0xFF40FF),  // 73 
    QColor(0xC0C040),  // 74 
    QColor(0x804040),  // 75 
    QColor(0x804080),  // 76 
    QColor(0x4040C0),  // 77 
    QColor(0x80C040),  // 78 
    QColor(0xC0C080),  // 79 
    QColor(0xFFFF80),  // 80 
    QColor(0x00C040),  // 81 
    QColor(0x0040C0),  // 82 
    QColor(0xC00000),  // 83 
    QColor(0x40FF40),  // 84 
    QColor(0x808040),  // 85 
    QColor(0xC000C0),  // 86 
    QColor(0x008080),  // 87 
    QColor(0x00FFC0),  // 88 
    QColor(0xFFC000),  // 89 
    QColor(0x4080FF),  // 90 
    QColor(0xFF80FF),  // 91 
    QColor(0x008040),  // 92 
    QColor(0x00C0FF),  // 93 
    QColor(0x00C0C0),  // 94 
    QColor(0xC040FF),  // 95 
    QColor(0x4000C0),  // 96 
    QColor(0xFF4000),  // 97 
    QColor(0x8080C0),  // 98 
    QColor(0x800080),  // 99 
    QColor(0x40C0C0),  // 100
    QColor(0x408000),  // 101
    QColor(0x80C000),  // 102
    QColor(0x008000),  // 103
    QColor(0x400080),  // 104
    QColor(0xA0A0A0),  // 105
    QColor(0xC0FF80),  // 106
    QColor(0x40FF80),  // 107
    QColor(0x80FF80)   // 108
                      
};                    
                      
