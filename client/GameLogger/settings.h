#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QDateTime>

// Structure to contain all info for a single game
struct GameInfo {
    qint32 id;
    QString completeName;
    qint32 treeIndex;
    QString exeName;
    QString logoUrl;
};

class Settings: public QObject
{
    Q_OBJECT
public:
    Settings(QString serverUrl, QString player);

    void parseSettings(QString data);

    // Collection of games
    QHash<QString, GameInfo*> games;

    // General settings
    int serverTime;
    int apmBufferSize;
    bool supressUpdates;
    QString player;
    QString serverUrl;

    bool ready;
};

#endif // SETTINGS_H
