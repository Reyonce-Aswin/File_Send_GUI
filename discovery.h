#pragma once
#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>
#include <QTimer>
#include <QSet>

struct Device {
    QString name;
    QHostAddress address;
};

class Discovery : public QObject
{
    Q_OBJECT
public:
    explicit Discovery(QObject *parent = nullptr);

    void setDeviceName(const QString &name) { m_name = name; }

    // Receiver side: answer "DISCOVER" broadcasts with "NAME:<name>"
    bool startListening();
    void stopListening();
    bool isListening() const { return m_listener.state() == QAbstractSocket::BoundState; }

    // Sender side: broadcast and collect replies for timeoutMs
    void scan(int timeoutMs = 2000);

signals:
    void deviceFound(const Device &device);
    void scanFinished(int count);

private slots:
    void onListenerReadyRead();
    void onScannerReadyRead();

private:
    QUdpSocket m_listener;
    QUdpSocket m_scanner;
    QTimer m_timer;
    QString m_name = QStringLiteral("Device");
    QSet<QString> m_found;
};
