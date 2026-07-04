#include "gameloggerui.h"
#include "ui_gameloggerui.h"

#include <QMessageBox>
#include <QDialog>
#include <QAction>
#include <QMenu>
#include <QCloseEvent>
#include <QScrollBar>
#include <QTreeWidgetItem>
#include <QDateTime>
#include <QPainter>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QPieSeries>

#include "common.h"
#include "settings.h"
#include "logbuffer.h"

// How many APM samples the live chart keeps in view. logsUpdated() fires
// roughly once per second, so this is about two minutes of history.
static const int APM_WINDOW = 120;

// How many games get their own bar/slice before the rest fold into "Other".
static const int TOP_N = 8;

// Human-readable play time, showing the two most significant units.
static QString formatDuration(qint64 seconds)
{
    qint64 minutes = seconds / 60;
    if (minutes < 1)
        return "<1 min";
    qint64 hours = minutes / 60;
    qint64 days = hours / 24;
    if (days > 0)
        return QString("%1d %2h").arg(days).arg(hours % 24);
    if (hours > 0)
        return QString("%1h %2m").arg(hours).arg(minutes % 60);
    return QString("%1 min").arg(minutes);
}

// Unix timestamp -> local date/time, or a dash when unknown.
static QString formatTimestamp(qint64 unixSeconds)
{
    if (unixSeconds <= 0)
        return "-";
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(unixSeconds * 1000LL);
    return dt.toString("d.M.yyyy h:mm");
}

GameLoggerUI::GameLoggerUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GameLoggerUI),
    settings(0)
{
    ui->setupUi(this);

    createTrayIcon();
    setupCharts();

    connect(LogBuffer::instance(), SIGNAL(updated()), this, SLOT(refreshLog()));
    refreshLog();
}

void GameLoggerUI::setupCharts()
{
    apmSampleX = 0;

    // --- Live APM line chart ---------------------------------------------
    apmSeries = new QLineSeries();

    QChart *apmChart = new QChart();
    apmChart->addSeries(apmSeries);
    apmChart->legend()->hide();
    apmChart->setTitle("Live APM (last 2 min)");
    apmChart->setMargins(QMargins(4, 4, 4, 4));

    apmAxisX = new QValueAxis();
    apmAxisX->setRange(0, APM_WINDOW - 1);
    apmAxisX->setLabelsVisible(false);
    apmChart->addAxis(apmAxisX, Qt::AlignBottom);
    apmSeries->attachAxis(apmAxisX);

    apmAxisY = new QValueAxis();
    apmAxisY->setRange(0, 60);
    apmAxisY->setLabelFormat("%d");
    apmChart->addAxis(apmAxisY, Qt::AlignLeft);
    apmSeries->attachAxis(apmAxisY);

    apmChartView = new QChartView(apmChart);
    apmChartView->setRenderHint(QPainter::Antialiasing);
    ui->apmChartContainer->layout()->addWidget(apmChartView);

    // --- Playtime bar chart & share donut (populated in statsUpdated) -----
    barChartView = new QChartView(new QChart());
    barChartView->setRenderHint(QPainter::Antialiasing);
    barChartView->chart()->setTitle("Time played per game");
    barChartView->chart()->legend()->hide();
    ui->barChartContainer->layout()->addWidget(barChartView);

    donutChartView = new QChartView(new QChart());
    donutChartView->setRenderHint(QPainter::Antialiasing);
    donutChartView->chart()->setTitle("Share of total playtime");
    ui->donutChartContainer->layout()->addWidget(donutChartView);
}

void GameLoggerUI::updateApmChart(int apm)
{
    apmSeries->append(apmSampleX, apm);

    // Drop samples that have scrolled off the left edge of the window.
    if (apmSeries->count() > APM_WINDOW)
        apmSeries->removePoints(0, apmSeries->count() - APM_WINDOW);

    // Slide the X window so the most recent APM_WINDOW samples stay visible.
    qreal maxX = qMax(apmSampleX, APM_WINDOW - 1);
    apmAxisX->setRange(maxX - (APM_WINDOW - 1), maxX);

    // Grow the Y axis to fit the peak currently in view (never below 60).
    qreal peak = 60;
    const QList<QPointF> pts = apmSeries->points();
    for (int i = 0; i < pts.size(); ++i)
        peak = qMax(peak, pts.at(i).y());
    apmAxisY->setRange(0, peak * 1.1);

    apmSampleX++;
}

