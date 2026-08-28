#include "deviceclient.h"

#include "reconnect.h"

#include <QTcpSocket>
#include <QTimer>

DeviceClient::DeviceClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_watchdog(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_backoffMs(Reconnect::kInitialDelayMs)
{
    m_watchdog->setSingleShot(true);
    m_watchdog->setInterval(Reconnect::kWatchdogTimeoutMs);
    m_reconnectTimer->setSingleShot(true);

    connect(m_socket, &QTcpSocket::readyRead, this, &DeviceClient::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &DeviceClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &DeviceClient::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &DeviceClient::onSocketError);
    connect(m_watchdog, &QTimer::timeout, this, &DeviceClient::onWatchdogTimeout);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DeviceClient::onReconnectTimeout);
}

bool DeviceClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool DeviceClient::isActive() const
{
    return m_wantConnected;
}

int DeviceClient::pendingReconnectMs() const
{
    return m_reconnectTimer->isActive() ? static_cast<int>(m_reconnectTimer->remainingTime()) : 0;
}

QString DeviceClient::host() const
{
    return m_host;
}

quint16 DeviceClient::port() const
{
    return m_port;
}

void DeviceClient::connectToServer(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_wantConnected = true;
    m_backoffMs = Reconnect::kInitialDelayMs;
    m_reconnectTimer->stop();

    m_suppressReconnect = true;
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->abort();
    m_suppressReconnect = false;

    tryConnectNow();
}

void DeviceClient::disconnectFromServer()
{
    m_wantConnected = false;
    m_reconnectTimer->stop();
    stopWatchdog();

    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->abort();
    else
        emit connectedChanged(false);
}

void DeviceClient::tryConnectNow()
{
    if (!m_wantConnected)
        return;

    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        return;

    emit logMessage(QString("正在连接 %1:%2 ...").arg(m_host).arg(m_port));
    m_socket->connectToHost(m_host, m_port);
}

void DeviceClient::scheduleReconnect()
{
    if (!m_wantConnected)
        return;

    if (m_reconnectTimer->isActive())
        return;

    emit logMessage(QString("将在 %1 秒后自动重连").arg(m_backoffMs / 1000));
    m_reconnectTimer->start(m_backoffMs);
    emit reconnectScheduled(m_backoffMs);
    m_backoffMs = Reconnect::nextDelayMs(m_backoffMs);
}

void DeviceClient::onSocketConnected()
{
    m_parser.reset();
    m_backoffMs = Reconnect::kInitialDelayMs;
    m_reconnectTimer->stop();
    armWatchdog();
    emit logMessage("已连接到服务器");
    emit connectedChanged(true);
}

void DeviceClient::onSocketDisconnected()
{
    stopWatchdog();
    m_parser.reset();
    emit connectedChanged(false);

    if (m_wantConnected && !m_suppressReconnect)
        scheduleReconnect();
}

void DeviceClient::onSocketError(QAbstractSocket::SocketError)
{
    if (!m_wantConnected || m_suppressReconnect)
        return;

    emit logMessage(QString("网络错误：%1").arg(m_socket->errorString()));

    if (m_socket->state() == QAbstractSocket::UnconnectedState)
        scheduleReconnect();
}

void DeviceClient::onWatchdogTimeout()
{
    if (!m_wantConnected || !isConnected())
        return;

    emit logMessage("心跳超时，主动断开并重连");
    m_socket->abort();
}

void DeviceClient::onReconnectTimeout()
{
    tryConnectNow();
}

void DeviceClient::armWatchdog()
{
    m_watchdog->start();
}

void DeviceClient::stopWatchdog()
{
    m_watchdog->stop();
}

void DeviceClient::onReadyRead()
{
    const int droppedBefore = m_parser.droppedBytes();

    m_parser.append(m_socket->readAll());

    while (const auto frame = m_parser.nextFrame())
        handleFrame(*frame);

    const int dropped = m_parser.droppedBytes() - droppedBefore;
    if (dropped > 0)
        emit logMessage(QString("丢弃 %1 个无法识别的字节").arg(dropped));
}

void DeviceClient::handleFrame(const Protocol::Frame& frame)
{
    armWatchdog();

    switch (frame.type) {
    case Protocol::MessageType::Sample: {
        const auto sample = Protocol::decodeSample(frame.payload);
        if (!sample) {
            emit logMessage("采样帧载荷长度异常，已丢弃");
            return;
        }
        emit sampleReceived(*sample);
        break;
    }

    case Protocol::MessageType::Heartbeat:
        break;

    default:
        emit logMessage(QString("忽略未知消息类型 0x%1")
                            .arg(static_cast<quint8>(frame.type), 2, 16, QChar('0')));
        break;
    }
}
