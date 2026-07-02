#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <QObject>
#include <QStringList>
#include <QMutex>

// Rotating in-memory log, fed by the Qt message handler installed in main.cpp.
// Singleton so any translation unit can log without threading a reference through.
class LogBuffer : public QObject
{
    Q_OBJECT
public:
    static LogBuffer *instance();

    void append(const QString &line);
    QStringList lines() const;

signals:
    void updated();

private:
    explicit LogBuffer(QObject *parent = 0);

    mutable QMutex mutex;
    QStringList buffer;

    static const int MAX_LINES = 1000;
};

#endif // LOGBUFFER_H