void GameLoggerUI::rebuildPlaytimeCharts(const QList<GameStat> &stats,
                                         const QHash<int, QString> &names)
{
    // Aggregate totals for the KPI header.
    qint64 totalSeconds = 0;
    int totalSessions = 0;
    for (int i = 0; i < stats.size(); ++i) {
        totalSeconds += stats.at(i).seconds;
        totalSessions += stats.at(i).sessions;
    }

    const int shown = qMin(TOP_N, stats.size());

    // --- Bar chart: top N games by hours played. Iterate ascending so the
    // largest bar lands at the top of the horizontal axis. --------------
    QHorizontalBarSeries *barSeries = new QHorizontalBarSeries();
    QBarSet *barSet = new QBarSet("Hours");
    QStringList categories;
    qreal maxHours = 0;
    for (int i = shown - 1; i >= 0; --i) {
        const GameStat &s = stats.at(i);
        qreal hours = s.seconds / 3600.0;
        *barSet << hours;
        categories << names.value(s.gameid, QString("Game #%1").arg(s.gameid));
        maxHours = qMax(maxHours, hours);
    }
    barSeries->append(barSet);

    QChart *barChart = new QChart();
    barChart->setTitle("Time played per game");
    barChart->legend()->hide();
    barChart->addSeries(barSeries);
    QBarCategoryAxis *catAxis = new QBarCategoryAxis();
    catAxis->append(categories);
    barChart->addAxis(catAxis, Qt::AlignLeft);
    barSeries->attachAxis(catAxis);
    QValueAxis *valAxis = new QValueAxis();
    valAxis->setRange(0, qMax(1.0, maxHours * 1.1));
    valAxis->setTitleText("Hours");
    barChart->addAxis(valAxis, Qt::AlignBottom);
    barSeries->attachAxis(valAxis);

    QChart *oldBar = barChartView->chart();
    barChartView->setChart(barChart);   // view takes ownership of the new chart
    delete oldBar;                       // ... but not of the old one

    // --- Donut: share of playtime, small games folded into "Other". ------
    QPieSeries *pie = new QPieSeries();
    pie->setHoleSize(0.35);
    qint64 otherSeconds = 0;
    for (int i = 0; i < stats.size(); ++i) {
        if (i < TOP_N)
            pie->append(names.value(stats.at(i).gameid,
                                    QString("Game #%1").arg(stats.at(i).gameid)),
                        (double)stats.at(i).seconds);
        else
            otherSeconds += stats.at(i).seconds;
    }
    if (otherSeconds > 0)
        pie->append("Other", (double)otherSeconds);

    QChart *donutChart = new QChart();
    donutChart->setTitle("Share of total playtime");
    donutChart->legend()->setAlignment(Qt::AlignRight);
    donutChart->addSeries(pie);

    QChart *oldDonut = donutChartView->chart();
    donutChartView->setChart(donutChart);
    delete oldDonut;

    // --- KPI header ------------------------------------------------------
    const QString topGame = stats.isEmpty()
        ? QString("-")
        : names.value(stats.first().gameid,
                      QString("Game #%1").arg(stats.first().gameid));
    const qint64 avgSession = (totalSessions > 0) ? totalSeconds / totalSessions : 0;

    ui->kpiLabel->setText(QString(
        "<b>Total played:</b> %1 &nbsp;&nbsp;&nbsp; "
        "<b>Sessions:</b> %2 &nbsp;&nbsp;&nbsp; "
        "<b>Top game:</b> %3 &nbsp;&nbsp;&nbsp; "
        "<b>Avg session:</b> %4")
        .arg(formatDuration(totalSeconds))
        .arg(totalSessions)
        .arg(topGame)
        .arg(formatDuration(avgSession)));
}

void GameLoggerUI::setup(QString serverUrl, QString player) {
    QDEBUG("[GameLoggerUI::setup()] called with url %s and player %s", qPrintable(serverUrl), qPrintable(player));

    // Populate the Settings tab immediately from the startup parameters so the
    // user can verify them even if fetching settings from the server fails.
    ui->userLabel->setText(player.isEmpty() ? "<not set>" : player);
    ui->serverURLLabel->setText(serverUrl.isEmpty() ? "<not set>" : serverUrl);
    ui->suppressLabel->setText("<pending server settings>");
    ui->statusLabel->setText("Fetching settings from server...");

    gameLogger = new GameLogger(serverUrl, player);
    connect(gameLogger, SIGNAL(error(QString)), this, SLOT(exitError(QString)));
    connect(gameLogger, SIGNAL(status(QString)), this, SLOT(statusUpdate(QString)));
    connect(gameLogger, SIGNAL(settingsReady(Settings*)), this, SLOT(settingsReady(Settings*)));
    connect(gameLogger, SIGNAL(updated(int,Session*)), this, SLOT(logsUpdated(int,Session*)));
    connect(gameLogger, SIGNAL(statsReady(QList<GameStat>)), this, SLOT(statsUpdated(QList<GameStat>)));
}

