/* Copyright 2013, 2015 Dan Williams. All Rights Reserved.
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
#ifndef dllHelper_h
#define dllHelper_h

#if (defined(_WIN32) || defined(__WIN32__))
   #define DLL_HELPER_WIN_BUILD
   #include <windows.h>
#else
   #define DLL_HELPER_LINUX_BUILD
#endif

#ifdef DLL_HELPER_WIN_BUILD
extern "C" Q_DECL_EXPORT BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpvReserved*/ );
#endif

extern "C" Q_DECL_EXPORT void startPlotGui(void);
extern "C" Q_DECL_EXPORT void stopPlotGui(void);

extern "C" Q_DECL_EXPORT void createPlot(char* msg);

extern "C" Q_DECL_EXPORT
void create1dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   int yAxisType,
                   void *yAxisSamples);

extern "C" Q_DECL_EXPORT
void create2dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   int xAxisType,
                   int yAxisType,
                   void *xAxisSamples,
                   void *yAxisSamples);

extern "C" Q_DECL_EXPORT
void update1dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   unsigned int sampleStartIndex,
                   int yAxisType,
                   void *yAxisSamples);

extern "C" Q_DECL_EXPORT
void update2dPlot( char* plotName,
                   char* curveName,
                   unsigned int numSamp,
                   unsigned int sampleStartIndex,
                   int xAxisType,
                   int yAxisType,
                   void *xAxisSamples,
                   void *yAxisSamples);

#endif


