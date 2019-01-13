/* Copyright 2017 - 2019 Dan Williams. All Rights Reserved.
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
 * persistentPlotParameters.h
 *
 *  Created on: May 21, 2017
 *      Author: d
 *
 *  This file contains functionality for restoring certain plot parameters based on plot name.
 *  The idea is that a plot window might be closed, and then later a new plot window gets created
 *  with the same plot name. In that case, we will restore certain plot parameters from the original
 *  plot window.
 *  Note: The persistence of this information only lasts while the plotter executable is running.
 *  All plot parameter information is lost when the plotter executable is closed.
 */

#ifndef PERSISTENTPLOTPARAMETERS_H_
#define PERSISTENTPLOTPARAMETERS_H_

#include <QMap>
#include <QString>
#include <qwt_plot_curve.h>
#include "PlotHelperTypes.h"

// Template class for storing an individual plot parameter.
template <class T>
class persistentPlotParam{
public:
    persistentPlotParam():m_valid(false){}
    bool isValid(){return m_valid;}
    void set(T& value){m_value = value; m_valid = true;}
    T get(){return m_value;}
private:
    bool m_valid;
    T m_value;
};


// Class that stores all persistent plot parameters for a specific plot name.
class persistentPlotParameters{
public:
   persistentPlotParameters(){}

   persistentPlotParam<int> m_displayPrecision;
   persistentPlotParam<eDisplayPointType> m_displayType;
   persistentPlotParam<QwtPlotCurve::CurveStyle> m_curveStyle;
   persistentPlotParam<bool> m_legend;
   persistentPlotParam<bool> m_cursorCanSelectAnyCurve;

};

// Map typedef. The key to the map is the plot name.
typedef QMap<QString, persistentPlotParameters> tPersistentPlotParamMap;

// Returns whether the map contains persistent plot parameters for the given plot name.
static inline bool persistentPlotParam_areThereAnyParams(tPersistentPlotParamMap& pppMap, const QString& plotName)
{
   return pppMap.find(plotName) != pppMap.end();
}

// Returns a pointer to a map instance for the given plot name. If a map instance does not already exist, one will be created.
static inline persistentPlotParameters* persistentPlotParam_get(tPersistentPlotParamMap& pppMap, const QString& plotName)
{
   if(persistentPlotParam_areThereAnyParams(pppMap, plotName) == false)
   {
      persistentPlotParameters temp;
      pppMap[plotName] = temp;
   }
   return &pppMap[plotName];
}




#endif /* PERSISTENTPLOTPARAMETERS_H_ */
