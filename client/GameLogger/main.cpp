#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <windows.h>
#include <lmcons.h>
#include "gameloggerui.h"
#include "logbuffer.h"

void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    const char *typeName;
    switch (type) {
    case QtWarningMsg:  typeName = "WARNING";  break;
    case QtCriticalMsg: typeName = "CRITICAL"; break;
    case QtFatalMsg:    typeName = "FATAL";    break;
    case QtInfoMsg:     typeName = "INFO";     break;
    default:            typeName = "DEBUG";    break;
    }

    LogBuffer::instance()->append(QString("[%1] [%2] %3")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"))
        .arg(typeName)
        .arg(msg));
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(logMessageHandler);

    QApplication a(argc, argv);

    // Refuse to start a second instance. The mutex handle is intentionally
    // never closed; it lives for the process lifetime and is released by
    // Windows on exit.
    HANDLE singleInstanceMutex = CreateMutexW(NULL, TRUE, L"GameLoggerSingleInstanceMutex");
    Q_UNUSED(singleInstanceMutex);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Warning);
        msg.setText("GameLogger is already running.");
        msg.exec();
        return -1;
    }

    const QString defaultUrl = "http://gamelogger.duckdns.org/gamelogger";

    QStringList args = a.arguments();

    // Player name: use the first argument if given, otherwise fall back to the
    // current Windows user name.
    QString player = (args.size() >= 2) ? args.at(1) : QString();
    if (player.isEmpty()) {
        wchar_t nameBuf[UNLEN + 1];
        DWORD nameLen = UNLEN + 1;
        if (GetUserNameW(nameBuf, &nameLen) && nameLen > 1) {
            // GetUserNameW reports nameLen including the null terminator.
            player = QString::fromWCharArray(nameBuf, static_cast<int>(nameLen) - 1);
        }
    }
    if (player.isEmpty()) {
        QMessageBox msg;
        msg.setText("Could not determine the Windows user name. Pass it as the first argument.");
        msg.exec();
        return -1;
    }

    // Settings URL: use the second argument if given, otherwise the default.
    QString serverUrl = (args.size() >= 3 && !args.at(2).isEmpty()) ? args.at(2) : defaultUrl;

    GameLoggerUI w(0);
    w.setup(serverUrl, player);

//    w.show();
    return a.exec();
}
