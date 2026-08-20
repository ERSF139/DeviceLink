#include "deviceclient.h"

#include <QStringList>
#include <QTcpSocket>

DeviceClient::DeviceClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket,&QTcpSocket::readyRead,
            this,&DeviceClient::onReadyRead);

    connect(m_socket,&QTcpSocket::connected,this,[this](){
        m_buffer.clear();
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
    m_buffer.append(m_socket->readAll());      // 1. 新数据无条件追加到缓冲区

    int index = m_buffer.indexOf('\n');        // 2. 找第一个换行符
    while (index >= 0) {                       // 3. 只要找得到，就说明有完整的一行
        const QByteArray line = m_buffer.left(index);   // 取出这一行（不含 \n）
        m_buffer.remove(0, index + 1);                  // 从缓冲区删掉它和那个 \n
        handleLine(line);
        index = m_buffer.indexOf('\n');                 // 继续找下一个
    }
    // 循环结束时，缓冲区里剩下的是不完整的半行，原封不动留到下次
}

void DeviceClient::handleLine(const QByteArray& line)
{
    const QString text = QString::fromUtf8(line).trimmed();
    if(text.isEmpty())
        return;

    const QStringList fields = text.split(',');
    if(fields.size() != 5){
        emit logMessage(QString("丢弃字段数不对的数据:%1").arg(text));
        return;
    }

    bool ok0 = false, ok1 = false, ok2 = false, ok3 = false, ok4 = false;

    Sample sample;
    sample.deviceId    = fields.at(0).toInt(&ok0);
    sample.timestampMs = fields.at(1).toLongLong(&ok1);
    sample.temperature = fields.at(2).toDouble(&ok2);
    sample.pressure    = fields.at(3).toDouble(&ok3);
    sample.vibration   = fields.at(4).toDouble(&ok4);

    if (!(ok0 && ok1 && ok2 && ok3 && ok4)) {
        emit logMessage(QString("丢弃无法解析的数据：%1").arg(text));
        return;
    }

     emit sampleReceived(sample);
}