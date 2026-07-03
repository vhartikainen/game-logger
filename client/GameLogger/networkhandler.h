#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include <QNetworkAccessManager>
#include <QtNetwork>
#include <QHash>

#include "settings.h"
#include "session.h"

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

signals:
    // Terminal failure: the request was retried and finally given up on.
    void error(QString error);
    // Non-terminal, informational (e.g. "retrying in 4s", "reconnected").
    void status(QString status);

private slots:
    void replyFinished();

private:
    // Everything needed to reissue a request if it fails.
    struct RequestState {
        QNetworkRequest request;
        int attempt;        // 0-based attempt counter
        bool isSettings;    // settings need parsing on success + infinite retry
    };

    void sendRequest(const QNetworkRequest &req, bool isSettings, int attempt);
    void handleFailure(const QString &errStr, const RequestState &state);
    int retryDelay(int attempt) const;

    Settings *settings;

    QNetworkAccessManager qnam;
    QHash<QNetworkReply *, RequestState> pending;

    // True once at least one request succeeded, so we can announce recovery.
    bool wasFailing;
};

#endif // NETWORKHANDLER_H
