#include <sstream>
#include <iomanip>
#include "setsampleratedialog.h"
#include "ui_setsampleratedialog.h"
#include "CurveCommander.h"

setsampleratedialog::setsampleratedialog(CurveCommander *curveCmdr, const QString &plotName, QWidget *parent) :
   QDialog(parent),
   ui(new Ui::setsampleratedialog),
   m_curveCmdr(curveCmdr),
   m_plotName(plotName)
{
   ui->setupUi(this);
   this->setWindowTitle(plotName + ": Set Sample Rate");

   // Determine the default sample rate.
   bool firstSampleRate = true;
   double defaultSampleRate = 0;
   auto curveCmdrInfo = m_curveCmdr->getCurveCommanderInfo();
   auto curveMap = curveCmdrInfo.find(m_plotName);
   if(curveMap != curveCmdrInfo.end()) // Make sure the plot name is valid.
   {
      // Find the curve that matches the index.
      for(auto& curve : curveMap->curves)
      {
         if(firstSampleRate)
         {
            firstSampleRate = false;
            defaultSampleRate = curve->getSampleRate();
         }
         else if(defaultSampleRate != curve->getSampleRate())
         {
            // The various curves have different sample rates. Just set the GUI to 0.
            defaultSampleRate = 0;
         }
      }
   }
   ui->txtSampRate->setText(QString::number(defaultSampleRate));
}

setsampleratedialog::~setsampleratedialog()
{
   delete ui;
}

void setsampleratedialog::on_cmdQuery_clicked()
{
   double sampRateToUse = 0;
   auto curveCmdrInfo = m_curveCmdr->getCurveCommanderInfo();
   auto curveMap = curveCmdrInfo.find(m_plotName);
   if(curveMap != curveCmdrInfo.end()) // Make sure the plot name is valid.
   {
      // Find the curve that matches the index.
      CurveData* curveData = nullptr;
      int index = 0;
      for(auto& curve : curveMap->curves)
      {
         curveData = curve;
         if(index == m_prevCurveIndex)
            break;
      }
      if(++m_prevCurveIndex >= curveMap->curves.size())
         m_prevCurveIndex = 0;

      // Get the sample rate.
      if(curveData != nullptr)
      {
         sampleRateSigFigs(curveData->getCalculatedSampleRateFromPlotMsgs(), sampRateToUse);
      }
   }

   // Set the sample rate Text Box.
   m_outSampleRate = sampRateToUse;
   std::stringstream calcSampRateStr;
   calcSampRateStr << std::setprecision(3) << std::fixed << sampRateToUse;
   ui->txtSampRate->setText(calcSampRateStr.str().c_str());
}

void setsampleratedialog::on_cmdApply_clicked()
{
   m_apply = (m_outSampleRate > 0 && isDoubleValid(m_outSampleRate)); // Only apply the sample rate change if it is a valid sample rate.
   this->accept(); // Return from exec()
}

void setsampleratedialog::on_cmdCancel_clicked()
{
   this->accept(); // Return from exec()
}
