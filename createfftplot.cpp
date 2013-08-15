/* Copyright 2013 Dan Williams. All Rights Reserved.
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
#include "createfftplot.h"
#include "ui_createfftplot.h"

#include "fftPlots.h"
#include "mainwindow.h"


void complexFFT(const dubVect& inRe, const dubVect& inIm, dubVect& outRe, dubVect& outIm);

const QString NEW_PLOT_TEXT = "<New>";
const QString X_AXIS_APPEND = ".xAxis";
const QString Y_AXIS_APPEND = ".yAxis";

createFFTPlot::createFFTPlot(fftPlots *fftPlotParent, tCurveCommanderInfo& initPlotsCurves, eFFTType fftType, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::createFFTPlot),
    m_parent(fftPlotParent),
    m_plotsAndCurves(initPlotsCurves),
    m_guiChanging(false),
    m_validUserInput(false)
{
    ui->setupUi(this);

    this->setFixedSize(351, 323);
    if(fftType == E_FFT_REAL)
    {
        ui->groupImaginary->setVisible(false);
    }

    this->setVisible(true);

    m_fftCurveData.plotName      = "";
    m_fftCurveData.curveName     = "";
    m_fftCurveData.fftType       = fftType;
    m_fftCurveData.srcRePlotName  = "";
    m_fftCurveData.srcReCurveName = "";
    m_fftCurveData.srcReAxis      = E_Y_AXIS;
    m_fftCurveData.srcImPlotName  = "";
    m_fftCurveData.srcImCurveName = "";
    m_fftCurveData.srcImAxis      = E_Y_AXIS;
    
    updateGui();

}

createFFTPlot::~createFFTPlot()
{
    delete ui;
}

void createFFTPlot::plotsCurvesChanged(const tCurveCommanderInfo &pc)
{
    m_plotsAndCurves = pc;
    updateGui();
}

void createFFTPlot::updateGui()
{
    m_guiChanging = true;

    unsigned int itemIndex = 0;

    QString oldPlotReName = ui->cmbRePlot->currentText();
    QString oldPlotImName = ui->cmbImPlot->currentText();
    QString oldPlotOutName = ui->cmbOutPlot->currentText();

    QString oldCurveReName = ui->cmbReCurve->currentText();
    QString oldCurveImName = ui->cmbImCurve->currentText();

    int oldPlotReNameCount = ui->cmbRePlot->count();
    int oldPlotImNameCount = ui->cmbImPlot->count();
    int oldPlotOutNameCount = ui->cmbOutPlot->count();

    int oldCurveReNameCount = ui->cmbReCurve->count();
    int oldCurveImNameCount = ui->cmbImCurve->count();

    int matchingPlotReNameIndex = -1;
    int matchingPlotImNameIndex = -1;
    int matchingPlotOutNameIndex = -1;

    int matchingCurveReNameIndex = -1;
    int matchingCurveImNameIndex = -1;

    ui->cmbRePlot->clear();
    ui->cmbImPlot->clear();
    ui->cmbOutPlot->clear();
    ui->cmbReCurve->clear();
    ui->cmbImCurve->clear();

    ui->cmbOutPlot->addItem(NEW_PLOT_TEXT);

    itemIndex = 0;
    tCurveCommanderInfo::Iterator plotIter;
    for(plotIter = m_plotsAndCurves.begin(); plotIter != m_plotsAndCurves.end(); ++plotIter)
    {
        QString plotName = plotIter.key();
        ui->cmbRePlot->addItem(plotName);
        ui->cmbImPlot->addItem(plotName);
        ui->cmbOutPlot->addItem(plotName);

        if(oldPlotReNameCount > 0 && plotName == oldPlotReName)
        {
            matchingPlotReNameIndex = itemIndex;
        }
        if(oldPlotImNameCount > 0 && plotName == oldPlotImName)
        {
            matchingPlotImNameIndex = itemIndex;
        }
        if(oldPlotOutNameCount > 0 && plotName == oldPlotOutName)
        {
            // Add 1 to index for new plot line
            matchingPlotOutNameIndex = itemIndex+1;
        }
        ++itemIndex;
    }
    if(matchingPlotReNameIndex >= 0)
    {
        ui->cmbRePlot->setCurrentIndex(matchingPlotReNameIndex);
    }
    if(matchingPlotImNameIndex >= 0)
    {
        ui->cmbImPlot->setCurrentIndex(matchingPlotImNameIndex);
    }
    if(matchingPlotOutNameIndex >= 0)
    {
        ui->cmbOutPlot->setCurrentIndex(matchingPlotOutNameIndex);
    }

    if(ui->cmbRePlot->count() > 0 && ui->cmbRePlot->currentIndex() >= 0)
    {
        itemIndex = 0;
        tCurveCommanderInfo::Iterator plotIter = m_plotsAndCurves.find(ui->cmbRePlot->currentText());
        tCurveDataInfo::Iterator curveIter;
        for(curveIter = plotIter.value().curves.begin(); curveIter != plotIter.value().curves.end(); ++curveIter)
        {
            CurveData* curveData = curveIter.value();
            if(curveData->getPlotType() == E_PLOT_TYPE_2D)
            {
                QString xAxisName = curveData->getCurveTitle();
                QString yAxisName = curveData->getCurveTitle();

                xAxisName.append(X_AXIS_APPEND);
                yAxisName.append(Y_AXIS_APPEND);

                ui->cmbReCurve->addItem(xAxisName);
                if(matchingPlotReNameIndex >= 0 && oldCurveReNameCount > 0 && xAxisName == oldCurveReName)
                {
                    matchingCurveReNameIndex = itemIndex;
                }
                ui->cmbReCurve->addItem(yAxisName);
                if(matchingPlotReNameIndex >= 0 && oldCurveReNameCount > 0 && yAxisName == oldCurveReName)
                {
                    matchingCurveReNameIndex = itemIndex;
                }

            }
            else if(curveData->getPlotType() == E_PLOT_TYPE_1D)
            {
                ui->cmbReCurve->addItem(curveData->getCurveTitle());
                if(matchingPlotReNameIndex >= 0 && oldCurveReNameCount > 0 && curveData->getCurveTitle() == oldCurveReName)
                {
                    matchingCurveReNameIndex = itemIndex;
                }
            }
            ++itemIndex;

        }
    }

    if(ui->cmbImPlot->count() > 0 && ui->cmbImPlot->currentIndex() >= 0)
    {
        itemIndex = 0;
        tCurveCommanderInfo::Iterator plotIter = m_plotsAndCurves.find(ui->cmbImPlot->currentText());
        tCurveDataInfo::Iterator curveIter;
        for(curveIter = plotIter.value().curves.begin(); curveIter != plotIter.value().curves.end(); ++curveIter)
        {
            CurveData* curveData = curveIter.value();
            if(curveData->getPlotType() == E_PLOT_TYPE_2D)
            {
                QString xAxisName = curveData->getCurveTitle();
                QString yAxisName = curveData->getCurveTitle();

                xAxisName.append(X_AXIS_APPEND);
                yAxisName.append(Y_AXIS_APPEND);

                ui->cmbImCurve->addItem(xAxisName);
                if(matchingPlotImNameIndex >= 0 && oldCurveImNameCount > 0 && xAxisName == oldCurveImName)
                {
                    matchingCurveImNameIndex = itemIndex;
                }
                ui->cmbImCurve->addItem(yAxisName);
                if(matchingPlotImNameIndex >= 0 && oldCurveImNameCount > 0 && yAxisName == oldCurveImName)
                {
                    matchingCurveImNameIndex = itemIndex;
                }

            }
            else if(curveData->getPlotType() == E_PLOT_TYPE_1D)
            {
                ui->cmbImCurve->addItem(curveData->getCurveTitle());
                if(matchingPlotImNameIndex >= 0 && oldCurveImNameCount > 0 && curveData->getCurveTitle() == oldCurveImName)
                {
                    matchingCurveImNameIndex = itemIndex;
                }
            }
            ++itemIndex;
        }
    }


    if(matchingCurveReNameIndex >= 0)
    {
        ui->cmbReCurve->setCurrentIndex(matchingCurveReNameIndex);
    }
    if(matchingCurveImNameIndex >= 0)
    {
        ui->cmbImCurve->setCurrentIndex(matchingCurveImNameIndex);
    }

    if(ui->cmbOutPlot->currentText() == NEW_PLOT_TEXT)
    {
        ui->lblNewPlotName->setVisible(true);
        ui->txtOutPlotName->setVisible(true);
    }
    else
    {
        ui->lblNewPlotName->setVisible(false);
        ui->txtOutPlotName->setVisible(false);
    }


    m_guiChanging = false;
}



void createFFTPlot::on_cmdOk_clicked()
{
    bool valid = true;
    int axisAppendLen = X_AXIS_APPEND.size();

    // Init return parameters
    m_fftCurveData.plotName       = ui->cmbOutPlot->currentText();
    m_fftCurveData.curveName      = ui->txtOutCurveName->text();
    m_fftCurveData.srcRePlotName  = ui->cmbRePlot->currentText();
    m_fftCurveData.srcReCurveName = ui->cmbReCurve->currentText();
    m_fftCurveData.srcReAxis      = E_Y_AXIS;
    m_fftCurveData.srcImPlotName  = ui->cmbImPlot->currentText();
    m_fftCurveData.srcImCurveName = ui->cmbImCurve->currentText();
    m_fftCurveData.srcImAxis      = E_Y_AXIS;
    if(m_fftCurveData.plotName == NEW_PLOT_TEXT)
    {
        m_fftCurveData.plotName = ui->txtOutPlotName->text();
    }

    // If curve name is invalid check against .xAxis or .yAxis
    if(m_plotsAndCurves[m_fftCurveData.srcRePlotName].curves.find(m_fftCurveData.srcReCurveName) ==
       m_plotsAndCurves[m_fftCurveData.srcRePlotName].curves.end())
    {
        QString curveName = m_fftCurveData.srcReCurveName.left(m_fftCurveData.srcReCurveName.size() - axisAppendLen);
        QString curveAxis = m_fftCurveData.srcReCurveName.right(axisAppendLen);
        if(curveAxis == X_AXIS_APPEND)
        {
            m_fftCurveData.srcReCurveName = curveName;
            m_fftCurveData.srcReAxis = E_X_AXIS;

        }
        else if(curveAxis == Y_AXIS_APPEND)
        {
            m_fftCurveData.srcReCurveName = curveName;
            m_fftCurveData.srcReAxis = E_Y_AXIS;
        }
        else
        {
            valid = false;
        }
    }

    if(valid == true && m_fftCurveData.fftType == E_FFT_COMPLEX)
    {
        // If curve name is invalid check against .xAxis or .yAxis
        if(m_plotsAndCurves[m_fftCurveData.srcImPlotName].curves.find(m_fftCurveData.srcImCurveName) ==
           m_plotsAndCurves[m_fftCurveData.srcImPlotName].curves.end())
        {
            QString curveName = m_fftCurveData.srcImCurveName.left(m_fftCurveData.srcImCurveName.size() - axisAppendLen);
            QString curveAxis = m_fftCurveData.srcImCurveName.right(axisAppendLen);
            if(curveAxis == X_AXIS_APPEND)
            {
                m_fftCurveData.srcImCurveName = curveName;
                m_fftCurveData.srcImAxis = E_X_AXIS;

            }
            else if(curveAxis == Y_AXIS_APPEND)
            {
                m_fftCurveData.srcImCurveName = curveName;
                m_fftCurveData.srcImAxis = E_Y_AXIS;
            }
            else
            {
                valid = false;
            }
        }

    }
    if(valid)
    {
        m_validUserInput = true;
        m_parent->createFftGuiFinished();
    }

}

void createFFTPlot::on_cmdCancel_clicked()
{
    m_parent->createFftGuiFinished();
}

void createFFTPlot::closeEvent(QCloseEvent* /*event*/)
{
    m_parent->createFftGuiFinished();
}


void createFFTPlot::on_cmbRePlot_currentIndexChanged(int /*index*/)
{
    if(m_guiChanging == false)
    {
        updateGui();
    }
}

void createFFTPlot::on_cmbImPlot_currentIndexChanged(int /*index*/)
{
    if(m_guiChanging == false)
    {
        updateGui();
    }
}

void createFFTPlot::on_cmbOutPlot_currentIndexChanged(int /*index*/)
{
    if(m_guiChanging == false)
    {
        updateGui();
    }
}


bool createFFTPlot::getFFTData(tFFTCurve &fftCurveData)
{
    if(m_validUserInput == true)
    {
        fftCurveData = m_fftCurveData;
    }
    return m_validUserInput;
}

