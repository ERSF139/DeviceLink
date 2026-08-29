#ifndef SAMPLEDATABASE_H
#define SAMPLEDATABASE_H

#include "sample.h"

#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>

class QTimer;

class SampleDatabase : public QObject
{
    Q_OBJECT

public:
    explicit SampleDatabase(QObject* parent = nullptr);
    ~SampleDatabase() override;

    bool    open(const QString& filePath);
    void    close();
    bool    isOpen() const;
    QString lastError() const;

    qint64 rowCount() const;
    int    pendingCount() const;

public slots:
    void initialize(const QString& filePath);
    void shutdown();
    void enqueue(const Sample& sample);
    bool flush();

signals:
    void opened(qint64 totalRows);
    void logMessage(const QString& text);
    void flushed(int rowCount, qint64 elapsedMs, qint64 totalRows);

private:
    bool applyPragmas();
    bool createSchema();

    QSqlDatabase  m_db;
    QString       m_connectionName;
    QList<Sample> m_pending;
    QTimer*       m_flushTimer;
    QString       m_lastError;
};

#endif // SAMPLEDATABASE_H