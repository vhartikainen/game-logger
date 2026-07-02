#-------------------------------------------------
#
# Project created by QtCreator 2014-04-05T13:56:27
#
#-------------------------------------------------

QT      += core gui network
LIBS    += -lpsapi -luser32 -lxinput

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GameLogger
TEMPLATE = app

debug {

#    DEFINES += MY_DEBUG
}

SOURCES += main.cpp\
    networkhandler.cpp \
    gameloggerui.cpp \
    settings.cpp \
    apmlog.cpp \
    gamelog.cpp \
    gamelogger.cpp

HEADERS  += \
    networkhandler.h \
    gameloggerui.h \
    settings.h \
    common.h \
    apmlog.h \
    gamelog.h \
    session.h \
    gamelogger.h

FORMS    += \
    gameloggerui.ui

RESOURCES += \
    gamelogger.qrc
