#-------------------------------------------------
#
# Project created by QtCreator 2013-06-21T22:30:17
#
#-------------------------------------------------
RC_FILE = mainwindow.rc
QWTDIR = F:/qwt-6.1
PTHREADDIR = F:/Projects/pthread32
FFTWDIR = F:/fftw-3.3.3-dll32
include ( F:/qwt-6.1/qwt.prf )

QT       += core gui

TARGET = qwtExample
TEMPLATE = app


SOURCES += main.cpp\
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
    PlotZoom.cpp

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
    fftHelper.h \
    CurveCommander.h

FORMS    += mainwindow.ui \
    createfftplot.ui

INCLUDEPATH += $$QWTDIR/src
INCLUDEPATH += $$PTHREADDIR
INCLUDEPATH += $$FFTWDIR
LIBS        += -L$$QWTDIR/lib -lqwt
LIBS += -L$$PTHREADDIR -lpthreadGC2
LIBS += -lws2_32
LIBS += -L$$FFTWDIR -lfftw3-3
