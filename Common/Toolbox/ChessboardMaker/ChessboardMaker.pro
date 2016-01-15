TEMPLATE = app

QT += core qml quick websockets

CONFIG += c++11

#CONFIG -= app_bundle

SOURCES += rcserver.cpp \
           main.cpp \
    geometryhelper.cpp


RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

DISTFILES +=

HEADERS += \
    rcserver.h \
    geometryhelper.h
