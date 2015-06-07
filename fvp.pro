#-------------------------------------------------
#
# Project created by QtCreator 2015-05-23T16:44:29
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = fvp
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += ordered
QMAKE_CXXFLAGS += -std=c++11

TEMPLATE = app

include(lodepng/lodepng.pri)

SOURCES += \
    fvp.cpp \
    parser.cpp \
    nvsgconverter.cpp \
    hcbdecoder.cpp \
    hcbrebuilder.cpp

HEADERS += \
    hcb_opcodes.h \
    fvp.h \
    parser.h \
    nvsgconverter.h \
    hcbdecoder.h \
    hcbrebuilder.h \
    hcb_global.h
