#-------------------------------------------------
#
# Project created by QtCreator 2016-02-02T11:36:09
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = StereoCalibration
TEMPLATE = app

CONFIG += c++11

QT += websockets

#QMAKE_CXXFLAGS += -fopenmp
#LIBS += -lgomp

SOURCES += \
    echoclient.cpp \
    main.cpp \
    stereocalibration.cpp \
    ../ImageAnalysis.cpp

HEADERS  += \
    echoclient.h \
    stereocalibration.h \
    ../ImageAnalysis.h


CONFIG(debug, debug|release) {
    unix:!macx: LIBS += -L$$PWD/../../../Linux/camaroapi/build/debug/ -lcamaroapi
    unix:!macx: PRE_TARGETDEPS += $$PWD/../../../Linux/camaroapi/build/debug/libcamaroapi.a
} else {
    unix:!macx: LIBS += -L$$PWD/../../../Linux/camaroapi/build/release/ -lcamaroapi
    unix:!macx: PRE_TARGETDEPS += $$PWD/../../../Linux/camaroapi/build/release/libcamaroapi.a
}

INCLUDEPATH += ${TBBROOT}/include

unix:!macx: INCLUDEPATH += $$PWD/../../../Linux/Include
unix:!macx: DEPENDPATH += $$PWD/../../../Linux/Include

INCLUDEPATH += $$PWD/../../Include
DEPENDPATH += $$PWD/../../Include

INCLUDEPATH += $$PWD/..
DEPENDPATH += $$PWD/..

RESOURCES += \
    stereocalibration.qrc

FORMS += \
    stereocalibration.ui

unix:!macx: LIBS += -lopencv_core
unix:!macx: LIBS += -lopencv_calib3d
unix:!macx: LIBS += -lopencv_imgcodecs
unix:!macx: LIBS += -lopencv_imgproc
unix:!macx: LIBS += -ltbb
