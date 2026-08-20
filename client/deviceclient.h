#ifndef DEVICECLIENT_H
#define DEVICECLIENT_H

#include "sample.h"

#include <QByteArray>
#include <QObject>

class QTcpSocket;

class DeviceClient : public QObject
{
    Q_OBJECT
public:
    explicit DeviceClient(QObject *parent = nullptr);
    bool isConnected() const;

public slots:
    void connectToServer(const QString& host,quint16 port);
    void disconnectFromServer();

signals:
    void connectedChanged(bool connected);
    void sampleReceived(const Sample& sample);
    void logMessage(const QString& text);

private:
    void onReadyRead();
    void handleLine(const QByteArray& line);

    QTcpSocket* m_socket;
    QByteArray  m_buffer;
};

#endif // DEVICECLIENT_H
