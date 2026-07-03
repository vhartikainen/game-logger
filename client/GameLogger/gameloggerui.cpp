#include "gameloggerui.h"
#include "ui_gameloggerui.h"

#include <QMessageBox>
#include <QDialog>
#include <QAction>
#include <QMenu>
#include <QCloseEvent>
#include <QScrollBar>

#include "common.h"
#include "settings.h"
#include "logbuffer.h"

GameLoggerUI::GameLoggerUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GameLoggerUI)
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
