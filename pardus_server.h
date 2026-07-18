#ifndef PARDUS_SERVER_H
#define PARDUS_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QHash>
#include <QAbstractSocket>


struct ClientState {
    QByteArray buffer;
    bool receivingFile;
    QFile *currentFile;
    QString currentFilename;
    qint64 currentFileSize;
    qint64 currentFileReceived;
};

class PardusServer : public QObject {
    Q_OBJECT

public:
    explicit PardusServer(QObject *parent = nullptr);
    bool startServer(quint16 port = 9999);

signals:
    void clipboardReceived(const QString &text);
    void fileTransferStarted(const QString &filename, qint64 size);
    void fileTransferProgress(qint64 received, qint64 total);
    void fileTransferCompleted(const QString &filepath);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    void processFileData(QTcpSocket *socket);
    void finishFileTransfer(QTcpSocket *socket);
    // Son parametre varsayılan olarak true (bağlantıyı kapatır), dosya yüklenirken false geçilir.
    void sendHttpResponse(QTcpSocket *socket, int code, const QString &contentType, const QString &body, bool closeConnection = true);
    void updatePardusClipboard(const QString &text);
    QString getDownloadPath() const;

    QTcpServer *tcpServer;
    QHash<QTcpSocket*, ClientState*> clients;
};

#endif