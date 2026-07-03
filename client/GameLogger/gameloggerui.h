#ifndef GAMELOGGERUI_H
#define GAMELOGGERUI_H

#include <QMainWindow>
#include <QDialog>
#include <QSystemTrayIcon>

#include "gamelogger.h"

#pragma once

namespace Ui {
    class GameLoggerUI;
}

class GameLoggerUI : public QDialog
{
    Q_OBJECT

public:
    explicit GameLoggerUI(QWidget *parent = 0);
    ~GameLoggerUI();

    void setup(QString serverUrl, QString player);

private slots:
    void trayClicked(QSystemTrayIcon::ActivationReason reason);

    void exitError(QString error);
    void statusUpdate(QString status);

    void startQuitting();

    void settingsReady(Settings * settings);
    void logsUpdated(int apm, Session * session);

    void refreshLog();

private:

    Ui::GameLoggerUI *ui;

    GameLogger * gameLogger;

    void createTrayIcon();        

    QTimer * quitter;

    QSystemTrayIcon *trayIcon;
    QAction *quitAction;
    QMenu   *trayIconMenu;

    QString lastError;

protected:
    void closeEvent(QCloseEvent *event);

};

#endif // GAMELOGGERUI_H
