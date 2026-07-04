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

#include "common.h"
#include "settings.h"
#include "logbuffer.h"

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

    connect(LogBuffer::instance(), SIGNAL(updated()), this, SLOT(refreshLog()));
    refreshLog();
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
