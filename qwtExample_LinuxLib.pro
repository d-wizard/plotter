#-------------------------------------------------
#
# Project created by QtCreator 2013-06-21T22:30:17
#
#-------------------------------------------------
RC_FILE = mainwindow.rc
QWTDIR = ../qwt-6.1
FFTWDIR = ../fftw-3.3.3/api
FFTWLIBDIR = /usr/local/lib
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
    createfftplot.cpp \
    CurveCommander.cpp \
    PlotZoom.cpp \
    dllHelper.cpp \
    fftPlots.cpp \
    CurveData.cpp

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
    createfftplot.h \
    CurveCommander.h \
    dllHelper.h \
    fftPlots.h \
    fftHelper.h

FORMS    += mainwindow.ui \
    plotguimain.ui \
    createfftplot.ui

INCLUDEPATH += $$QWTDIR/src
INCLUDEPATH += $$FFTWDIR
LIBS        += -L$$QWTDIR/lib -lqwt
LIBS += -L$$FFTWLIBDIR -lfftw3

RESOURCES += \
    qtResource.qrc
