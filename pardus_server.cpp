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

    QByteArray newData = senderSocket->readAll();
    state->buffer.append(newData);

    if (state->receivingFile) {
        processFileData(senderSocket);
        return;
    }

    int headerEnd = state->buffer.indexOf("\r\n\r\n");
    if (headerEnd == -1) {
        if (state->buffer.size() > 8192) {
            qDebug() << "⚠️ Aşırı büyük HTTP başlığı reddedildi.";
            state->buffer.clear();
            senderSocket->disconnectFromHost();
        }
        return;
    }


    QByteArray headerBytes = state->buffer.left(headerEnd);
    QString requestStr = QString::fromUtf8(headerBytes);
    QStringList lines = requestStr.split("\r\n");

    qint64 contentLength = 0;
    for (const QString &line : lines) {
        if (line.startsWith("Content-Length:", Qt::CaseInsensitive)) {
            contentLength = line.mid(15).trimmed().toLongLong();
            break;
        }
    }



    if (!requestStr.startsWith("POST /upload")) {
        qint64 totalExpectedSize = headerEnd + 4 + contentLength;
        if (state->buffer.size() < totalExpectedSize) {
            return;
        }
    }


    QString correctPin = this->property("pinCode").toString();
    QString clientPin = "";

    for (const QString &line : lines) {
        if (line.startsWith("X-Pin-Code:", Qt::CaseInsensitive)) {
            clientPin = line.mid(11).trimmed();
            break;
        }
    }

    if (clientPin.isEmpty() || clientPin != correctPin) {
        qDebug() << "❌ Yetkisiz Erişim Denemesi! Gelen PIN:" << clientPin << "Beklenen PIN:" << correctPin;
        state->buffer.clear();
        sendHttpResponse(senderSocket, 401, "text/plain", "Unauthorized: Gecersiz veya Eski PIN!", true);
        return;
    }


    if (requestStr.startsWith("POST /upload")) {
        QString filename;
        qint64 filesize = 0;

        for (const QString &line : lines) {
            if (line.startsWith("X-Filename:", Qt::CaseInsensitive)) filename = line.mid(11).trimmed();
            if (line.startsWith("X-Filesize:", Qt::CaseInsensitive)) filesize = line.mid(11).trimmed().toLongLong();
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


            state->buffer = state->buffer.mid(headerEnd + 4);

            qDebug() << "🚀 Dosya transferi onaylandı:" << filename << "Boyut:" << filesize;
            emit fileTransferStarted(filename, filesize);

            processFileData(senderSocket);
            return;
        }

        state->buffer.clear();
        sendHttpResponse(senderSocket, 400, "text/plain", "Bad Request", true);
        return;
    }


    QByteArray bodyBytes = state->buffer.mid(headerEnd + 4, contentLength);


    if (requestStr.startsWith("POST /share-clipboard")) {
        QString body = QString::fromUtf8(bodyBytes);
        if (!body.isEmpty()) {
            updatePardusClipboard(body);
        }
        state->buffer.clear();
        sendHttpResponse(senderSocket, 200, "text/plain", "Basarili!", true);
        return;
    }


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


    if (!state->buffer.isEmpty()) {
        qint64 bytesWritten = state->currentFile->write(state->buffer);
        if (bytesWritten > 0) {
            state->currentFileReceived += bytesWritten;
        }
        state->buffer.clear();
    }

    qint64 received = state->currentFileReceived;
    qint64 total = state->currentFileSize;

    emit fileTransferProgress(received, total);

    int progress = (total > 0) ? static_cast<int>((received * 100) / total) : 0;
    qDebug() << "Dosya Aliniyor: %" << progress << " (" << received << "/" << total << ")";


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

    state->receivingFile = false;

    qDebug() << "========================================";
    qDebug() << "💾 DOSYA TAM KONUMU:" << filepath;
    qDebug() << "========================================";

    emit fileTransferCompleted(filepath);


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
            if (state->receivingFile && state->currentFile) {
                QString filepath = state->currentFile->fileName();
                state->currentFile->close();
                QFile::remove(filepath);
                delete state->currentFile;
                qDebug() << "Dosya transferi yarim kaldi (Istemci koptu), silindi:" << filepath;
            } else if (state->currentFile) {
                delete state->currentFile;
            }
            delete state;
        }
    }
    senderSocket->deleteLater();
}

QString PardusServer::getDownloadPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/PardusLanShare";
}