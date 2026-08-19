#ifndef DEVICE_H
#define DEVICE_H

#include "sample.h"

#include <QObject>
#include <QString>

class QTimer;

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(int id,const QString& name,QObject* parent = nullptr);

    int id()const;
    QString name()const;
    bool isRunning()const;

public slots:
    void start();
    void stop();

signals:
    void sampleGenerated(const Sample& sample);

private:
    void generateSample();

    int m_id;
    QString m_name;
    QTimer* m_timer;

    double m_temperature;
    double m_pressure;
    double m_vibration;
};

#endif // DEVICE_H
