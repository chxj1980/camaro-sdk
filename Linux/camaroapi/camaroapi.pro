#-------------------------------------------------
#
# Project created by QtCreator 2015-10-30T14:28:19
#
#-------------------------------------------------

QT       -= core gui

TARGET = camaroapi
TEMPLATE = lib
CONFIG += staticlib

message(Platform: $$QMAKE_HOST.arch)

unix {
    target.path = /usr/lib
    INSTALLS += target
}

#contains(QMAKE_HOST.arch, armv7l)
#{
#    QMAKE_CXXFLAGS += -mfpu=neon -mfloat-abi=hard
#}

CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/build/debug
} else {
    DESTDIR = $$PWD/build/release
}

INCLUDEPATH += ../../Common/Include
INCLUDEPATH += ../Include

CONFIG += c++11

#LIBS += -lusb-1.0

HEADERS += \
    ../../Common/Include/BufferQueue.h \
    ../../Common/Include/Camaro.h \
    ../../Common/Include/CamaroDual.h \
    ../../Common/Include/CameraBase.h \
    ../../Common/Include/CameraSoloBase.h \
    ../../Common/Include/DeepCamAPI.h \
    ../../Common/Include/DeviceDecorator.h \
    ../../Common/Include/ExtensionInfo.h \
    ../../Common/Include/ExtensionRepository.h \
    ../../Common/Include/ExtensionVCDevice.h \
    ../../Common/Include/GenericVCDevice.h \
    ../../Common/Include/ICameraControl.h \
    ../../Common/Include/ICameraFactory.h \
    ../../Common/Include/IDeviceControl.h \
    ../../Common/Include/IDeviceFactory.h \
    ../../Common/Include/IDiscernible.h \
    ../../Common/Include/IExtensionAccess.h \
    ../../Common/Include/IExtensionLite.h \
    ../../Common/Include/IGenericVCDevice.h \
    ../../Common/Include/ILowlevelControl.h \
    ../../Common/Include/IMultiVideoSource.h \
    ../../Common/Include/IMultiVideoStream.h \
    ../../Common/Include/IProcessor.h \
    ../../Common/Include/IValidation.h \
    ../../Common/Include/IVideoFrame.h \
    ../../Common/Include/IVideoStream.h \
    ../../Common/Include/StandardUVC.h \
    ../../Common/Include/StandardUVCFilter.h \
    ../../Common/Include/StandardVCDevice.h \
    ../../Common/Include/VideoFormat.h \
    ../../Common/Include/VideoFrameEx.h \
    ../../Common/Include/VideoSourceProxy.h \
    ../Include/CameraFactory.h \
    ../Include/DeviceFactory.h \
    ../Include/ExtensionAccess.h \
    ../Include/ExtensionFilterBase.h \
    ../Include/LSource.h \
    ../Include/v4l2helper.h \
    ../Include/VideoBufferLock.h \
    ../Include/VideoSourceReader.h \
    ../../Common/Include/ExtensionAccessAdapter.h \
    ../../Common/Include/json.h \
    ../../Common/Include/json-forwards.h \
    ../../Common/Include/CameraProfile.h \
    ../../Common/Include/DGExtensionFilter.h \
    ../../Common/Include/EtronExtensionFilter.h \
    ../../Common/Include/ImpalaE.h \
    ../../Common/Include/Logger.h \
    ../../Common/Include/pointgrey.h \
#    ../../Common/Include/flycapturesource.h \
#    ../../Common/Include/flycapturedevice.h \
#    ../../Common/Include/flycapturereader.h \
    ../../Common/Include/CamaroISP.h \
    ../../Common/Include/Fovea.h \
    ../../Common/Include/RetrievalMap.h \
    ../../Common/Include/ThreadPool.h

SOURCES += \
    ../../Common/Source/Camaro.cpp \
    ../../Common/Source/CamaroDual.cpp \
    ../../Common/Source/ExtensionRepository.cpp \
    ../../Common/Source/VideoSourceProxy.cpp \
    ../Source/ExtensionAccess.cpp \
    ../Source/ExtensionFilterBase.cpp \
    ../Source/GenericVCDevice.cpp \
    ../Source/StandardUVCFilter.cpp \
    ../Source/v4l2helper.cpp \
    ../Source/VideoSourceReader.cpp \
    ../../Common/Source/CameraProfile.cpp \
    ../../Common/Source/jsoncpp.cpp \
    ../../Common/Source/DeepCamAPI.cpp \
    ../../Common/Source/DGExtensionFilter.cpp \
    ../../Common/Source/ImpalaE.cpp \
    ../../Common/Source/Logger.cpp \
#    ../../Common/Source/pointgrey.cpp \
#    ../../Common/Source/flycapturedevice.cpp \
#    ../../Common/Source/flycapturereader.cpp \
    ../../Common/Source/CamaroISP.cpp \
    ../../Common/Source/VideoFormat.cpp \
    ../../Common/Source/Fovea.cpp \
    ../../Common/Source/RetrievalMap.cpp \
    ../../Common/Source/IProcessor.cpp
