#-------------------------------------------------
#
# Project created by QtCreator 2015-11-30T16:51:46
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = stereoview
TEMPLATE = app


SOURCES += main.cpp\
        processimage.cpp \
    ../apitest/bayer.cpp \
    ../apitest/FrameRateCounter.cpp \
    recmgr.c \
    msgq.c \
    SDL/src/thread/pthread/SDL_syscond.c \
    SDL/src/thread/pthread/SDL_sysmutex.c \
    SDL/src/thread/pthread/SDL_syssem.c \
    SDL/src/thread/pthread/SDL_systhread.c \
    SDL/src/thread/SDL_thread.c \
    SDL/src/timer/unix/SDL_systimer.c \
    SDL/src/timer/SDL_timer.c \
    SDL/src/SDL_error.c

HEADERS  += processimage.h \
    ../apitest/FrameRateCounter.h \
    recmgr.h \
    msgq.h \
    SDL/include/begin_code.h \
    SDL/include/close_code.h \
    SDL/include/SDL.h \
    SDL/include/SDL_config.h \
    SDL/include/SDL_config_minimal.h \
    SDL/include/SDL_error.h \
    SDL/include/SDL_mutex.h \
    SDL/include/SDL_platform.h \
    SDL/include/SDL_stdinc.h \
    SDL/include/SDL_thread.h \
    SDL/include/SDL_timer.h \
    SDL/include/SDL_types.h \
    SDL/src/thread/pthread/SDL_sysmutex_c.h \
    SDL/src/thread/pthread/SDL_systhread_c.h \
    SDL/src/thread/SDL_systhread.h \
    SDL/src/thread/SDL_thread_c.h \
    SDL/src/timer/SDL_systimer.h \
    SDL/src/timer/SDL_timer_c.h \
    SDL/src/SDL_error_c.h

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

INCLUDEPATH += SDL/include
