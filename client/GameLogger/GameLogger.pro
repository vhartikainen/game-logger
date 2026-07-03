#-------------------------------------------------
#
# Project created by QtCreator 2014-04-05T13:56:27
#
#-------------------------------------------------

QT      += core gui network
LIBS    += -lpsapi -luser32 -lxinput -ladvapi32

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GameLogger
TEMPLATE = app

SOURCES += main.cpp\
    networkhandler.cpp \
    gameloggerui.cpp \
    settings.cpp \
    apmlog.cpp \
    gamelog.cpp \
    gamelogger.cpp \
    logbuffer.cpp

HEADERS  += \
    networkhandler.h \
    gameloggerui.h \
    settings.h \
    common.h \
    apmlog.h \
    gamelog.h \
    session.h \
    gamelogger.h \
    logbuffer.h

FORMS    += \
    gameloggerui.ui

RESOURCES += \
    gamelogger.qrc
