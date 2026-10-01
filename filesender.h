#pragma once
#include <QObject>
#include <QFile>
#include <QHostAddress>

class QTcpSocket;

class FileSender : public QObject
{
    Q_OBJECT
public:
    explicit FileSender(QObject *parent = nullptr) : QObject(parent) {}

    bool isBusy() const { return m_socket != nullptr; }
    void send(const QHostAddress &address, const QString &path);

signals:
    void progress(qint64 sent, qint64 total);
    void finished(bool ok, const QString &message);

private slots:
    void onConnected();
    void onBytesWritten(qint64 n);

private:
    void pump();
    void finish(bool ok, const QString &message);

    QTcpSocket *m_socket = nullptr;
    QFile m_file;
    QByteArray m_header;
    qint64 m_total = 0;
    qint64 m_written = 0;
    bool m_allQueued = false;
};
