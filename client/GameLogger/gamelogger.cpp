#include "gamelogger.h"
#include "common.h"

GameLogger::GameLogger(QString serverUrl, QString player)
{
    settings = new Settings(serverUrl, player);

    networkHandler = new NetworkHandler(settings);
    connect(networkHandler, SIGNAL(error(QString)), this, SLOT(networkError(QString)));
    // Forward informational (non-terminal) connection status to the UI.
    connect(networkHandler, SIGNAL(status(QString)), this, SIGNAL(status(QString)));

    // Transfer handling to timer thread
    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(false);
    updateTimer->setInterval(0);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(querySettings()));
    updateTimer->start();

    frameTimer = new QTimer(this);
    frameTimer->setSingleShot(false);
    frameTimer->setInterval(1000/GAMEPAD_CHECK_FPS);
    connect(frameTimer, SIGNAL(timeout()), this, SLOT(frame()));

    updates = 0;
}

GameLogger::~GameLogger() {
    updateTimer->stop();
    if (frameTimer->isActive())
        frameTimer->stop();
    delete frameTimer;
    delete updateTimer;
    delete networkHandler;
    delete apmLog;
    delete gameLog;
    delete settings;
}

void GameLogger::notifyClose()
{
    updateTimer->stop();
    networkHandler->reportNoSession(0);
}


void GameLogger::querySettings() {
    QDEBUG("[GameLogger::querySettings()] called");
    networkHandler->querySettings();
    waitedForSettings = 0;

    // The first timeout in timer was to get the settings
    disconnect(updateTimer, SIGNAL(timeout()), this, SLOT(querySettings()));
    updateTimer->setInterval(1000-QTime::currentTime().msec()); // Baseline 1 second interval/resolution
    metronome.start();
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(update()));
}

void GameLogger::frame() {
    apmLog->updateGamepad();
}

void GameLogger::update() {
    QDEBUG("[GameLogger::update()] called at %d", QTime::currentTime().msec());

    if (!settings->ready) {
        // Settings not ready yet. NetworkHandler retries the fetch with
        // backoff indefinitely, so we just keep waiting rather than giving up.
        // Surface an informational note once if it's taking a while.
        ++waitedForSettings;
        if (waitedForSettings == 30) {
            emit status("Still waiting for initial settings from server...");
        }
        return;
    } else {
        if (waitedForSettings >= 0) {
            // This is the first time the settings are ready
            gameLog = new GameLog(settings);
            apmLog = new APMLog(settings->apmBufferSize, settings->serverTime);

            // Start frame timer to apm gamepad checker
            frameTimer->start();

            // When program is fired up, stop any previous sessions.
            networkHandler->reportNoSession(apmLog->current);            

            // Notify settings changed
            emit settingsReady(settings);

            waitedForSettings = -1;
        }
    }


    // Settings are ready at this stage

    // Update APM
    if (apmLog->update()) {
        // APM buffer is filled. Send it to server
        networkHandler->sendAPM(apmLog->buffer, apmLog->bufferSize);
    }

    if ((++updates) % 60 == 1) {
        // Updage game session with current APM every minute
        gameLog->update(apmLog->current);

        if (gameLog->current) {
            // There is an ongoing session. Update to server
            networkHandler->updateSession(gameLog->current);

        } else {
            // No games are active, report no session to remove previous sessions
            // and store current apm
            networkHandler->reportNoSession(apmLog->current);
        }
    }

    // Notify observers
    emit updated(apmLog->current, gameLog->current);

    // Update timer to keep within 1 second period
    int msec = QTime::currentTime().msec();
    if (msec < 500)
        updateTimer->setInterval(1000-msec);
    else
        updateTimer->setInterval(2000-msec);
}

void GameLogger::networkError(QString errorStr)
{
    QDEBUG("[GameLogger::networkError()] called with error %s", qPrintable(errorStr));

    // Propagate error to UI
    emit error(errorStr);
}
