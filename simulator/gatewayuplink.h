#ifndef GATEWAYUPLINK_H
#define GATEWAYUPLINK_H

#include "sample.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class QTcpSocket;
class QTimer;

// 网关模式：每台设备一条连接，主动连到 LinkGate 的设备端口（默认 9100），
// 每秒发一次心跳，断线后按指数退避重连。与 DeviceServer 的"监听、等客户端来连"互为两种拓扑。
class GatewayUplink : public QObject
{
    Q_OBJECT

public:
    explicit GatewayUplink(QObject* parent = nullptr);
    ~GatewayUplink() override;

    bool isActive() const;
    int  connectedCount() const;

public slots:
    void start(const QString& host, quint16 port, const QList<int>& deviceIds);
    void stop();
    void sendSample(const Sample& sample);

signals:
    void connectedCountChanged(int count);
    void logMessage(const QString& text);

private:
    struct Link
    {
        QTcpSocket* socket         = nullptr;
        QTimer*     reconnectTimer = nullptr;
        int         backoffMs      = 0;
    };

    void connectLink(int deviceId);
    void scheduleReconnect(int deviceId);
    void sendHeartbeats();
    void updateConnectedCount();

    QHash<int, Link> m_links;
    QTimer*          m_heartbeatTimer;
    QString          m_host;
    quint16          m_port      = 0;
    bool             m_active    = false;
    int              m_connected = 0;
};

#endif // GATEWAYUPLINK_H
