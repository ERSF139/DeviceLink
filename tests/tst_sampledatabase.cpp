#include "sample.h"
#include "sampledatabase.h"

#include <QElapsedTimer>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

namespace {

Sample makeSample(int deviceId, qint64 ts)
{
    Sample sample;
    sample.deviceId    = deviceId;
    sample.timestampMs = ts;
    sample.temperature = 25.0 + deviceId;
    sample.pressure    = 101.0 + deviceId;
    sample.vibration   = 0.5;
    return sample;
}

// 建一个裸连接，供性能对比用
QSqlDatabase openRaw(const QString& path, const QString& name)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    db.setDatabaseName(path);
    db.open();

    QSqlQuery query(db);
    query.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    query.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS samples ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  device_id INTEGER NOT NULL, ts_ms INTEGER NOT NULL,"
        "  temperature REAL NOT NULL, pressure REAL NOT NULL, vibration REAL NOT NULL)"));
    return db;
}

} // namespace

class TestSampleDatabase : public QObject
{
    Q_OBJECT

private slots:
    void open_createsSchema();
    void enqueueAndFlush_persistsRows();
    void flush_withoutData_succeeds();
    void flush_isTriggeredByBatchSize();
    void selectHistory_filtersByDeviceAndTime();
    void selectHistory_allDevicesAndLimit();
    void writeCsv_exportsFilteredUtf8();
    void benchmark_transactionVsPerRow();
};

void TestSampleDatabase::open_createsSchema()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SampleDatabase db;
    QVERIFY(db.open(dir.filePath(QStringLiteral("test.db"))));
    QVERIFY(db.isOpen());
    QCOMPARE(db.rowCount(), 0LL);
}

void TestSampleDatabase::enqueueAndFlush_persistsRows()
{
    QTemporaryDir dir;
    SampleDatabase db;
    QVERIFY(db.open(dir.filePath(QStringLiteral("test.db"))));

    for (int i = 0; i < 10; ++i)
        db.enqueue(makeSample(1, 1700000000000LL + i));

    QCOMPARE(db.pendingCount(), 10);
    QCOMPARE(db.rowCount(), 0LL);        // 还没落盘

    QVERIFY(db.flush());

    QCOMPARE(db.pendingCount(), 0);
    QCOMPARE(db.rowCount(), 10LL);
}

void TestSampleDatabase::flush_withoutData_succeeds()
{
    QTemporaryDir dir;
    SampleDatabase db;
    QVERIFY(db.open(dir.filePath(QStringLiteral("test.db"))));

    QVERIFY(db.flush());                 // 空队列 flush 不应报错
    QCOMPARE(db.rowCount(), 0LL);
}

void TestSampleDatabase::flush_isTriggeredByBatchSize()
{
    QTemporaryDir dir;
    SampleDatabase db;
    QVERIFY(db.open(dir.filePath(QStringLiteral("test.db"))));

    for (int i = 0; i < 250; ++i)        // 超过 200 的批量阈值
        db.enqueue(makeSample(1, 1700000000000LL + i));

    QVERIFY(db.rowCount() >= 200);       // 已经自动落盘过一次
    QVERIFY(db.pendingCount() < 200);
}

void TestSampleDatabase::selectHistory_filtersByDeviceAndTime()
{
    QTemporaryDir dir;
    SampleDatabase db;
    QVERIFY(db.open(dir.filePath(QStringLiteral("query.db"))));

    db.enqueue(makeSample(1, 100));
    db.enqueue(makeSample(1, 200));
    db.enqueue(makeSample(1, 300));
    db.enqueue(makeSample(2, 150));
    db.enqueue(makeSample(2, 250));
    QVERIFY(db.flush());

    qint64 total = 0;
    const QList<Sample> rows = db.selectHistory(1, 150, 300, 100, &total);
    QCOMPARE(total, 2LL);
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(0).timestampMs, 200LL);
    QCOMPARE(rows.at(1).timestampMs, 300LL);
    QCOMPARE(rows.at(0).deviceId, 1);
}

