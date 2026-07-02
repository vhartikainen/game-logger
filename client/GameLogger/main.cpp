#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <windows.h>
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

    QStringList args = a.arguments();
    if (args.size() < 3) {
        QMessageBox msg;
        msg.setText("Two input arguments required: first is the user name, second is the url to settings.");
        msg.exec();
        return -1;
    }
    GameLoggerUI w(0);
    w.setup(args.at(2),args.at(1));

//    w.show();
    return a.exec();
}
