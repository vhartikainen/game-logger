#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include <QNetworkAccessManager>
#include <QtNetwork>
#include <QHash>

#include "settings.h"
#include "session.h"
#include "gamestat.h"

class NetworkHandler: public QObject
{
    Q_OBJECT
public:
    NetworkHandler(Settings *settings);

    void querySettings();

    // Game session related functions
    void updateSession(Session *session);
    void reportNoSession(int apm);

    // APM related functions
    void sendAPM(int *buffer, int size);

    // Per-game play statistics for the local player.
    void queryPlayerStats();

signals:
    // Terminal failure: the request was retried and finally given up on.
    void error(QString error);
    // Non-terminal, informational (e.g. "retrying in 4s", "reconnected").
    void status(QString status);
    // Emitted when a playerStats response was fetched and parsed.
    void statsReady(QList<GameStat> stats);

private slots:
    void replyFinished();

private:
    // How a request's response should be handled, and its retry policy.
    //   ReqSettings -- parsed into Settings; retried forever (client needs them).
    //   ReqData     -- fire-and-forget (session/APM); bounded retries then dropped.
    //   ReqStats    -- parsed & emitted; bounded retries (next poll supersedes).
    enum RequestKind { ReqSettings, ReqData, ReqStats };

    // Everything needed to reissue a request if it fails.
    struct RequestState {
        QNetworkRequest request;
        int attempt;        // 0-based attempt counter
        RequestKind kind;
    };

    void sendRequest(const QNetworkRequest &req, RequestKind kind, int attempt);
    void handleFailure(const QString &errStr, const RequestState &state);
    void parseStats(const QByteArray &data);
    int retryDelay(int attempt) const;

    Settings *settings;

    QNetworkAccessManager qnam;
    QHash<QNetworkReply *, RequestState> pending;

    // True once at least one request succeeded, so we can announce recovery.
    bool wasFailing;
};

#endif // NETWORKHANDLER_H
