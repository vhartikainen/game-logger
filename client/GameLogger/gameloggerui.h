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

// Qt Charts types used only as pointers here; forward-declared to keep the
// header light. In Qt 6 these live in the global namespace.
class QChartView;
class QLineSeries;
class QValueAxis;

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
    void statsUpdated(QList<GameStat> stats);

    void refreshLog();

private:

    Ui::GameLoggerUI *ui;

    GameLogger * gameLogger;

    // Kept from settingsReady() so incoming stats (keyed by game id) can be
    // resolved to game names and logos. Not owned.
    Settings * settings;

    void createTrayIcon();

    // --- Graphs tab (Qt Charts) ---
    void setupCharts();
    void updateApmChart(int apm);
    void rebuildPlaytimeCharts(const QList<GameStat> &stats,
                               const QHash<int, QString> &names);

    // Live APM line chart, updated once per logsUpdated() tick.
    QChartView *apmChartView;
    QLineSeries *apmSeries;
    QValueAxis *apmAxisX;
    QValueAxis *apmAxisY;
    int apmSampleX;               // monotonically increasing x for APM samples

    // Playtime charts, rebuilt on each statsUpdated().
    QChartView *barChartView;
    QChartView *donutChartView;

    QTimer * quitter;

    QSystemTrayIcon *trayIcon;
    QAction *quitAction;
    QMenu   *trayIconMenu;

    QString lastError;

protected:
    void closeEvent(QCloseEvent *event);

};

#endif // GAMELOGGERUI_H
