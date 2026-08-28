#ifndef DEVICECLIENT_H
#define DEVICECLIENT_H

#include "frameparser.h"
#include "protocol.h"
#include "sample.h"

#include <QAbstractSocket>
#include <QObject>
#include <QString>

class QTcpSocket;
class QTimer;

class DeviceClient : public QObject
{
    Q_OBJECT

public:
    explicit DeviceClient(QObject* parent = nullptr);

    bool    isConnected() const;
    bool    isActive() const;
    int     pendingReconnectMs() const;
    QString host() const;
    quint16 port() const;

public slots:
    void connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();

signals:
    void connectedChanged(bool connected);
    void reconnectScheduled(int delayMs);
    void sampleReceived(const Sample& sample);
    void logMessage(const QString& text);

private:
    void onReadyRead();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onWatchdogTimeout();
    void onReconnectTimeout();

    void handleFrame(const Protocol::Frame& frame);
    void armWatchdog();
    void stopWatchdog();
    void tryConnectNow();
    void scheduleReconnect();

    QTcpSocket* m_socket;
    QTimer*     m_watchdog;
    QTimer*     m_reconnectTimer;
    FrameParser m_parser;

    QString m_host;
    quint16 m_port          = 0;
    bool    m_wantConnected      = false;
    bool    m_suppressReconnect  = false;
    int     m_backoffMs          = 0;
};

#endif // DEVICECLIENT_H
