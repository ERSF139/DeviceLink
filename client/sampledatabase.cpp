#include "sampledatabase.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringConverter>
#include <QTextStream>
#include <QTimer>
#include <QVariant>

namespace {

constexpr int kFlushIntervalMs = 2000;   // 最长 2 秒落盘一次
constexpr int kFlushBatchSize  = 200;    // 攒够 200 条立刻落盘

int nextConnectionId()
{
    static int counter = 0;
    return ++counter;
}

} // namespace

SampleDatabase::SampleDatabase(QObject* parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("devicelink_%1").arg(nextConnectionId()))
    , m_flushTimer(new QTimer(this))
{
    m_flushTimer->setInterval(kFlushIntervalMs);
    connect(m_flushTimer, &QTimer::timeout, this, [this]() { flush(); });
}

SampleDatabase::~SampleDatabase()
{
    close();
}

bool SampleDatabase::isOpen() const
{
    return m_db.isOpen();
}

QString SampleDatabase::lastError() const
{
    return m_lastError;
}

int SampleDatabase::pendingCount() const
{
    return static_cast<int>(m_pending.size());
}

bool SampleDatabase::open(const QString& filePath)
{
    close();

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(filePath);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        emit logMessage(QString("打开数据库失败：%1").arg(m_lastError));
        return false;
    }

    if (!applyPragmas() || !createSchema()) {
        close();
        return false;
    }

    m_flushTimer->start();
    emit opened(rowCount());
    listDeviceIds();
    emit logMessage(QString("数据库已打开：%1")
                        .arg(QFileInfo(filePath).absoluteFilePath()));
    return true;
}

void SampleDatabase::initialize(const QString& filePath)
{
    open(filePath);
}

void SampleDatabase::shutdown()
{
    flush();
    close();
}

void SampleDatabase::close()
{
    if (m_db.isOpen()) {
        m_flushTimer->stop();
        flush();
        m_db.close();
        emit logMessage(QStringLiteral("数据库已关闭"));
    }

    m_db = QSqlDatabase();                              // 先让成员失效
    if (QSqlDatabase::contains(m_connectionName))       // 再移除连接
        QSqlDatabase::removeDatabase(m_connectionName);
}

bool SampleDatabase::applyPragmas()
{
    QSqlQuery query(m_db);

    // WAL：读写不互相阻塞，写入吞吐显著高于默认的 rollback journal
    if (!query.exec(QStringLiteral("PRAGMA journal_mode = WAL"))) {
        m_lastError = query.lastError().text();
        emit logMessage(QString("设置 WAL 失败：%1").arg(m_lastError));
        return false;
    }

    // NORMAL：不再每次提交都 fsync，崩溃一致性由 WAL 保证
    if (!query.exec(QStringLiteral("PRAGMA synchronous = NORMAL"))) {
        m_lastError = query.lastError().text();
        emit logMessage(QString("设置 synchronous 失败：%1").arg(m_lastError));
        return false;
    }

    return true;
}

bool SampleDatabase::createSchema()
{
    QSqlQuery query(m_db);

    const QString createTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS samples ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  device_id   INTEGER NOT NULL,"
        "  ts_ms       INTEGER NOT NULL,"
        "  temperature REAL    NOT NULL,"
        "  pressure    REAL    NOT NULL,"
        "  vibration   REAL    NOT NULL)");

    if (!query.exec(createTable)) {
        m_lastError = query.lastError().text();
        emit logMessage(QString("建表失败：%1").arg(m_lastError));
        return false;
    }

    // 复合索引，列顺序对应"按设备 + 时间范围"的典型查询
    const QString createIndex = QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_samples_device_ts "
        "ON samples(device_id, ts_ms)");

    if (!query.exec(createIndex)) {
        m_lastError = query.lastError().text();
        emit logMessage(QString("建索引失败：%1").arg(m_lastError));
        return false;
    }

    const QString createTsIndex = QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_samples_ts ON samples(ts_ms)");
    if (!query.exec(createTsIndex)) {
        m_lastError = query.lastError().text();
        emit logMessage(QString("建时间索引失败：%1").arg(m_lastError));
        return false;
    }

    return true;
}

void SampleDatabase::enqueue(const Sample& sample)
{
    if (!m_db.isOpen())
        return;

    m_pending.append(sample);

    if (m_pending.size() >= kFlushBatchSize)
        flush();
}

bool SampleDatabase::flush()
{
    if (!m_db.isOpen() || m_pending.isEmpty())
        return true;

    QElapsedTimer timer;
    timer.start();

    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        emit logMessage(QString("开启事务失败：%1").arg(m_lastError));
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO samples (device_id, ts_ms, temperature, pressure, vibration) "
        "VALUES (?, ?, ?, ?, ?)"));

    for (const Sample& sample : m_pending) {
        query.addBindValue(sample.deviceId);
        query.addBindValue(sample.timestampMs);
        query.addBindValue(sample.temperature);
        query.addBindValue(sample.pressure);
        query.addBindValue(sample.vibration);

        if (!query.exec()) {
            m_lastError = query.lastError().text();
            emit logMessage(QString("写入失败，整批回滚：%1").arg(m_lastError));
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        emit logMessage(QString("提交事务失败：%1").arg(m_lastError));
        m_db.rollback();
        return false;
    }

    const int written = static_cast<int>(m_pending.size());
    m_pending.clear();

    emit flushed(written, timer.elapsed(), rowCount());
    return true;
}

