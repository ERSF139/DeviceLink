#include "gatewayuplink.h"

#include "protocol.h"
#include "reconnect.h"

#include <QTcpSocket>
#include <QTimer>

#include <utility>

namespace {

// 网关处理不过来时，不让模拟器自己的发送缓冲无限增长：积压超过这个量就丢弃新采样。
constexpr qint64 kMaxPendingBytes = 256 * 1024;

} // namespace

GatewayUplink::GatewayUplink(QObject* parent)
    : QObject(parent)
    , m_heartbeatTimer(new QTimer(this))
{
    m_heartbeatTimer->setInterval(Reconnect::kHeartbeatIntervalMs);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &GatewayUplink::sendHeartbeats);
}

GatewayUplink::~GatewayUplink()
{
    stop();
}

bool GatewayUplink::isActive() const
{
    return m_active;
}

int GatewayUplink::connectedCount() const
{
    return m_connected;
}

void GatewayUplink::start(const QString& host, quint16 port, const QList<int>& deviceIds)
{
    stop();
    m_host   = host;
    m_port   = port;
    m_active = true;

    for (const int deviceId : deviceIds) {
        Link link;
        link.socket         = new QTcpSocket(this);
        link.reconnectTimer = new QTimer(this);
        link.reconnectTimer->setSingleShot(true);
        link.backoffMs      = Reconnect::kInitialDelayMs;
        m_links.insert(deviceId, link);

        connect(link.socket, &QTcpSocket::connected, this, [this, deviceId]() {
            m_links[deviceId].backoffMs = Reconnect::kInitialDelayMs;
            updateConnectedCount();
        });
        connect(link.socket, &QTcpSocket::disconnected, this, [this, deviceId]() {
            updateConnectedCount();
            scheduleReconnect(deviceId);
        });
        connect(link.socket, &QTcpSocket::errorOccurred, this, [this, deviceId](QAbstractSocket::SocketError) {
            const QTcpSocket* s = m_links.value(deviceId).socket;
            if (s != nullptr && s->state() == QAbstractSocket::UnconnectedState)
                scheduleReconnect(deviceId);
        });
        connect(link.reconnectTimer, &QTimer::timeout, this, [this, deviceId]() { connectLink(deviceId); });

        connectLink(deviceId);
    }

    m_heartbeatTimer->start();
    emit logMessage(QString("网关模式：%1 台设备正在连接 %2:%3").arg(deviceIds.size()).arg(host).arg(port));
}

void GatewayUplink::stop()
{
    if (!m_active)
        return;
    m_active = false;
    m_heartbeatTimer->stop();

    for (auto it = m_links.begin(); it != m_links.end(); ++it) {
        it->reconnectTimer->stop();
        it->socket->abort();
        it->socket->deleteLater();
        it->reconnectTimer->deleteLater();
    }
    m_links.clear();
    updateConnectedCount();
    emit logMessage("已断开网关");
}

void GatewayUplink::sendSample(const Sample& sample)
{
    const auto it = m_links.constFind(sample.deviceId);
    if (it == m_links.constEnd())
        return;

    QTcpSocket* socket = it->socket;
    if (socket->state() != QAbstractSocket::ConnectedState || socket->bytesToWrite() > kMaxPendingBytes)
        return;

    socket->write(Protocol::buildFrame(Protocol::MessageType::Sample, Protocol::encodeSample(sample)));
}

void GatewayUplink::connectLink(int deviceId)
{
    if (!m_active)
        return;
    QTcpSocket* socket = m_links.value(deviceId).socket;
    if (socket != nullptr && socket->state() == QAbstractSocket::UnconnectedState)
        socket->connectToHost(m_host, m_port);
}

void GatewayUplink::scheduleReconnect(int deviceId)
{
    if (!m_active || !m_links.contains(deviceId))
        return;
    Link& link = m_links[deviceId];
    if (link.reconnectTimer->isActive())
        return;
    link.reconnectTimer->start(link.backoffMs);
    link.backoffMs = Reconnect::nextDelayMs(link.backoffMs);
}

void GatewayUplink::sendHeartbeats()
{
    const QByteArray frame = Protocol::buildFrame(Protocol::MessageType::Heartbeat, QByteArray());
    for (const Link& link : std::as_const(m_links)) {
        if (link.socket->state() == QAbstractSocket::ConnectedState)
            link.socket->write(frame);
    }
}

void GatewayUplink::updateConnectedCount()
{
    int connected = 0;
    for (const Link& link : std::as_const(m_links)) {
        if (link.socket->state() == QAbstractSocket::ConnectedState)
            ++connected;
    }
    if (connected != m_connected) {
        m_connected = connected;
        emit connectedCountChanged(connected);
    }
}
