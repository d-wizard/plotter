#-------------------------------------------------
#
# Project created by QtCreator 2013-06-21T22:30:17
#
#-------------------------------------------------

# Run pre-build python script to generate revDateStamp.h
revDateStampTarget.target = ./revDateStamp.h
revDateStampTarget.depends = FORCE
revDateStampTarget.commands = $$system(python ./revDateStamp.py)
QMAKE_EXTRA_TARGETS += revDateStampTarget

RC_FILE = mainwindow.rc

# QWT and FFTW libraries are in directories labeled 32 or 64. Determine which directory to find the libraries in.
contains(QT_ARCH, i386) {
    ARCHDIR = 32
} else {
    ARCHDIR = 64
}

QWTDIR = ../PlotterDependencies/prebuilt/qwt_sources_and_bin/latest6.1_myMods/qwt-6.1
FFTWDIR = ../PlotterDependencies/prebuilt/fftw-dll
include ( $${QWTDIR}/qwt.prf )

QT += core gui
QT += widgets

TARGET = qwtExample
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    TCPThreads.cpp \
    TCPMsgReader.cpp \
    PackUnpackPlotMsg.cpp \
    openrawdialog.cpp \
    plotguimain.cpp \
    dString.cpp \
    FileSystemOperations.cpp \
    fftHelper.cpp \
    CurveCommander.cpp \
    PlotZoom.cpp \
    CurveData.cpp \
    ChildCurves.cpp \
    curveproperties.cpp \
    rawFileTypes.cpp \
    saveRestoreCurve.cpp \
    plotcurvenamedialog.cpp \
    overwriterenamedialog.cpp \
    smartMaxMin.cpp \
    persistentParameters.cpp \
    localPlotCreate.cpp \
    plotBar.cpp \
    plotSnrCalc.cpp \
    ipBlocker.cpp \
    update.cpp \
    sampleRateCalculator.cpp \
    createplotfromdata.cpp \
    AutoDelimiter.cpp \
    Cursor.cpp \
    fftSpectrumAnalyzerFunctions.cpp \
    curveStatsChildParam.cpp \
    zoomlimitsdialog.cpp

HEADERS  += mainwindow.h \
    TCPThreads.h \
    TCPMsgReader.h \
    PackUnpackPlotMsg.h \
    DataTypes.h \
    PlotZoom.h \
    PlotHelperTypes.h \
    Cursor.h \
    CurveData.h \
    openrawdialog.h \
    plotguimain.h \
    dString.h \
    FileSystemOperations.h \
    CurveCommander.h \
    fftHelper.h \
    ChildCurves.h \
    curveproperties.h \
    UDPThreads.h \
    saveRestoreCurve.h \
    plotcurvenamedialog.h \
    overwriterenamedialog.h \
    AmFmPmDemod.h \
    handleLogData.h \
    logToFile.h \
    smartMaxMin.h \
    persistentParameters.h \
    sendTCPPacket.h \
    localPlotCreate.h \
    plotBar.h \
    plotSnrCalc.h \
    ipBlocker.h \
    plotCurveComboBox.h \
    persistentPlotParameters.h \
    update.h \
    sampleRateCalculator.h \
    createplotfromdata.h \
    AutoDelimiter.h \
    hist.h \
    spectrumAnalyzerModeTypes.h \
    fftSpectrumAnalyzerFunctions.h \
    curveStatsChildParam.h \
    zoomlimitsdialog.h

FORMS    += mainwindow.ui \
    openrawdialog.ui \
    plotguimain.ui \
    curveproperties.ui \
    plotcurvenamedialog.ui \
    overwriterenamedialog.ui \
    createplotfromdata.ui \
    saverawdialog.ui \
    zoomlimitsdialog.ui

INCLUDEPATH += $$QWTDIR/src
INCLUDEPATH += $$FFTWDIR

qwtAddLibrary($${QWTDIR}/../lib/$${ARCHDIR}, qwt)

win32 {
    LIBS += -lws2_32
    LIBS += -L$$FFTWDIR/$$ARCHDIR -lfftw3-3
} else {
    LIBS += -L$$FFTWDIR/$$ARCHDIR -lfftw3
}

RESOURCES += \
    qtResource.qrc
