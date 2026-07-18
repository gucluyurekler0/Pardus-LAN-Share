#ifndef DEVICE_DISCOVERY_H
#define DEVICE_DISCOVERY_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

struct DiscoveredDevice {
    QString name;
    QString type;
    QString ip;
    int port;
    qint64 lastSeen;
};

class DeviceDiscovery : public QObject {
    Q_OBJECT
public:
    explicit DeviceDiscovery(QObject *parent = nullptr);
    void startDiscovery();
    void stopDiscovery();
    QList<DiscoveredDevice> getDiscoveredDevices() const;

signals:
    void deviceFound(const DiscoveredDevice &device);
    void deviceLost(const QString &ip);

private slots:
    void broadcastPresence();
    void readPendingDatagrams();
    void checkDeviceTimeouts();

private:
    QUdpSocket *udpSocket;
    QTimer *broadcastTimer;
    QTimer *timeoutTimer;
    QList<DiscoveredDevice> discoveredDevices;

    static const quint16 BROADCAST_PORT = 8888;
    static const int BROADCAST_INTERVAL = 3000;
    static const int DEVICE_TIMEOUT = 15000;

    void sendBroadcast();
    void processDatagram(const QByteArray &data, const QHostAddress &sender);
    void updateDeviceList(const DiscoveredDevice &device);
    QJsonObject createDiscoveryMessage() const;
    QString getLocalIpAddress() const;
};

#endif