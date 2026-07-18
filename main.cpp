#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QStyle>
#include <QDebug>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QClipboard>
#include <QTime>
#include <QNetworkInterface>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QRandomGenerator>
#include "pardus_server.h"
#include "device_discovery.h"


QString getLocalWifiIp() {
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &iface : interfaces) {
        if ((iface.flags() & QNetworkInterface::IsUp) &&
            (iface.flags() & QNetworkInterface::IsRunning) &&
            !(iface.flags() & QNetworkInterface::IsLoopBack)) {

            QString name = iface.humanReadableName().toLower();

            if (!name.contains("virtual") && !name.contains("vbox") &&
                !name.contains("vmware") && !name.contains("vethernet") &&
                !name.contains("host-only")) {

                for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        QString ipStr = entry.ip().toString();

                        if (!ipStr.startsWith("192.168.56.") && !ipStr.startsWith("169.254.")) {
                            return ipStr; // Gerçek IP bulundu!
                        }
                    }
                }
            }
        }
    }

    // İkinci aşama: Eğer yukarıda çok katı filtreye takılıp bulamadıysa, standart bir IPv4 döndür
    for (const QNetworkInterface &iface : interfaces) {
        if (iface.flags() & QNetworkInterface::IsUp && !(iface.flags() & QNetworkInterface::IsLoopBack)) {
            for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    return entry.ip().toString();
                }
            }
        }
    }

    return "127.0.0.1";
}



