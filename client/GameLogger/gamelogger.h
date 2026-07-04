#ifndef GAMELOGGER_H
#define GAMELOGGER_H

#include <QTimer>
#include <QElapsedTimer>

#include "apmlog.h"
#include "gamelog.h"
#include "networkhandler.h"
#include "settings.h"

class GameLogger : public QObject
{
    Q_OBJECT
public:
    GameLogger(QString serverUrl, QString player);
    ~GameLogger();

    void notifyClose();

    GameLog *gameLog;
    APMLog *apmLog;

    Settings *settings;

public slots:
    void querySettings();
    void update();
    void frame();
    void networkError(QString error);

signals:
    void error(QString error);
    void status(QString status);
    void settingsReady(Settings * settings);
    void updated(int apm, Session * session);
    void statsReady(QList<GameStat> stats);

private:

    NetworkHandler *networkHandler;

    int waitedForSettings;

    QTimer *updateTimer;
    QTimer *frameTimer;

    QElapsedTimer metronome;

    int updates;
};

#endif // GAMELOGGER_H
