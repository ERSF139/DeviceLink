#include "deviceserver.h"
#include "protocol.h"

#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

DeviceServer::DeviceServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &DeviceServer::onNewConnection);
}

bool DeviceServer::isListening() const
{
    return m_server->isListening();
}

int DeviceServer::clientCount() const
{
    return m_clients.size();
}

bool DeviceServer::startListening(quint16 port)
{
    if (m_server->isListening())
        return true;

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit logMessage(QString("监听端口 %1 失败：%2")
                            .arg(port)
                            .arg(m_server->errorString()));
        return false;
    }

    emit logMessage(QString("开始监听端口 %1").arg(port));
    return true;
}

void DeviceServer::stopListening()
{
    m_server->close();

    const QList<QTcpSocket*> clients = m_clients;
    for (QTcpSocket* socket : clients)
        socket->disconnectFromHost();

    emit logMessage("已停止监听");
}

void DeviceServer::broadcastSample(const Sample& sample)
{
    if (m_clients.isEmpty())
        return;

    const QByteArray frame = Protocol::buildFrame(Protocol::MessageType::Sample,
                                                  Protocol::encodeSample(sample));

    for (QTcpSocket* socket : m_clients){
        socket->write(frame);
    }
}

void DeviceServer::onNewConnection()
{
    while (QTcpSocket* socket = m_server->nextPendingConnection()) {
        m_clients.append(socket);

        const QString peer = QString("%1:%2")
                                 .arg(socket->peerAddress().toString())
                                 .arg(socket->peerPort());

        connect(socket, &QTcpSocket::disconnected, this, [this, socket, peer]() {
            m_clients.removeAll(socket);
            socket->deleteLater();
            emit logMessage(QString("客户端断开：%1").arg(peer));
            emit clientCountChanged(m_clients.size());
        });

        emit logMessage(QString("客户端接入：%1").arg(peer));
        emit clientCountChanged(m_clients.size());
    }
}