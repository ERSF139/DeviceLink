#ifndef SAMPLEDATABASE_H
#define SAMPLEDATABASE_H

#include "sample.h"

#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>

class QTimer;
class QSqlQuery;

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

    QList<Sample> selectHistory(int deviceId, qint64 fromMs, qint64 toMs,
                                int limit, qint64* totalMatched = nullptr) const;
    bool writeCsv(const QString& filePath, int deviceId, qint64 fromMs, qint64 toMs,
                  qint64* written = nullptr);

    static constexpr int kMaxQueryRows = 5000;

public slots:
    void initialize(const QString& filePath);
    void shutdown();
    void enqueue(const Sample& sample);
    bool flush();
    void queryHistory(int deviceId, qint64 fromMs, qint64 toMs);
    void exportCsv(const QString& filePath, int deviceId, qint64 fromMs, qint64 toMs);
    void listDeviceIds();

signals:
    void opened(qint64 totalRows);
    void logMessage(const QString& text);
    void flushed(int rowCount, qint64 elapsedMs, qint64 totalRows);
    void queryFinished(const QList<Sample>& rows, qint64 totalMatched);
    void exportFinished(bool ok, const QString& message);
    void deviceIdsReady(const QList<int>& ids);

private:
    bool applyPragmas();
    bool createSchema();
    bool prepareHistoryQuery(QSqlQuery& query, const QString& sqlHead, const QString& sqlTail,
                             int deviceId, qint64 fromMs, qint64 toMs) const;

    QSqlDatabase  m_db;
    QString       m_connectionName;
    QList<Sample> m_pending;
    QTimer*       m_flushTimer;
    QString       m_lastError;
};

#endif // SAMPLEDATABASE_H