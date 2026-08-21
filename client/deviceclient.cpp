#include "deviceclient.h"

#include <QTcpSocket>

DeviceClient::DeviceClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket,&QTcpSocket::readyRead,
            this,&DeviceClient::onReadyRead);

    connect(m_socket,&QTcpSocket::connected,this,[this](){
        m_parser.reset();
        emit logMessage("已连接到服务器");
        emit connectedChanged(true);
    });

    connect(m_socket,&QTcpSocket::disconnected,this,[this](){
        emit logMessage("连接已断开");
        emit connectedChanged(false);
    });

    connect(m_socket,&QTcpSocket::errorOccurred,this,[this](QAbstractSocket::SocketError){
        emit logMessage(QString("网络错误：%1").arg(m_socket->errorString()));
    });
}

bool DeviceClient::isConnected()const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void DeviceClient::connectToServer(const QString& host,quint16 port)
{
    if(m_socket->state()!=QAbstractSocket::UnconnectedState)
        return;
    emit logMessage(QString("正在连接 %1:%2").arg(host).arg(port));
    m_socket->connectToHost(host,port);
}

void DeviceClient::disconnectFromServer()
{
    m_socket->abort();
    emit connectedChanged(false);
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
        break;                                  // 第 9 步再处理

    default:
        emit logMessage(QString("忽略未知消息类型 0x%1")
                            .arg(static_cast<quint8>(frame.type), 2, 16, QChar('0')));
        break;
    }
}