GameLoggerUI::~GameLoggerUI()
{
    delete quitter;
    delete gameLogger;
    delete ui;
}

void GameLoggerUI::startQuitting()
{
    // Send close information to gameLogger
    gameLogger->notifyClose();

    quitter = new QTimer(this);
    quitter->setSingleShot(true);
    quitter->setInterval(1000);
    quitter->start();

    connect(quitter, SIGNAL(timeout()), qApp, SLOT(quit()));
}

void GameLoggerUI::settingsReady(Settings *settings)
{
    // Keep the settings around so incoming stats (keyed by game id) can be
    // resolved to human-readable names.
    this->settings = settings;

    // Show game data
    for (QHash<QString, GameInfo*>::iterator iter = settings->games.begin(); iter != settings->games.end(); ++iter) {
        QStringList lst;
        lst.append(iter.key());
        lst.append(((GameInfo *)iter.value())->completeName);
        ui->gameTree->addTopLevelItem(new QTreeWidgetItem(lst));
    }

    // Show general settings
    ui->serverURLLabel->setText(settings->serverUrl);
    ui->userLabel->setText(settings->player);
    ui->suppressLabel->setText((settings->supressUpdates)?"true":"false");
    ui->statusLabel->setText("Settings loaded, running.");
}

void GameLoggerUI::logsUpdated(int apm, Session *session)
{
    ui->apmLabel->setText(QString::number(apm));
    updateApmChart(apm);
    if (session) {
        ui->gameNameLabel->setText(session->gameName);
        ui->durationLabel->setText("Started at " + session->begun.toString("d.M.yyyy h:mm:ss") + ", played for " + QTime(0,0).addSecs(session->begun.secsTo(session->updated)).toString("h:mm:ss"));

    } else {
        ui->gameNameLabel->setText("<no active game>");
        ui->durationLabel->setText("");
    }
}

void GameLoggerUI::statsUpdated(QList<GameStat> stats)
{
    // Build an id -> name lookup from settings (games are keyed by exe name).
    QHash<int, QString> names;
    if (settings) {
        for (QHash<QString, GameInfo*>::const_iterator iter = settings->games.constBegin();
             iter != settings->games.constEnd(); ++iter) {
            names.insert(iter.value()->id, iter.value()->completeName);
        }
    }

    // Server returns rows already ranked by time played (descending).
    ui->statsTree->clear();
    for (int i = 0; i < stats.size(); ++i) {
        const GameStat &s = stats.at(i);
        QString name = names.value(s.gameid, QString("Game #%1").arg(s.gameid));

        QStringList cols;
        cols << name
             << formatDuration(s.seconds)
             << QString::number(s.sessions)
             << formatTimestamp(s.lastPlayed);

        QTreeWidgetItem *item = new QTreeWidgetItem(cols);
        item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        ui->statsTree->addTopLevelItem(item);
    }

    for (int c = 0; c < ui->statsTree->columnCount(); ++c)
        ui->statsTree->resizeColumnToContents(c);

    // Feed the same data (plus the id->name map) into the Graphs tab.
    rebuildPlaytimeCharts(stats, names);
}

void GameLoggerUI::refreshLog()
{
    QScrollBar *scrollBar = ui->logView->verticalScrollBar();
    bool atBottom = scrollBar->value() >= scrollBar->maximum() - 4;

    ui->logView->setPlainText(LogBuffer::instance()->lines().join("\n"));

    if (atBottom)
        scrollBar->setValue(scrollBar->maximum());
}

void GameLoggerUI::createTrayIcon() {
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/images/thumbs_up_48.png"));
    trayIcon->show();
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClicked(QSystemTrayIcon::ActivationReason)));

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(startQuitting()));

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayIconMenu);
}

void GameLoggerUI::trayClicked(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        showNormal();
    }
}

void GameLoggerUI::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void GameLoggerUI::exitError(QString error)
{
    QDEBUG("[GameLoggerUI::exitError()] called with error %s", qPrintable(error));

    // Surface the error on the Settings tab (and via a tray notification) so
    // the user can tell startup/connection failed rather than silently hanging.
    // Network errors can repeat every poll, so only notify when it changes.
    ui->statusLabel->setText("Error: " + error);
    if (trayIcon && error != lastError)
        trayIcon->showMessage("GameLogger", error, QSystemTrayIcon::Warning);
    lastError = error;
}

void GameLoggerUI::statusUpdate(QString status)
{
    QDEBUG("[GameLoggerUI::statusUpdate()] %s", qPrintable(status));

    // Non-terminal connection status (retrying, reconnected, ...). Update the
    // Settings tab only -- no tray notification, to avoid spam on flaky links.
    ui->statusLabel->setText(status);
    // A fresh non-error status means we're no longer sitting on a hard error.
    lastError.clear();
}
