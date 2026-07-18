#include "pardus_server.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QPointer>
#include <QFileInfo>

PardusServer::PardusServer(QObject *parent) : QObject(parent) {
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &PardusServer::onNewConnection);
}

bool PardusServer::startServer(quint16 port) {
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Sunucu baslatilamadi! Hata:" << tcpServer->errorString();
        return false;
    }
    qDebug() << "Pardus LAN Share aktif. Port:" << port;
    return true;
}

void PardusServer::onNewConnection() {
    while (tcpServer->hasPendingConnections()) {
        QTcpSocket *newClient = tcpServer->nextPendingConnection();
        if (!newClient) continue;

        ClientState *state = new ClientState();
        state->receivingFile = false;
        state->currentFile = nullptr;
        state->currentFileSize = 0;
        state->currentFileReceived = 0;
        state->buffer.clear();

        clients[newClient] = state;

        connect(newClient, &QTcpSocket::readyRead, this, &PardusServer::onReadyRead);
        connect(newClient, &QTcpSocket::disconnected, this, &PardusServer::onClientDisconnected);

        // KESİN ÇÖZÜM (Qt 6 Hatası): QOverload yerine doğrudan yeni errorOccurred sinyalini bağlıyoruz
        connect(newClient, &QTcpSocket::errorOccurred, this, &PardusServer::onSocketError);

        qDebug() << "Yeni istemci baglandi:" << newClient->peerAddress().toString();
    }
}

void PardusServer::onSocketError(QAbstractSocket::SocketError socketError) {
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;
    qDebug() << "Socket hatasi:" << socketError << senderSocket->errorString();
}



void PardusServer::onReadyRead() {
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket || !clients.contains(senderSocket)) return;

    ClientState *state = clients[senderSocket];

    // Gelen veriyi tampona (buffer) ekle
    QByteArray newData = senderSocket->readAll();
    state->buffer.append(newData);

    // KURAL 1: Eğer zaten dosya transfer modundaysak, veriyi doğrudan dosyaya işle
    if (state->receivingFile) {
        processFileData(senderSocket);
        return;
    }

    // HTTP Header sonunu ara
    int headerEnd = state->buffer.indexOf("\r\n\r\n");
    if (headerEnd == -1) return; // Header henüz tamamlanmadıysa bir sonraki paketi bekle

    QByteArray headerBytes = state->buffer.left(headerEnd);
    QString requestStr = QString::fromUtf8(headerBytes);
    QStringList lines = requestStr.split("\r\n");

    // HTTP Başlığından Content-Length (Metin Boyutu) değerini ayıkla
    qint64 contentLength = 0;
    for (const QString &line : lines) {
        if (line.startsWith("Content-Length:", Qt::CaseInsensitive)) {
            contentLength = line.mid(15).trimmed().toLongLong();
        }
    }

    // Toplam beklenen paket boyutu = Header uzunluğu + 4 byte (\r\n\r\n) + Content-Length
    qint64 totalExpectedSize = headerEnd + 4 + contentLength;

    // EĞER verinin tamamı henüz gelmediyse, fonksiyonu durdur ve ağdan yeni paket gelmesini bekle!
    if (state->buffer.size() < totalExpectedSize) {
        return;
    }

    // Buraya geldiysek artık metnin TAMAMI buffer içinde birikmiş demektir.
    QByteArray bodyBytes = state->buffer.mid(headerEnd + 4, contentLength);

    // === PANO PAYLAŞIMI (POST /share-clipboard) ===
    if (requestStr.startsWith("POST /share-clipboard")) {
        QString body = QString::fromUtf8(bodyBytes); // trimmed() kaldırıldı, kullanıcının kasıtlı boşlukları yutulmasın
        if (!body.isEmpty()) {
            updatePardusClipboard(body);
        }
        state->buffer.clear();
        sendHttpResponse(senderSocket, 200, "text/plain", "Basarili!", true);
        return;
    }

    // === DOSYA TRANSFERİ (POST /upload) ===
    if (requestStr.startsWith("POST /upload")) {
        QString filename;
        qint64 filesize = 0;

        for (const QString &line : lines) {
            if (line.startsWith("X-Filename:")) filename = line.mid(11).trimmed();
            if (line.startsWith("X-Filesize:")) filesize = line.mid(11).trimmed().toLongLong();
        }

        if (!filename.isEmpty() && filesize > 0) {
            QString downloadPath = getDownloadPath();
            QDir().mkpath(downloadPath);
            QString filepath = downloadPath + "/" + filename;

            if (QFile::exists(filepath)) QFile::remove(filepath);

            QFile *file = new QFile(filepath);
            if (!file->open(QIODevice::WriteOnly)) {
                delete file;
                state->buffer.clear();
                sendHttpResponse(senderSocket, 500, "text/plain", "FAILED", true);
                return;
            }

            state->currentFile = file;
            state->currentFilename = filename;
            state->currentFileSize = filesize;
            state->currentFileReceived = 0;
            state->receivingFile = true;

            // Buffer'ı sadece başlığı kesip, kalan gövde (body) verileriyle dosya moduna geçir
            state->buffer = state->buffer.mid(headerEnd + 4);

            qDebug() << "Dosya transferi baslatiliyor:" << filename << "Toplam Boyut:" << filesize;
            emit fileTransferStarted(filename, filesize);

            // İlk pakette kalan gövde verisini diske yazmak için tetikle
            processFileData(senderSocket);
            return;
        }

        state->buffer.clear();
        sendHttpResponse(senderSocket, 400, "text/plain", "Bad Request", true);
        return;
    }

    // === SAĞLIK KONTROLÜ (GET /share-clipboard) ===
    if (requestStr.startsWith("GET /share-clipboard")) {
        state->buffer.clear();
        sendHttpResponse(senderSocket, 200, "text/plain", "Pardus LAN Share Active", true);
        return;
    }

    state->buffer.clear();
    sendHttpResponse(senderSocket, 404, "text/plain", "Not Found", true);
}

