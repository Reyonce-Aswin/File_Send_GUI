#include "discovery.h"
#include "common.h"

#include <QNetworkInterface>

Discovery::Discovery(QObject *parent) : QObject(parent)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this] { emit scanFinished(m_found.size()); });
    connect(&m_listener, &QUdpSocket::readyRead, this, &Discovery::onListenerReadyRead);
    connect(&m_scanner, &QUdpSocket::readyRead, this, &Discovery::onScannerReadyRead);
}

bool Discovery::startListening()
{
    if (isListening()) return true;
    return m_listener.bind(QHostAddress::AnyIPv4, Protocol::DiscoveryPort,
                           QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
}

void Discovery::stopListening()
{
    m_listener.close();
}

void Discovery::onListenerReadyRead()
{
    while (m_listener.hasPendingDatagrams()) {
        QByteArray data;
        data.resize(int(m_listener.pendingDatagramSize()));
        QHostAddress from;
        quint16 port = 0;
        m_listener.readDatagram(data.data(), data.size(), &from, &port);

        if (data.startsWith("DISCOVER"))
            m_listener.writeDatagram("NAME:" + m_name.toUtf8(), from, port);
    }
}

void Discovery::scan(int timeoutMs)
{
    m_found.clear();
    if (m_scanner.state() == QAbstractSocket::UnconnectedState)
        m_scanner.bind(QHostAddress::AnyIPv4, 0);

    // Send to every adapter's own broadcast address (e.g. 192.168.1.255).
    // A single 255.255.255.255 broadcast often leaves via the wrong adapter
    // on Windows when VPN / VirtualBox / WSL / Hyper-V adapters are present.
    const auto ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &ifc : ifaces) {
        const auto flags = ifc.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)
            || (flags & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &e : ifc.addressEntries()) {
            if (e.ip().protocol() != QAbstractSocket::IPv4Protocol || e.broadcast().isNull())
                continue;
            m_scanner.writeDatagram("DISCOVER", e.broadcast(), Protocol::DiscoveryPort);
        }
    }
    // Fallback
    m_scanner.writeDatagram("DISCOVER", QHostAddress::Broadcast, Protocol::DiscoveryPort);
    m_timer.start(timeoutMs);
}

void Discovery::onScannerReadyRead()
{
    while (m_scanner.hasPendingDatagrams()) {
        QByteArray data;
        data.resize(int(m_scanner.pendingDatagramSize()));
        QHostAddress from;
        m_scanner.readDatagram(data.data(), data.size(), &from);

        if (!data.startsWith("NAME:")) continue;

        bool ok = false;
        QHostAddress v4(from.toIPv4Address(&ok));
        if (!ok) continue;

        const QString ip = v4.toString();
        if (m_found.contains(ip)) continue;
        m_found.insert(ip);

        emit deviceFound({QString::fromUtf8(data.mid(5)), v4});
    }
}