void TestSampleDatabase::selectHistory_allDevicesAndLimit()
{
    QTemporaryDir dir;
    SampleDatabase db;
    QVERIFY(db.open(dir.filePath(QStringLiteral("limit.db"))));

    for (int i = 0; i < 5; ++i)
        db.enqueue(makeSample(i % 2 + 1, 1000 + i));
    QVERIFY(db.flush());

    qint64 total = 0;
    const QList<Sample> rows = db.selectHistory(0, 0, 10000, 3, &total);
    QCOMPARE(total, 5LL);
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows.at(0).timestampMs, 1000LL);
}

void TestSampleDatabase::writeCsv_exportsFilteredUtf8()
{
    QTemporaryDir dir;
    SampleDatabase db;
    QVERIFY(db.open(dir.filePath(QStringLiteral("csv.db"))));

    db.enqueue(makeSample(1, 1700000000000LL));
    db.enqueue(makeSample(2, 1700000001000LL));
    QVERIFY(db.flush());

    const QString csvPath = dir.filePath(QStringLiteral("out.csv"));
    qint64 written = 0;
    QVERIFY(db.writeCsv(csvPath, 2, 0, 1800000000000LL, &written));
    QCOMPARE(written, 1LL);

    QFile file(csvPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray bytes = file.readAll();
    QVERIFY(bytes.startsWith("\xEF\xBB\xBF"));
    QVERIFY(bytes.contains("device_id,ts_ms,time,temperature,pressure,vibration"));
    QVERIFY(bytes.contains("2,1700000001000"));
    QVERIFY(!bytes.contains("1,1700000000000"));
}

void TestSampleDatabase::benchmark_transactionVsPerRow()
{
    constexpr int kRows = 2000;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    qint64 perRowMs = 0;
    qint64 batchMs  = 0;

    // ---- 方式一：逐条自动提交 ----
    {
        QSqlDatabase db = openRaw(dir.filePath(QStringLiteral("a.db")),
                                  QStringLiteral("bench_perrow"));
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT INTO samples (device_id, ts_ms, temperature, pressure, vibration) "
            "VALUES (?, ?, ?, ?, ?)"));

        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < kRows; ++i) {
            query.addBindValue(1);
            query.addBindValue(1700000000000LL + i);
            query.addBindValue(25.0);
            query.addBindValue(101.0);
            query.addBindValue(0.5);
            QVERIFY(query.exec());
        }
        perRowMs = timer.elapsed();

        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("bench_perrow"));

    // ---- 方式二：单事务批量提交 ----
    {
        QSqlDatabase db = openRaw(dir.filePath(QStringLiteral("b.db")),
                                  QStringLiteral("bench_batch"));
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT INTO samples (device_id, ts_ms, temperature, pressure, vibration) "
            "VALUES (?, ?, ?, ?, ?)"));

        QElapsedTimer timer;
        timer.start();
        QVERIFY(db.transaction());
        for (int i = 0; i < kRows; ++i) {
            query.addBindValue(1);
            query.addBindValue(1700000000000LL + i);
            query.addBindValue(25.0);
            query.addBindValue(101.0);
            query.addBindValue(0.5);
            QVERIFY(query.exec());
        }
        QVERIFY(db.commit());
        batchMs = timer.elapsed();

        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("bench_batch"));

    const double perRowRate = perRowMs > 0 ? kRows * 1000.0 / perRowMs : 0.0;
    const double batchRate  = batchMs  > 0 ? kRows * 1000.0 / batchMs  : 0.0;

    qInfo("逐条提交 : %lld ms  (%.0f 条/秒)", perRowMs, perRowRate);
    qInfo("事务批量 : %lld ms  (%.0f 条/秒)", batchMs,  batchRate);
    qInfo("提升倍数 : %.1fx", perRowMs > 0 ? double(perRowMs) / double(qMax(batchMs, 1LL)) : 0.0);

    QVERIFY(batchMs <= perRowMs);        // 批量不应该更慢
}

QTEST_GUILESS_MAIN(TestSampleDatabase)

#include "tst_sampledatabase.moc"