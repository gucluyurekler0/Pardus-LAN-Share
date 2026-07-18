#include "device_discovery.h"
#include <QNetworkInterface>
#include <QDebug>
#include <QUuid>

// Her cihaz için sabit benzersiz ID
static const QString DEVICE_ID = QUuid::createUuid().toString(QUuid::WithoutBraces);

DeviceDiscovery::DeviceDiscovery(QObject *parent) : QObject(parent) {
    udpSocket = new QUdpSocket(this);
    broadcastTimer = new QTimer(this);
    timeoutTimer = new QTimer(this);

    connect(broadcastTimer, &QTimer::timeout, this, &DeviceDiscovery::broadcastPresence);
    connect(timeoutTimer, &QTimer::timeout, this, &DeviceDiscovery::checkDeviceTimeouts);
    connect(udpSocket, &QUdpSocket::readyRead, this, &DeviceDiscovery::readPendingDatagrams);

    qDebug() << "Bu cihazin ID'si:" << DEVICE_ID.left(8) << "...";
}

void DeviceDiscovery::startDiscovery() {
    if (!udpSocket->bind(BROADCAST_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qDebug() << "UDP soketi baslatilamadi!";
        return;
    }

    qDebug() << "Cihaz kesfi basladi. Port:" << BROADCAST_PORT;

    broadcastPresence();
    broadcastTimer->start(BROADCAST_INTERVAL);
    timeoutTimer->start(5000);
}

void DeviceDiscovery::stopDiscovery() {
    broadcastTimer->stop();
    timeoutTimer->stop();
    udpSocket->close();
    qDebug() << "Cihaz kesfi durduruldu.";
}

void DeviceDiscovery::broadcastPresence() {
    QJsonObject message = createDiscoveryMessage();
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    udpSocket->writeDatagram(data, QHostAddress::Broadcast, BROADCAST_PORT);
}

void DeviceDiscovery::readPendingDatagrams() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        processDatagram(datagram, sender);
    }
}

void DeviceDiscovery::processDatagram(const QByteArray &data, const QHostAddress &sender) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();

    // === KENDİNİ BULMA FİLTRESİ ===
    QString incomingDeviceId = obj["device_id"].toString();
    if (incomingDeviceId == DEVICE_ID) {
        return; // Kendi yayınımızı ignore et
    }

    QString messageType = obj["message"].toString();
    if (messageType != "DISCOVER" && messageType != "HELLO") return;

    DiscoveredDevice device;
    device.name = obj["device_name"].toString();
    device.type = obj["device_type"].toString();
    device.ip = sender.toString();
    device.port = obj["port"].toInt();
    device.lastSeen = QDateTime::currentDateTime().toSecsSinceEpoch();

    // IPv6 mapped IPv4 adreslerini düzelt (::ffff:192.168.x.x → 192.168.x.x)
    if (device.ip.startsWith("::ffff:")) {
        device.ip = device.ip.mid(7);
    }

    updateDeviceList(device);
    emit deviceFound(device);

    qDebug() << "Cihaz bulundu:" << device.name << "-" << device.ip;
}

void DeviceDiscovery::updateDeviceList(const DiscoveredDevice &device) {
    for (int i = 0; i < discoveredDevices.size(); ++i) {
        if (discoveredDevices[i].ip == device.ip) {
            discoveredDevices[i] = device;
            return;
        }
    }
    discoveredDevices.append(device);
}

void DeviceDiscovery::checkDeviceTimeouts() {
    qint64 now = QDateTime::currentDateTime().toSecsSinceEpoch();

    for (int i = discoveredDevices.size() - 1; i >= 0; --i) {
        if ((now - discoveredDevices[i].lastSeen) > DEVICE_TIMEOUT / 1000) {
            QString lostIp = discoveredDevices[i].ip;
            qDebug() << "Cihaz kayboldu:" << discoveredDevices[i].name;
            emit deviceLost(lostIp);
            discoveredDevices.removeAt(i);
        }
    }
}

QList<DiscoveredDevice> DeviceDiscovery::getDiscoveredDevices() const {
    return discoveredDevices;
}

QJsonObject DeviceDiscovery::createDiscoveryMessage() const {
    QJsonObject obj;
    obj["device_name"] = "Pardus-PC";
    obj["device_type"] = "desktop";
    obj["ip_address"] = "192.168.1.141";  // Wi-Fi IP'n
    obj["port"] = 9999;
    obj["timestamp"] = QDateTime::currentDateTime().toSecsSinceEpoch();
    obj["message"] = "DISCOVER";
    obj["device_id"] = DEVICE_ID;
    return obj;
}
QString DeviceDiscovery::getLocalIpAddress() const {
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &iface : interfaces) {
        // Sadece aktif ve çalışan arayüzleri kontrol et
        if (iface.flags() & QNetworkInterface::IsUp &&
            iface.flags() & QNetworkInterface::IsRunning &&
            !(iface.flags() & QNetworkInterface::IsLoopBack)) {

            QString name = iface.humanReadableName().toLower();

            // Sanal adaptörleri atla
            if (name.contains("virtual") ||
                name.contains("vmware") ||
                name.contains("vmnet") ||
                name.contains("vbox") ||
                name.contains("docker") ||
                name.contains("hyper-v") ||
                name.contains("tailscale") ||
                name.contains("bluetooth")) {
                continue;
            }

            // Sadece Wi-Fi ve Ethernet adaptörlerini kontrol et
            if (!name.contains("wi-fi") &&
                !name.contains("wireless") &&
                !name.contains("ethernet") &&
                !name.contains("wlan")) {
                continue;
            }

            QList<QNetworkAddressEntry> entries = iface.addressEntries();
            for (const QNetworkAddressEntry &entry : entries) {
                QHostAddress addr = entry.ip();

                if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
                    !addr.isLoopback()) {

                    QString ipStr = addr.toString();

                    // Sadece 192.168.1.x ağını kullan (senin Wi-Fi ağın)
                    if (ipStr.startsWith("192.168.1.")) {
                        qDebug() << "Wi-Fi IP bulundu:" << ipStr << "Arayuz:" << name;
                        return ipStr;
                    }
                }
            }
        }
    }

    // Hiçbiri bulunamazsa manuel fallback
    qDebug() << "UYARI: Wi-Fi IP bulunamadi, manuel IP kullaniliyor!";
    return "192.168.1.141"; // Senin gerçek IP'n
}