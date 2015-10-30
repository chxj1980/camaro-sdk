#-------------------------------------------------
#
# Project created by QtCreator 2015-10-30T14:39:10
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = apitest
TEMPLATE = app


SOURCES += main.cpp\
        processimage.cpp \
    bayer.cpp \
    FrameRateCounter.cpp

HEADERS  += processimage.h \
    FrameRateCounter.h

CONFIG += c++11

QMAKE_CXXFLAGS += -fopenmp
LIBS += -lgomp

CONFIG(debug, debug|release) {
    unix:!macx: LIBS += -L$$PWD/../camaroapi/build/debug/ -lcamaroapi
    unix:!macx: PRE_TARGETDEPS += $$PWD/../camaroapi/build/debug/libcamaroapi.a
} else {
    unix:!macx: LIBS += -L$$PWD/../camaroapi/build/release/ -lcamaroapi
    unix:!macx: PRE_TARGETDEPS += $$PWD/../camaroapi/build/release/libcamaroapi.a
}





INCLUDEPATH += $$PWD/../Include
DEPENDPATH += $$PWD/../Include

INCLUDEPATH += $$PWD/../../Common/Include
DEPENDPATH += $$PWD/../../Common/Include
