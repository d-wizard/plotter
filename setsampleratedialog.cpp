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
}

setsampleratedialog::~setsampleratedialog()
{
   delete ui;
}

void setsampleratedialog::on_cmdQuery_clicked()
{
   double setSampleRate = 0;
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
         setSampleRate = curveData->getCalculatedSampleRateFromPlotMsgs();
      }
   }

   ui->txtSampRate->setText(QString::number(setSampleRate));
}
