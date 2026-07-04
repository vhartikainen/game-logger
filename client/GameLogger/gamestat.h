#ifndef GAMESTAT_H
#define GAMESTAT_H

#include <QMetaType>

// Aggregated play statistics for a single game, as returned by the server's
// "playerStats" request. Times are in seconds; lastPlayed is a Unix timestamp.
struct GameStat {
    int gameid;
    qint64 seconds;
    int sessions;
    qint64 lastPlayed;

    GameStat() : gameid(0), seconds(0), sessions(0), lastPlayed(0) {}
};

// Allows GameStat / QList<GameStat> to travel through signals & slots.
Q_DECLARE_METATYPE(GameStat)

#endif // GAMESTAT_H
