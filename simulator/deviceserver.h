#ifndef DEVICESERVER_H
#define DEVICESERVER_H

#include "sample.h"

#include <QList>
#include <QObject>

class QTcpServer;
class QTcpSocket;

class DeviceServer : public QObject
{
    Q_OBJECT

public:
    explicit DeviceServer(QObject* parent = nullptr);

    bool isListening() const;
    int  clientCount() const;

public slots:
    bool startListening(quint16 port);
    void stopListening();
    void broadcastSample(const Sample& sample);

signals:
    void clientCountChanged(int count);
    void logMessage(const QString& text);

private:
    void onNewConnection();

    QTcpServer*        m_server;
    QList<QTcpSocket*> m_clients;
};

#endif // DEVICESERVER_H