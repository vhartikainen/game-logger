#include "networkhandler.h"
#include "common.h"

#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

// Retry policy for transient connection failures. Backoff is exponential,
// starting at RETRY_BASE_MS and doubling up to RETRY_MAX_MS.
//
// Settings are retried forever (the client can't run without them). Fire-and-
// forget data requests (session/APM updates) are retried a bounded number of
// times, after which they are dropped -- the next loop iteration supersedes
// them with fresher data anyway.
static const int RETRY_BASE_MS    = 1000;
static const int RETRY_MAX_MS     = 30000;
static const int MAX_DATA_RETRIES = 5;

NetworkHandler::NetworkHandler(Settings *settings) : settings(settings), wasFailing(false)
{

}

int NetworkHandler::retryDelay(int attempt) const
{
    // Cap the shift so we never overflow, then clamp to the max delay.
    int shift = qMin(attempt, 16);
    qint64 delay = (qint64)RETRY_BASE_MS << shift;
    return (int)qMin<qint64>(delay, RETRY_MAX_MS);
}

void NetworkHandler::sendRequest(const QNetworkRequest &req, RequestKind kind, int attempt)
{
    const char *kindStr = (kind == ReqSettings) ? "settings" : (kind == ReqStats) ? "stats" : "data";
    QDEBUG("[NetworkHandler::sendRequest()] %s (attempt %d) URL: %s",
           kindStr, attempt + 1, qPrintable(req.url().toString()));

    QNetworkReply *rep = qnam.get(req);
    RequestState state;
    state.request = req;
    state.attempt = attempt;
    state.kind = kind;
    pending.insert(rep, state);

    // finished() fires for both success and error, so it's the only signal we
    // need to drive success handling, retries and cleanup.
    connect(rep, SIGNAL(finished()), this, SLOT(replyFinished()));
}

void NetworkHandler::replyFinished()
{
    QNetworkReply *rep = qobject_cast<QNetworkReply *>(sender());
    if (!rep)
        return;

    RequestState state = pending.take(rep);
    rep->deleteLater();

    if (rep->error() != QNetworkReply::NoError) {
        handleFailure(rep->errorString(), state);
        return;
    }

    // Success. Announce recovery if we had been in a failing state.
    if (wasFailing) {
        wasFailing = false;
        emit status("Connection restored.");
    }

    if (state.kind == ReqSettings) {
        QString data = QString(rep->readAll());
        settings->parseSettings(data);
        QDEBUG("[NetworkHandler::replyFinished()] settings read");
    } else if (state.kind == ReqStats) {
        parseStats(rep->readAll());
    }
}

void NetworkHandler::parseStats(const QByteArray &data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        QDEBUG("[NetworkHandler::parseStats()] bad stats response: %s", qPrintable(err.errorString()));
        return;
    }

    QList<GameStat> stats;
    const QJsonArray arr = doc.array();
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject obj = arr.at(i).toObject();
        GameStat s;
        s.gameid     = obj.value("gameid").toInt();
        s.seconds    = (qint64)obj.value("seconds").toDouble();
        s.sessions   = obj.value("sessions").toInt();
        s.lastPlayed = (qint64)obj.value("lastPlayed").toDouble();
        stats.append(s);
    }

    QDEBUG("[NetworkHandler::parseStats()] parsed %d game stats", stats.size());
    emit statsReady(stats);
}

void NetworkHandler::handleFailure(const QString &errStr, const RequestState &state)
{
    wasFailing = true;

    const bool canRetry = (state.kind == ReqSettings) || state.attempt < MAX_DATA_RETRIES;
    if (!canRetry) {
        QDEBUG("[NetworkHandler::handleFailure()] giving up: %s", qPrintable(errStr));
        emit error(errStr + " (gave up after " + QString::number(state.attempt + 1) + " attempts)");
        return;
    }

    const int delay = retryDelay(state.attempt);
    QDEBUG("[NetworkHandler::handleFailure()] %s -- retrying in %d ms", qPrintable(errStr), delay);
    emit status("Connection problem: " + errStr
                + " -- retrying in " + QString::number(delay / 1000) + "s");

    // Capture by value so the request survives until the timer fires. `this`
    // as the context object cancels the retry if the handler is destroyed.
    const QNetworkRequest req = state.request;
    const RequestKind kind = state.kind;
    const int nextAttempt = state.attempt + 1;
    QTimer::singleShot(delay, this, [this, req, kind, nextAttempt]() {
        sendRequest(req, kind, nextAttempt);
    });
}

void NetworkHandler::querySettings() {
    QDEBUG("[NetworkHandler::querySettings()] called");
    sendRequest(QNetworkRequest(QUrl(settings->serverUrl + "/clientConnect.php?request=settings")), ReqSettings, 0);
}

void NetworkHandler::queryPlayerStats()
{
    QDEBUG("[NetworkHandler::queryPlayerStats()] called");

    QUrl url(settings->serverUrl + "/clientConnect.php");

    QUrlQuery query;
    query.addQueryItem("request", "playerStats");
    query.addQueryItem("player", settings->player);
    url.setQuery(query.query());

    sendRequest(QNetworkRequest(url), ReqStats, 0);
}

void NetworkHandler::updateSession(Session *session)
{
    QDEBUG("[NetworkHandler::updateSession()] updating session for %s",qPrintable(settings->games[session->game]->completeName));

    QUrl url(settings->serverUrl + "/clientConnect.php");

    QUrlQuery query;

    query.addQueryItem("request", "updateSession");
    query.addQueryItem("player", settings->player);
    query.addQueryItem("gameid", QString::number(settings->games[session->game]->id));
    query.addQueryItem("apm", QString::number(session->apm));
    query.addQueryItem("apmsum", QString::number(session->apmSum));
    query.addQueryItem("updates", QString::number(session->updateCount));

    url.setQuery(query.query());

    sendRequest(QNetworkRequest(url), ReqData, 0);
}

void NetworkHandler::reportNoSession(int apm)
{
    QDEBUG("[NetworkHandler::reportNoSession()] called");

    QUrl url(settings->serverUrl + "/clientConnect.php");

    QUrlQuery query;

    query.addQueryItem("request", "reportNoSession");
    query.addQueryItem("player", settings->player);
    if (apm > 0)
        query.addQueryItem("apm", QString::number(apm));

    url.setQuery(query.query());

    sendRequest(QNetworkRequest(url), ReqData, 0);
}

void NetworkHandler::sendAPM(int *buffer, int size)
{
    QDEBUG("[NetworkHandler::sendAPM()] sending APM");

    QUrl url(settings->serverUrl + "/clientConnect.php");

    QUrlQuery query;

    query.addQueryItem("request", "updateAPM");
    query.addQueryItem("player", settings->player);
    QString apm = QString::number(buffer[0]);
    for (int i = 1; i < size; ++i) {
        apm += ',';
        apm += QString::number(buffer[i]);
    }
    query.addQueryItem("apm", apm);

    url.setQuery(query.query());

    sendRequest(QNetworkRequest(url), ReqData, 0);
}