qint64 SampleDatabase::rowCount() const
{
    if (!m_db.isOpen())
        return 0;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM samples")) || !query.next())
        return 0;

    return query.value(0).toLongLong();
}

bool SampleDatabase::prepareHistoryQuery(QSqlQuery& query, const QString& sqlHead,
                                         const QString& sqlTail, int deviceId,
                                         qint64 fromMs, qint64 toMs) const
{
    QString sql = sqlHead;
    sql += QStringLiteral(" WHERE ts_ms >= ? AND ts_ms <= ?");
    if (deviceId > 0)
        sql += QStringLiteral(" AND device_id = ?");
    sql += sqlTail;

    if (!query.prepare(sql)) {
        return false;
    }

    query.addBindValue(fromMs);
    query.addBindValue(toMs);
    if (deviceId > 0)
        query.addBindValue(deviceId);
    return true;
}

QList<Sample> SampleDatabase::selectHistory(int deviceId, qint64 fromMs, qint64 toMs,
                                            int limit, qint64* totalMatched) const
{
    if (totalMatched)
        *totalMatched = 0;

    QList<Sample> rows;
    if (!m_db.isOpen())
        return rows;

    qint64 from = fromMs;
    qint64 to   = toMs;
    if (from > to)
        qSwap(from, to);

    QSqlQuery countQuery(m_db);
    if (!prepareHistoryQuery(countQuery,
                             QStringLiteral("SELECT COUNT(*) FROM samples"),
                             QString(), deviceId, from, to)
        || !countQuery.exec() || !countQuery.next()) {
        return rows;
    }

    const qint64 total = countQuery.value(0).toLongLong();
    if (totalMatched)
        *totalMatched = total;

    QSqlQuery query(m_db);
    QString tail = QStringLiteral(" ORDER BY ts_ms ASC, device_id ASC");
    if (limit > 0)
        tail += QStringLiteral(" LIMIT ?");

    if (!prepareHistoryQuery(query,
                             QStringLiteral(
                                 "SELECT device_id, ts_ms, temperature, pressure, vibration "
                                 "FROM samples"),
                             tail, deviceId, from, to)) {
        return rows;
    }
    if (limit > 0)
        query.addBindValue(limit);

    if (!query.exec())
        return rows;

    while (query.next()) {
        Sample sample;
        sample.deviceId    = query.value(0).toInt();
        sample.timestampMs = query.value(1).toLongLong();
        sample.temperature = query.value(2).toDouble();
        sample.pressure    = query.value(3).toDouble();
        sample.vibration   = query.value(4).toDouble();
        rows.append(sample);
    }
    return rows;
}

bool SampleDatabase::writeCsv(const QString& filePath, int deviceId, qint64 fromMs, qint64 toMs,
                              qint64* written)
{
    if (written)
        *written = 0;

    if (!m_db.isOpen()) {
        m_lastError = QStringLiteral("数据库未打开");
        return false;
    }

    qint64 from = fromMs;
    qint64 to   = toMs;
    if (from > to)
        qSwap(from, to);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = file.errorString();
        return false;
    }

    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << QStringLiteral("device_id,ts_ms,time,temperature,pressure,vibration\n");

    QSqlQuery query(m_db);
    if (!prepareHistoryQuery(query,
                             QStringLiteral(
                                 "SELECT device_id, ts_ms, temperature, pressure, vibration "
                                 "FROM samples"),
                             QStringLiteral(" ORDER BY ts_ms ASC, device_id ASC"),
                             deviceId, from, to)
        || !query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    qint64 count = 0;
    while (query.next()) {
        const qint64 ts = query.value(1).toLongLong();
        const QString time =
            QDateTime::fromMSecsSinceEpoch(ts).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        out << query.value(0).toInt() << ','
            << ts << ','
            << time << ','
            << QString::number(query.value(2).toDouble(), 'f', 3) << ','
            << QString::number(query.value(3).toDouble(), 'f', 3) << ','
            << QString::number(query.value(4).toDouble(), 'f', 4) << '\n';
        ++count;
    }

    out.flush();
    if (written)
        *written = count;
    return true;
}

void SampleDatabase::queryHistory(int deviceId, qint64 fromMs, qint64 toMs)
{
    flush();
    qint64 total = 0;
    const QList<Sample> rows =
        selectHistory(deviceId, fromMs, toMs, kMaxQueryRows, &total);
    emit queryFinished(rows, total);
    listDeviceIds();
}

void SampleDatabase::exportCsv(const QString& filePath, int deviceId, qint64 fromMs, qint64 toMs)
{
    flush();
    qint64 written = 0;
    if (!writeCsv(filePath, deviceId, fromMs, toMs, &written)) {
        emit logMessage(QString("导出 CSV 失败：%1").arg(m_lastError));
        emit exportFinished(false, m_lastError);
        return;
    }

    const QString message = QStringLiteral("已导出 %1 条到 %2").arg(written).arg(filePath);
    emit logMessage(message);
    emit exportFinished(true, message);
}

void SampleDatabase::listDeviceIds()
{
    QList<int> ids;
    if (!m_db.isOpen()) {
        emit deviceIdsReady(ids);
        return;
    }

    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral("SELECT DISTINCT device_id FROM samples ORDER BY device_id"))) {
        while (query.next())
            ids.append(query.value(0).toInt());
    }
    emit deviceIdsReady(ids);
}
