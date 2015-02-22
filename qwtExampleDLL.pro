#-------------------------------------------------
#
# Project created by QtCreator 2013-06-21T22:30:17
#
#-------------------------------------------------
RC_FILE = mainwindow.rc
QWTDIR = ../qwt-6.1
PTHREADDIR = ../pthread32
FFTWDIR = ../fftw-3.3.3-dll32
include ( ../qwt-6.1/qwt.prf )

QT       += core gui

TARGET = qwtExample
TEMPLATE = lib


SOURCES +=\
        mainwindow.cpp \
    TCPThreads.cpp \
    TCPMsgReader.cpp \
    PackUnpackPlotMsg.cpp \
    plotguimain.cpp \
    dString.cpp \
    FileSystemOperations.cpp \
    fftHelper.cpp \
    CurveCommander.cpp \
    PlotZoom.cpp \
    dllHelper.cpp \
    CurveData.cpp \
    ChildCurves.cpp \
    curveproperties.cpp \
    saveRestoreCurve.cpp \
    plotcurvenamedialog.cpp \
    overwriterenamedialog.cpp

HEADERS  += mainwindow.h \
    TCPThreads.h \
    TCPMsgReader.h \
    PackUnpackPlotMsg.h \
    DataTypes.h \
    PlotZoom.h \
    PlotHelperTypes.h \
    Cursor.h \
    CurveData.h \
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
    dllHelper.h

FORMS    += mainwindow.ui \
    plotguimain.ui \
    curveproperties.ui \
    plotcurvenamedialog.ui \
    overwriterenamedialog.ui

INCLUDEPATH += $$QWTDIR/src
INCLUDEPATH += $$PTHREADDIR
INCLUDEPATH += $$FFTWDIR
LIBS        += -L$$QWTDIR/lib -lqwt
LIBS += -L$$PTHREADDIR -lpthreadGC2
LIBS += -lws2_32
LIBS += -L$$FFTWDIR -lfftw3-3

RESOURCES += \
    qtResource.qrc
