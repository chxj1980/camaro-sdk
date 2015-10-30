#-------------------------------------------------
#
# Project created by QtCreator 2015-10-30T14:28:19
#
#-------------------------------------------------

QT       -= core gui

TARGET = camaroapi
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    ../../Common/Source/Camaro.cpp \
    ../../Common/Source/CamaroDual.cpp \
    ../Source/CameraFactory.cpp \
    ../Source/DeviceFactory.cpp \
    ../Source/ExtensionAccess.cpp \
    ../Source/GeneralExtensionFilter.cpp \
    ../Source/GenericVCDevice.cpp \
    ../Source/StandardUVCFilter.cpp \
    ../Source/v4l2helper.cpp \
    ../Source/VideoSourceReader.cpp

HEADERS += \
    ../../Common/Include/BufferQueue.h \
    ../../Common/Include/Camaro.h \
    ../../Common/Include/CamaroDual.h \
    ../../Common/Include/CameraBase.h \
    ../../Common/Include/CameraComboBase.h \
    ../../Common/Include/ExtensionInfo.h \
    ../../Common/Include/ICameraControl.h \
    ../../Common/Include/ICameraFactory.h \
    ../../Common/Include/IDeviceControl.h \
    ../../Common/Include/IDeviceFactory.h \
    ../../Common/Include/IExtensionAccess.h \
    ../../Common/Include/IExtensionLite.h \
    ../../Common/Include/IGenericVCDevice.h \
    ../../Common/Include/ILowlevelControl.h \
    ../../Common/Include/IVideoFrame.h \
    ../../Common/Include/IVideoStream.h \
    ../../Common/Include/StandardUVC.h \
    ../../Common/Include/VideoFormat.h \
    ../../Common/Include/VideoFrameEx.h \
    ../Include/CameraFactory.h \
    ../Include/DeviceFactory.h \
    ../Include/ExtensionAccess.h \
    ../Include/GeneralExtensionFilter.h \
    ../Include/GenericVCDevice.h \
    ../Include/ILExtensionLite.h \
    ../Include/ILSource.h \
    ../Include/StandardUVCFilter.h \
    ../Include/v4l2helper.h \
    ../Include/VideoBufferLock.h \
    ../Include/VideoSourceReader.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}

CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/build/debug
} else {
    DESTDIR = $$PWD/build/release
}

INCLUDEPATH += ../../Common/Include
INCLUDEPATH += ../Include

CONFIG += c++11

#LIBS += -lusb-1.0
