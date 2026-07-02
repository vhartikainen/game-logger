#ifndef COMMON_H
#define COMMON_H

#include <QtDebug>

// Routed through the message handler in main.cpp into LogBuffer, viewable
// from the tray icon's "Log" tab.
#define QDEBUG qDebug

#define VERSION QString("3.1")
#define VERSION_CONTEXT QString("Summerkampf 2014")

#endif // COMMON_H
