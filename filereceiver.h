#pragma once
#include <QObject>
#include <QTcpServer>
#include <QFile>

class QTcpSocket;

class FileReceiver : public QObject
{
    Q_OBJECT
public:
    explicit FileReceiver(QObject *parent = nullptr);

    bool start(const QString &saveDir);
    void stop();
    bool isRunning() const { return m_server.isListening(); }
    QString errorString() const { return m_server.errorString(); }

signals:
    void transferStarted(const QString &fileName, qint64 size, const QString &peer);
    void progress(qint64 received, qint64 total);
    void transferFinished(bool ok, const QString &pathOrError);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    bool parseHeader();
    void writeBody();
    void finishTransfer(bool ok, const QString &message);
    static QString uniquePath(const QString &dir, const QString &name);

    QTcpServer m_server;
    QTcpSocket *m_client = nullptr;
    QString m_saveDir;

    QByteArray m_buffer;
    bool m_haveHeader = false;
    QFile m_file;
    QString m_path;
    qint64 m_expected = 0;
    qint64 m_received = 0;
};
