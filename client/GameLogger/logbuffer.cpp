#include "logbuffer.h"

LogBuffer *LogBuffer::instance()
{
    static LogBuffer inst;
    return &inst;
}

LogBuffer::LogBuffer(QObject *parent) : QObject(parent)
{
}

void LogBuffer::append(const QString &line)
{
    {
        QMutexLocker locker(&mutex);
        buffer.append(line);
        while (buffer.size() > MAX_LINES)
            buffer.removeFirst();
    }
    emit updated();
}

QStringList LogBuffer::lines() const
{
    QMutexLocker locker(&mutex);
    return buffer;
}