void PardusServer::processFileData(QTcpSocket *socket) {
    if (!clients.contains(socket)) return;
    ClientState *state = clients[socket];

    if (!state->receivingFile || !state->currentFile || !state->currentFile->isOpen()) return;

    // Buffer'da biriken ham TCP paket verisini dosyaya yaz
    if (!state->buffer.isEmpty()) {
        qint64 bytesWritten = state->currentFile->write(state->buffer);
        if (bytesWritten > 0) {
            state->currentFileReceived += bytesWritten;
        }
        state->buffer.clear(); // Paket işlendi, buffer'ı boşalt
    }

    qint64 received = state->currentFileReceived;
    qint64 total = state->currentFileSize;

    emit fileTransferProgress(received, total);

    int progress = (total > 0) ? static_cast<int>((received * 100) / total) : 0;
    qDebug() << "Dosya Aliniyor: %" << progress << " (" << received << "/" << total << ")";

    // KESİN KONTROL: Eğer gelen veri hedeflenen boyuta ulaştıysa VEYA geçtüyse transferi bitir
    if (received >= total) {
        state->buffer.clear();
        finishFileTransfer(socket);
    }
}

void PardusServer::finishFileTransfer(QTcpSocket *socket) {
    if (!clients.contains(socket)) return;
    ClientState *state = clients[socket];

    if (!state->receivingFile) return;

    QString filepath;
    if (state->currentFile && state->currentFile->isOpen()) {
        QFileInfo fileInfo(*state->currentFile);
        filepath = fileInfo.absoluteFilePath();

        state->currentFile->close();
        delete state->currentFile;
    }

    state->currentFile = nullptr;
    // DÜZELTME: Kapanma krizini önlemek için bayrağı erkenden indiriyoruz
    state->receivingFile = false;

    qDebug() << "========================================";
    qDebug() << "💾 DOSYA TAM KONUMU:" << filepath;
    qDebug() << "========================================";

    emit fileTransferCompleted(filepath);

    // Yanıtı gönderip soketi güvenle sonlandırıyoruz
    sendHttpResponse(socket, 200, "text/plain", "SUCCESS", true);
}

void PardusServer::sendHttpResponse(QTcpSocket *socket, int code, const QString &contentType, const QString &body, bool closeConnection) {
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QString statusText = "OK";
    if (code == 400) statusText = "Bad Request";
    else if (code == 404) statusText = "Not Found";
    else if (code == 500) statusText = "Internal Server Error";

    QString response = QString("HTTP/1.1 %1 %2\r\n"
                               "Content-Type: %3\r\n"
                               "Access-Control-Allow-Origin: *\r\n"
                               "Content-Length: %4\r\n"
                               "Connection: %5\r\n"
                               "\r\n"
                               "%6")
                           .arg(code)
                           .arg(statusText)
                           .arg(contentType)
                           .arg(body.toUtf8().size())
                           .arg(closeConnection ? "close" : "keep-alive")
                           .arg(body);

    socket->write(response.toUtf8());
    socket->flush();

    if (closeConnection) {
        // DÜZELTME: Döngüsel çökmeleri engellemek için Lambda bağlantısını teke düşürüp güvenli hale getirdik.
        if (socket->bytesToWrite() == 0) {
            socket->disconnectFromHost();
        } else {
            QPointer<QTcpSocket> safeSocket(socket);
            connect(socket, &QTcpSocket::bytesWritten, this, [safeSocket]() {
                if (safeSocket && safeSocket->bytesToWrite() == 0) {
                    safeSocket->disconnectFromHost();
                }
            }, Qt::UniqueConnection); // Mükerrer tetiklenmeyi önler
        }
    }
}

void PardusServer::updatePardusClipboard(const QString &text) {
    QTimer::singleShot(0, this, [this, text]() {
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard) {
            if (clipboard->text() != text) {
                clipboard->setText(text);
                qDebug() << "Pano Guncellendi ->" << text.left(50);
                emit clipboardReceived(text);
            }
        }
    });
}

void PardusServer::onClientDisconnected() {
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    qDebug() << "Istemci ayrildi:" << senderSocket->peerAddress().toString();

    if (clients.contains(senderSocket)) {
        ClientState *state = clients.take(senderSocket);
        if (state) {
            // Sadece dosya transferi GERÇEKTEN yarım kaldıysa (finish çağrılmadıysa) sil
            if (state->receivingFile && state->currentFile) {
                QString filepath = state->currentFile->fileName();
                state->currentFile->close();
                QFile::remove(filepath);
                delete state->currentFile;
                qDebug() << "Dosya transferi yarim kaldi (Istemci koptu), silindi:" << filepath;
            } else if (state->currentFile) {
                // Her ihtimale karşı açık pointer kaldıysa temizle ama dosyayı silme
                delete state->currentFile;
            }
            delete state;
        }
    }
    senderSocket->deleteLater();
}

QString PardusServer::getDownloadPath() const {
    // Dosyaların kaydedileceği klasör: "İndirilenler/PardusLanShare" klasörüdür.
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/PardusLanShare";
}