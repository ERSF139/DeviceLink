#ifndef DEVICECLIENT_H
#define DEVICECLIENT_H

#include "frameparser.h"
#include "protocol.h"
#include "sample.h"

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
    void handleFrame(const Protocol::Frame& frame);

    QTcpSocket* m_socket;
    FrameParser m_parser;
};

#endif // DEVICECLIENT_H
