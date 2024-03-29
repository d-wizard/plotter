/* Copyright 2016 - 2017, 2019 - 2021, 2024 Dan Williams. All Rights Reserved.
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
/*
 * persistentParameters.h
 *
 *  Created on: Sep 27, 2016
 *      Author: d
 */

#ifndef PERSISTENTPARAMETERS_H_
#define PERSISTENTPARAMETERS_H_

#include <string.h>
#include <string>


static const std::string PERSIST_PARAM_CURVE_SAVE_PREV_DIR_STR = "curveSavePrevDir";

static const std::string PERSIST_PARAM_CURVE_OPEN_PREV_TYPE_SELECTION = "curveOpenPrevTypeSelection";
static const std::string PERSIST_PARAM_PLOT_SAVE_PREV_SAVE_SELECTION_STR    = "plotSavePrevSaveSelection";
static const std::string PERSIST_PARAM_PLOT_SAVE_PREV_SAVE_SELECTION_INDEX  = "plotSavePrevSaveSelectionIndex";
static const std::string PERSIST_PARAM_CURVE_SAVE_PREV_SAVE_SELECTION_STR   = "curveSavePrevSaveSelection";
static const std::string PERSIST_PARAM_CURVE_SAVE_PREV_SAVE_SELECTION_INDEX = "curveSavePrevSaveSelectionIndex";

static const std::string PERSIST_PARAM_CHILD_CURVE_PLOT_TYPE = "childCurvePlotType";
static const std::string PERSIST_PARAM_CHILD_CURVE_MATH_TYPE = "childCurveMathType";
static const std::string PERSIST_PARAM_CHILD_CURVE_STAT_TYPE = "childCurveStatType";

static const std::string PERSIST_PARAM_FFT_MEAS_CHILD_SIZE = "fftMeasurementChildSize";
static const std::string PERSIST_PARAM_MATCH_PARENT_SCROLL = "matchParentScroll";

static const std::string PERSIST_PARAM_OPEN_RAW_TYPE = "openRawType";

void persistentParam_setPath(std::string path);

bool persistentParam_setParam_str(const std::string& paramName, std::string writeVal);
bool persistentParam_setParam_f64(const std::string& paramName, double writeVal);

bool persistentParam_getParam_str(const std::string& paramName, std::string& retVal);
bool persistentParam_getParam_f64(const std::string& paramName, double& retVal);


#endif /* PERSISTENTPARAMETERS_H_ */
