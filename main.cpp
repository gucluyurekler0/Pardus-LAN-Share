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
#include <QPixmap>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFontDialog>
#include <QListWidgetItem>
#include <QScreen>
#include <QGuiApplication>

#include "pardus_server.h"
#include "device_discovery.h"

// Konsol loglarını tamamen gizlemek için mesaj yöneticisi
void quietMessageHandler(QtMsgType, const QMessageLogContext &, const QString &) {
    // Hiçbir şey yazdırma (Susturuldu)
}

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
                            return ipStr;
                        }
                    }
                }
            }
        }
    }

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
    // Konsol/Terminal çıktılarını tamamen kapatır
    qInstallMessageHandler(quietMessageHandler);

    QApplication app(argc, argv);

    int randomPin = QRandomGenerator::global()->bounded(100000, 1000000);
    QString pinCode = QString::number(randomPin);

    QWidget mainWindow;
    mainWindow.setWindowTitle("Pardus LAN Share - Gelişmiş Dosya Transferi ");
    mainWindow.resize(900, 580);
    mainWindow.setStyleSheet("background-color: #F8FAFC; font-family: 'Segoe UI', Arial, sans-serif;");

    QIcon appIcon(":/app_icon.png");    app.setWindowIcon(appIcon);
    mainWindow.setWindowIcon(appIcon);
    // Pencereyi ekranın sağ tarafına konumlandırma
    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    int x = screenGeometry.width() - mainWindow.width() - 20; // Sağ kenardan 20 piksel boşluk
    int y = (screenGeometry.height() - mainWindow.height()) / 2; // Dikeyde ortalanmış
    mainWindow.move(x, y);

    QHBoxLayout *rootLayout = new QHBoxLayout(&mainWindow);
    rootLayout->setSpacing(15);
    rootLayout->setContentsMargins(20, 20, 20, 20);

    // SOL PANEL
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(10);

    QLabel *lblTitle = new QLabel("🖥️ Pardus LAN Share", &mainWindow);
    lblTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #1E293B;");
    leftLayout->addWidget(lblTitle);

    QString ipAddress = getLocalWifiIp();
    QLabel *lblIp = new QLabel("📌 Sunucu IP Adresi: <b>" + ipAddress + ":9999</b>", &mainWindow);
    lblIp->setStyleSheet("font-size: 13px; color: #475569; background-color: #E2E8F0; padding: 8px; border-radius: 6px;");
    leftLayout->addWidget(lblIp);

    QLabel *lblPin = new QLabel("🔑 Bağlantı Kodu (PIN): <span style='font-size: 16px; font-weight: bold; color: #EF4444;'>" + pinCode + "</span>", &mainWindow);
    lblPin->setStyleSheet("font-size: 13px; color: #475569; background-color: #FEE2E2; border: 1px solid #FCA5A5; padding: 8px; border-radius: 6px;");
    leftLayout->addWidget(lblPin);

    QLabel *lblStatus = new QLabel("⏳ Gönderimler bekleniyor...", &mainWindow);
    lblStatus->setStyleSheet("font-size: 12px; font-weight: bold; color: #64748B;");
    leftLayout->addWidget(lblStatus);

    QProgressBar *progressBar = new QProgressBar(&mainWindow);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #CBD5E1; border-radius: 6px; text-align: center; background-color: #FFFFFF; height: 22px; color: #1E293B; font-weight: bold; }"
        "QProgressBar::chunk { background-color: #3B82F6; border-radius: 5px; }"
        );
    leftLayout->addWidget(progressBar);

    // Pano Başlığı ve Yazı Tipi Ayar Butonu
    QHBoxLayout *editHeaderLayout = new QHBoxLayout();
    QLabel *lblEditTitle = new QLabel("📝 Gelen Pano İçeriği:", &mainWindow);
    lblEditTitle->setStyleSheet("font-size: 12px; font-weight: bold; color: #475569;");

    QPushButton *btnChangeFont = new QPushButton("⚙️ Yazı Tipi", &mainWindow);
    btnChangeFont->setStyleSheet("QPushButton { background-color: #64748B; color: white; border: none; padding: 4px 10px; font-size: 11px; font-weight: bold; border-radius: 4px; } QPushButton:hover { background-color: #475569; }");

    editHeaderLayout->addWidget(lblEditTitle);
    editHeaderLayout->addStretch();
    editHeaderLayout->addWidget(btnChangeFont);
    leftLayout->addLayout(editHeaderLayout);

    QTextEdit *txtEditor = new QTextEdit(&mainWindow);
    txtEditor->setPlaceholderText("Telefondan gelen metinler burada görünecek...");
    txtEditor->setStyleSheet(
        "QTextEdit { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 8px; color: #1E293B; }"
        );
    leftLayout->addWidget(txtEditor);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnCopyToClipboard = new QPushButton("📋 Panoya Kopyala", &mainWindow);
    btnCopyToClipboard->setStyleSheet("QPushButton { background-color: #10B981; color: white; border: none; padding: 8px; font-weight: bold; border-radius: 6px; } QPushButton:hover { background-color: #059669; }");
    QPushButton *btnClearEditor = new QPushButton("🗑️ Temizle", &mainWindow);
    btnClearEditor->setStyleSheet("QPushButton { background-color: #EF4444; color: white; border: none; padding: 8px; font-weight: bold; border-radius: 6px; } QPushButton:hover { background-color: #DC2626; }");
    btnLayout->addWidget(btnCopyToClipboard);
    btnLayout->addWidget(btnClearEditor);
    leftLayout->addLayout(btnLayout);

    QListWidget *listLog = new QListWidget(&mainWindow);
    listLog->setMaximumHeight(90);
    listLog->setStyleSheet("QListWidget { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 11px; color: #334155; }");

    QListWidgetItem *initItem = new QListWidgetItem(app.style()->standardIcon(QStyle::SP_ComputerIcon), "[" + QTime::currentTime().toString() + "] Sunucu başlatıldı. PIN: " + pinCode);
    listLog->addItem(initItem);
    leftLayout->addWidget(listLog);

    rootLayout->addLayout(leftLayout, 65);

    // SAĞ PANEL (QR Kod)
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setAlignment(Qt::AlignCenter);
    rightLayout->setSpacing(10);

    QLabel *lblQrTitle = new QLabel(" Hızlı Bağlantı QR", &mainWindow);
    lblQrTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E293B;");
    lblQrTitle->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(lblQrTitle);

    QLabel *lblQrImage = new QLabel(&mainWindow);
    lblQrImage->setFixedSize(200, 200);
    lblQrImage->setStyleSheet("background-color: white; border: 2px solid #CBD5E1; border-radius: 10px; padding: 10px;");
    lblQrImage->setAlignment(Qt::AlignCenter);

    QString qrData = ipAddress + ":" + pinCode;
    QString qrApiUrl = QString("https://api.qrserver.com/v1/create-qr-code/?size=180x180&data=%1").arg(qrData);

    QNetworkAccessManager *networkManager = new QNetworkAccessManager(&mainWindow);
    QObject::connect(networkManager, &QNetworkAccessManager::finished, [lblQrImage](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray imageData = reply->readAll();
            QPixmap pixmap;
            pixmap.loadFromData(imageData);
            lblQrImage->setPixmap(pixmap.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            lblQrImage->setText("QR Yüklenemedi");
        }
        reply->deleteLater();
    });
    networkManager->get(QNetworkRequest(QUrl(qrApiUrl)));

    rightLayout->addWidget(lblQrImage);

    rootLayout->addLayout(rightLayout, 35);

    PardusServer server;
    server.setProperty("pinCode", pinCode);

    if (!server.startServer(9999)) {
        QListWidgetItem *errItem = new QListWidgetItem(app.style()->standardIcon(QStyle::SP_MessageBoxCritical), "❌ HATA: TCP Sunucu başlatılamadı!");
        listLog->addItem(errItem);
        mainWindow.show();
        return app.exec();
    }

    DeviceDiscovery discovery;
    discovery.startDiscovery();

    QObject::connect(btnChangeFont, &QPushButton::clicked, [&]() {
        bool ok;
        QFont currentFont = txtEditor->font();
        QFont selectedFont = QFontDialog::getFont(&ok, currentFont, &mainWindow, "Pano Metin Yazı Tipini Seçin");
        if (ok) {
            txtEditor->setFont(selectedFont);
            lblStatus->setText("⚙️ Yazı tipi güncellendi.");
        }
    });

    QObject::connect(btnCopyToClipboard, &QPushButton::clicked, [&]() {
        QString editedText = txtEditor->toPlainText();
        QGuiApplication::clipboard()->setText(editedText);
        lblStatus->setText("✅ Metin panoya kopyalandı!");
    });

    QObject::connect(btnClearEditor, &QPushButton::clicked, [&]() {
        txtEditor->clear();
        lblStatus->setText("🗑️ Editör temizlendi.");
    });

    QObject::connect(&server, &PardusServer::clipboardReceived, [&](const QString &text) {
        lblStatus->setText("📋 Yeni metin alındı!");
        txtEditor->setPlainText(text);

        QListWidgetItem *clipItem = new QListWidgetItem(app.style()->standardIcon(QStyle::SP_FileIcon), "[" + QTime::currentTime().toString() + "] Pano Metni Alındı");
        listLog->insertItem(0, clipItem);
    });

    QObject::connect(&server, &PardusServer::fileTransferStarted, [&](const QString &filename, qint64 size) {
        progressBar->setValue(0);
        lblStatus->setText("📥 Dosya Alınıyor: " + filename);
    });

    QObject::connect(&server, &PardusServer::fileTransferProgress, [&](qint64 received, qint64 total) {
        int progress = (total > 0) ? static_cast<int>((received * 100) / total) : 0;
        progressBar->setValue(progress);
    });

    QObject::connect(&server, &PardusServer::fileTransferCompleted, [&](const QString &filepath) {
        progressBar->setValue(100);
        lblStatus->setText("✅ Dosya başarıyla kaydedildi!");
        QFileInfo fileInfo(filepath);
        QString ext = fileInfo.suffix().toLower();

        QIcon fileIcon;
        if (ext == "pdf") {
            fileIcon = app.style()->standardIcon(QStyle::SP_FileIcon);
        } else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif" || ext == "mp4" || ext == "mkv") {
            fileIcon = app.style()->standardIcon(QStyle::SP_MediaPlay);
        } else if (ext == "zip" || ext == "rar" || ext == "tar" || ext == "gz") {
            fileIcon = app.style()->standardIcon(QStyle::SP_DirClosedIcon);
        } else {
            fileIcon = app.style()->standardIcon(QStyle::SP_FileIcon);
        }

        QListWidgetItem *fileItem = new QListWidgetItem(fileIcon, "[" + QTime::currentTime().toString() + "] Dosya: " + fileInfo.fileName());
        // Dosya tam yolunu (filepath) öğenin içerisine veri olarak saklıyoruz
        fileItem->setData(Qt::UserRole, filepath);
        listLog->insertItem(0, fileItem);

        // Dosya tamamlandığında klasör açık değilse (veya her durumda klasörü gösterip dosyayı seçili hale getirmek için)
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
    });

    // Listede bir öğeye çift tıklandığında (Dosya ise) açılmasını sağlayan bağlantı
    QObject::connect(listLog, &QListWidget::itemDoubleClicked, [&](QListWidgetItem *item) {
        QString filepath = item->data(Qt::UserRole).toString();
        if (!filepath.isEmpty() && QFile::exists(filepath)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filepath));
        }
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