int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    int randomPin = QRandomGenerator::global()->bounded(100000, 1000000);
    QString pinCode = QString::number(randomPin);


    QWidget mainWindow;
    mainWindow.setWindowTitle("Pardus LAN Share - Gelişmiş Sunucu Paneli");
    mainWindow.resize(550, 650); // Kod alanının sığması için dikey boyutu biraz artırdık
    mainWindow.setStyleSheet("background-color: #F8FAFC; font-family: 'Segoe UI', Arial, sans-serif;");

    QVBoxLayout *mainLayout = new QVBoxLayout(&mainWindow);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);


    QLabel *lblTitle = new QLabel("🖥️ Pardus LAN Share", &mainWindow);
    lblTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #1E293B;");
    mainLayout->addWidget(lblTitle);

    QString ipAddress = getLocalWifiIp();
    QLabel *lblIp = new QLabel("📌 Sunucu IP Adresi: <b>" + ipAddress + ":9999</b>", &mainWindow);
    lblIp->setStyleSheet("font-size: 13px; color: #475569; background-color: #E2E8F0; padding: 8px; border-radius: 6px;");
    mainLayout->addWidget(lblIp);


    QLabel *lblPin = new QLabel("🔑 Bağlantı Kodu (PIN): <span style='font-size: 16px; font-weight: bold; color: #EF4444;'>" + pinCode + "</span>", &mainWindow);
    lblPin->setStyleSheet("font-size: 13px; color: #475569; background-color: #FEE2E2; border: 1px solid #FCA5A5; padding: 8px; border-radius: 6px;");
    mainLayout->addWidget(lblPin);


    QLabel *lblStatus = new QLabel("⏳ Telefon bağlantısı bekleniyor...", &mainWindow);
    lblStatus->setStyleSheet("font-size: 12px; font-weight: bold; color: #64748B;");
    mainLayout->addWidget(lblStatus);


    QProgressBar *progressBar = new QProgressBar(&mainWindow);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #CBD5E1; border-radius: 6px; text-align: center; background-color: #FFFFFF; height: 22px; color: #1E293B; font-weight: bold; }"
        "QProgressBar::chunk { background-color: #3B82F6; border-radius: 5px; }"
        );
    mainLayout->addWidget(progressBar);


    QLabel *lblEditTitle = new QLabel("📝 Gelen Pano İçeriği (Düzenlenebilir):", &mainWindow);
    lblEditTitle->setStyleSheet("font-size: 12px; font-weight: bold; color: #475569; margin-top: 5px;");
    mainLayout->addWidget(lblEditTitle);

    QTextEdit *txtEditor = new QTextEdit(&mainWindow);
    txtEditor->setPlaceholderText("Telefondan gelen metinler burada görünecek, isterseniz üzerinde değişiklik yapabilirsiniz...");
    txtEditor->setStyleSheet(
        "QTextEdit { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 8px; "
        "font-size: 12px; color: #1E293B; font-family: 'Courier New', monospace; }"
        );
    mainLayout->addWidget(txtEditor);


    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnCopyToClipboard = new QPushButton("📋 Değişiklikleri Panoya Kopyala", &mainWindow);
    btnCopyToClipboard->setStyleSheet(
        "QPushButton { background-color: #10B981; color: white; border: none; padding: 8px 12px; font-weight: bold; border-radius: 6px; }"
        "QPushButton:hover { background-color: #059669; }"
        );
    QPushButton *btnClearEditor = new QPushButton("🗑️ Temizle", &mainWindow);
    btnClearEditor->setStyleSheet(
        "QPushButton { background-color: #EF4444; color: white; border: none; padding: 8px 12px; font-weight: bold; border-radius: 6px; }"
        "QPushButton:hover { background-color: #DC2626; }"
        );
    btnLayout->addWidget(btnCopyToClipboard);
    btnLayout->addWidget(btnClearEditor);
    mainLayout->addLayout(btnLayout);


    QLabel *lblLogTitle = new QLabel("📋 Sistem Günlükleri (En son yapılan en üsttedir. Dosyaya çift tıklayarak açabilirsiniz):", &mainWindow);
    lblLogTitle->setStyleSheet("font-size: 12px; font-weight: bold; color: #475569; margin-top: 5px;");
    mainLayout->addWidget(lblLogTitle);

    QListWidget *listLog = new QListWidget(&mainWindow);
    listLog->setMaximumHeight(100);
    listLog->setStyleSheet(
        "QListWidget { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 5px; font-size: 11px; color: #334155; }"
        "QListWidget::item:hover { background-color: #E2E8F0; border-radius: 4px; color: #1E293B; }"
        );
    listLog->insertItem(0, "[" + QTime::currentTime().toString() + "] Sunucu başarıyla başlatıldı. PIN: " + pinCode);
    mainLayout->addWidget(listLog);

    PardusServer server;


    server.setProperty("pinCode", pinCode);

    if (!server.startServer(9999)) {
        listLog->insertItem(0, "❌ HATA: TCP Sunucu başlatılamadı!");
        mainWindow.show();
        return app.exec();
    }

    DeviceDiscovery discovery;
    discovery.startDiscovery();


    QObject::connect(btnCopyToClipboard, &QPushButton::clicked, [&]() {
        QString editedText = txtEditor->toPlainText();
        QGuiApplication::clipboard()->setText(editedText);
        lblStatus->setText("✅ Düzenlenen metin bilgisayar panosuna güncellendi!");
        listLog->insertItem(0, "[" + QTime::currentTime().toString() + "] 📝 Pano manuel olarak güncellendi.");
    });

    QObject::connect(btnClearEditor, &QPushButton::clicked, [&]() {
        txtEditor->clear();
        lblStatus->setText("🗑️ Editör temizlendi.");
    });


    QObject::connect(listLog, &QListWidget::itemDoubleClicked, [&](QListWidgetItem *item) {
        QVariant data = item->data(Qt::UserRole);
        if (data.isValid()) {
            QString filePath = data.toString();
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    });


    QObject::connect(&discovery, &DeviceDiscovery::deviceFound, [&](const DiscoveredDevice &device) {
        listLog->insertItem(0, "[" + QTime::currentTime().toString() + "] 📱 Cihaz Algılandı: " + device.name + " (" + device.ip + ")");
    });

    QObject::connect(&server, &PardusServer::clipboardReceived, [&](const QString &text) {
        lblStatus->setText("📋 Yeni metin alındı!");
        txtEditor->setPlainText(text);
        listLog->insertItem(0, "[" + QTime::currentTime().toString() + "] 📝 Pano Alındı (Boyut: " + QString::number(text.length()) + " karakter)");
    });

    QObject::connect(&server, &PardusServer::fileTransferStarted, [&](const QString &filename, qint64 size) {
        progressBar->setValue(0);
        lblStatus->setText("📥 Dosya Alınıyor: " + filename);
        listLog->insertItem(0, "[" + QTime::currentTime().toString() + "] 📁 Dosya Alımı Başladı: " + filename);
    });

    QObject::connect(&server, &PardusServer::fileTransferProgress, [&](qint64 received, qint64 total) {
        int progress = (total > 0) ? static_cast<int>((received * 100) / total) : 0;
        progressBar->setValue(progress);
    });


    QObject::connect(&server, &PardusServer::fileTransferCompleted, [&](const QString &filepath) {
        progressBar->setValue(100);
        lblStatus->setText("✅ Dosya başarıyla kaydedildi!");

        QFileInfo fileInfo(filepath);
        QListWidgetItem *newItem = new QListWidgetItem(
            "[" + QTime::currentTime().toString() + "] 💾 Dosya Kaydedildi (Açmak için çift tıkla): " + fileInfo.fileName()
            );

        newItem->setData(Qt::UserRole, filepath);
        newItem->setForeground(QColor("#2563EB"));
        listLog->insertItem(0, newItem);
        listLog->setCurrentItem(newItem);
    });


    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(app.style()->standardIcon(QStyle::SP_ComputerIcon));
    trayIcon.setToolTip("Pardus LAN Share");

    QMenu trayMenu;
    QAction *showAction = trayMenu.addAction("Göster");
    QAction *quitAction = trayMenu.addAction("Çıkış");

    QObject::connect(showAction, &QAction::triggered, [&]() {
        mainWindow.show();
        mainWindow.raise();
        mainWindow.activateWindow();
    });

    QObject::connect(quitAction, &QAction::triggered, [&]() {
        discovery.stopDiscovery();
        app.quit();
    });

    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

    mainWindow.show();
    return app.exec();
}