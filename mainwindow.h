/* Copyright 2013 - 2014 Dan Williams. All Rights Reserved.
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <list>
#include <math.h>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_scale_widget.h>

#include <QLabel>
#include <QSignalMapper>
#include <QList>
#include <QMenu>
#include <QAction>
#include <QString>
#include <QCursor>
#include <QSharedPointer >

#include "PlotHelperTypes.h"
#include "TCPMsgReader.h"
#include "PlotZoom.h"
#include "Cursor.h"
#include "CurveData.h"

#define PLOT_CANVAS_OFFSET (6)

#define MIN_DISPLAY_PRECISION (4)

class CurveCommander;
class plotGuiMain;


typedef enum
{
    E_CURSOR,
    E_DELTA_CURSOR,
    E_ZOOM
}eSelectMode;


typedef struct
{
    QAction* action;
    QSignalMapper* mapper;
}tMenuActionMapper;

class tQMenuActionMapper
{
public:
   tQMenuActionMapper(QString actionStr, QObject* _this):
      m_action(actionStr, _this),
      m_mapper(_this){}
   tQMenuActionMapper(QIcon actionIcon, QString actionStr, QObject* _this):
      m_action(actionIcon, actionStr, _this),
      m_mapper(_this){}
    QAction m_action;
    QSignalMapper m_mapper;
private:
   tQMenuActionMapper();
};


class curveStyleMenuActionMapper
{
public:
   curveStyleMenuActionMapper(QwtPlotCurve::CurveStyle style, QString title, QObject* _this):
      m_qmam(title, _this),
      m_style(style){}
   curveStyleMenuActionMapper(QIcon actionIcon, QwtPlotCurve::CurveStyle style, QString title, QObject* _this):
      m_qmam(actionIcon, title, _this),
      m_style(style){}
   tQMenuActionMapper m_qmam;
   QwtPlotCurve::CurveStyle m_style;
private:
   curveStyleMenuActionMapper();
};

class curveStyleMenu
{
public:
   curveStyleMenu(QString menuStr, QObject* _this):
      m_menu(menuStr)
   {
      m_actionMapper.push_back(new curveStyleMenuActionMapper(QwtPlotCurve::Lines, "Lines", _this));
      m_actionMapper.push_back(new curveStyleMenuActionMapper(QwtPlotCurve::Dots, "Dots", _this));
      m_actionMapper.push_back(new curveStyleMenuActionMapper(QwtPlotCurve::Sticks, "Sticks", _this));
      m_actionMapper.push_back(new curveStyleMenuActionMapper(QwtPlotCurve::Steps, "Steps", _this));
   }
   ~curveStyleMenu()
   {
      for(int i = 0; i < m_actionMapper.size(); ++i)
      {
         delete m_actionMapper[i];
      }
   }

   QMenu m_menu;
   QVector<curveStyleMenuActionMapper*> m_actionMapper;
private:
   curveStyleMenu();
};


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(CurveCommander* curveCmdr, plotGuiMain* plotGui, QWidget *parent = 0);
    ~MainWindow();


    void resetPlot();
    void create1dCurve(QString name, ePlotType plotType, dubVect& yPoints);
    void create2dCurve(QString name, dubVect& xPoints, dubVect& yPoints);
    void update1dCurve(QString name, unsigned int sampleStartIndex, ePlotType plotType, dubVect &yPoints);
    void update2dCurve(QString name, unsigned int sampleStartIndex, dubVect& xPoints, dubVect& yPoints);

    void setCurveSampleRate(QString curveName, double sampleRate, bool userSpecified);

    void setCurveProperties(QString curveName, eAxis axis, double sampleRate, tMathOpList& mathOps, bool hidden);

private:
    Ui::MainWindow *ui;

    CurveCommander* m_curveCommander;
    plotGuiMain* m_plotGuiMain;


    QwtPlot* m_qwtPlot;
    QList<CurveData*> m_qwtCurves;
    Cursor* m_qwtSelectedSample;
    Cursor* m_qwtSelectedSampleDelta;
    QwtPlotPicker* m_qwtPicker;
    QwtPlotGrid* m_qwtGrid;

    eSelectMode m_selectMode;

    int m_selectedCurveIndex;

    PlotZoom* m_plotZoom;
    maxMinXY m_maxMin;

    QIcon m_checkedIcon;
    QCursor* m_zoomCursor;

    bool m_normalizeCurves;

    bool m_legendDisplayed;

    double m_canvasXOverYRatio;

    bool m_allowNewCurves;

    QList<tMenuActionMapper> m_selectedCursorActions;

    QAction m_zoomAction;
    QAction m_cursorAction;
    QAction m_deltaCursorAction;
    QAction m_resetZoomAction;
    QAction m_normalizeAction;
    QAction m_toggleLegendAction;
    QMenu m_rightClickMenu;
    QMenu m_selectedCurvesMenu;
    QMenu m_visibleCurvesMenu;
    QMenu m_stylesCurvesMenu;

    QAction m_enableDisablePlotUpdate;

    QAction m_curveProperties;

    QwtPlotCurve::CurveStyle m_defaultCurveStyle;
    QList< QSharedPointer< curveStyleMenu> > m_curveLineMenu;

    eDisplayPointType m_displayType;
    int m_displayPrecision;
    QMenu m_displayPointsMenu;
    tQMenuActionMapper m_displayPointsAutoAction;
    tQMenuActionMapper m_displayPointsFixedAction;
    tQMenuActionMapper m_displayPointsScientificAction;

    tQMenuActionMapper m_displayPointsPrecisionAutoAction;
    tQMenuActionMapper m_displayPointsPrecisionUpAction;
    tQMenuActionMapper m_displayPointsPrecisionDownAction;
    tQMenuActionMapper m_displayPointsPrecisionUpBigAction;
    tQMenuActionMapper m_displayPointsPrecisionDownBigAction;

    void createUpdateCurve( QString& name,
                            bool resetCurve,
                            unsigned int sampleStartIndex,
                            ePlotType plotType,
                            dubVect *xPoints,
                            dubVect *yPoints );

    void handleCurveDataChange(int curveIndex);

    void calcMaxMin();

    void setDisplayIoMapIp(std::stringstream &iostr);
    void clearDisplayIoMapIp(std::stringstream& iostr);
    bool setDisplayIoMapipXAxis(std::stringstream& iostr, CurveData *curve);
    void setDisplayIoMapipYAxis(std::stringstream& iostr);
    void clearPointLabels();
    void displayPointLabels();
    void displayDeltaLabel();
    void updatePointDisplay();
    void setDisplayRightClickIcons();

    void updateCursors();

    void modifySelectedCursor(int modDelta);
    void modifyCursorPos(int modDelta);

    void setSelectedCurveIndex(int index);

    void replotMainPlot();

    int findMatchingCurve(const QString& curveTitle);

    // Key Press Functions
    bool keyPressModifyZoom(int key);
    bool keyPressModifyCursor(int key);


    bool eventFilter(QObject *obj, QEvent *event);
    void closeEvent(QCloseEvent* event);
    void resizeEvent(QResizeEvent* event);


    void setCurveStyleMenu();
    void setCurveStyleMenuIcons();
private slots:
    void pointSelected(const QPointF &pos);
    void rectSelected(const QRectF &pos);

    // Menu Commands
    void toggleLegend();
    void cursorMode();
    void deltaCursorMode();
    void zoomMode();
    void resetZoom();
    void normalizeCurves();

    void visibleCursorMenuSelect(int index);
    void selectedCursorMenuSelect(int index);

    void changeCurveStyle(int inVal);

    void showCurveProperties();
    void togglePlotUpdateAbility();

    void on_verticalScrollBar_sliderMoved(int position);
    void on_horizontalScrollBar_sliderMoved(int position);

    void on_verticalScrollBar_actionTriggered(int action);
    void on_horizontalScrollBar_actionTriggered(int action);


    void onApplicationFocusChanged(QWidget* old, QWidget* now);
    void ShowRightClickForPlot(const QPoint& pos);
    void ShowRightClickForDisplayPoints(const QPoint& pos);

    void displayPointsChangeType(int type);
    void displayPointsChangePrecision(int precision);

// Functions that could be called from a thread, but modify ui
public slots:
    void updateCursorMenus();
signals:
    void updateCursorMenusSignal();

};

#endif // MAINWINDOW_H
