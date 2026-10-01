#include "filesender.h"
#include "common.h"

#include <QTcpSocket>
#include <QFileInfo>
#include <QtEndian>

// Wire format (all integers big-endian):
//   uint32 nameLength | name (UTF-8) | uint64 fileSize | file bytes
void FileSender::send(const QHostAddress &address, const QString &path)
{
    if (m_socket) return;

    m_file.setFileName(path);
    if (!m_file.open(QIODevice::ReadOnly)) {
        emit finished(false, tr("Cannot open file: %1").arg(m_file.errorString()));
        return;
    }

    const QByteArray name = QFileInfo(path).fileName().toUtf8();   // basename only
    if (name.isEmpty() || name.size() > Protocol::MaxNameBytes) {
        m_file.close();
        emit finished(false, tr("File name is too long."));
        return;
    }

    m_total = m_file.size();
    m_written = 0;
    m_allQueued = false;

    QByteArray len(4, 0), size(8, 0);
    qToBigEndian<quint32>(quint32(name.size()), len.data());
    qToBigEndian<quint64>(quint64(m_total), size.data());
    m_header = len + name + size;

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &FileSender::onConnected);
    connect(m_socket, &QTcpSocket::bytesWritten, this, &FileSender::onBytesWritten);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this] {
        finish(false, m_socket ? m_socket->errorString() : QString());
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [this] {
        finish(m_allQueued && m_written >= m_header.size() + m_total,
               tr("Connection closed before the transfer completed."));
    });

    m_socket->connectToHost(address, Protocol::TransferPort);
}

void FileSender::onConnected()
{
    m_socket->write(m_header);
    pump();
}

// Keep the socket's write buffer topped up without loading the whole file in RAM.
void FileSender::pump()
{
    while (m_socket && m_socket->bytesToWrite() < 4 * Protocol::ChunkSize && !m_file.atEnd()) {
        const QByteArray chunk = m_file.read(Protocol::ChunkSize);
        if (chunk.isEmpty()) break;
        m_socket->write(chunk);
    }
    if (m_file.atEnd()) m_allQueued = true;
}

void FileSender::onBytesWritten(qint64 n)
{
    m_written += n;
    emit progress(qMax<qint64>(0, m_written - m_header.size()), m_total);

    if (m_written >= m_header.size() + m_total) {
        m_socket->disconnectFromHost();   // flushes, then "disconnected" -> finish(true)
        return;
    }
    pump();
}

void FileSender::finish(bool ok, const QString &message)
{
    if (!m_socket) return;                 // already finished

    m_socket->disconnect(this);
    m_socket->abort();
    m_socket->deleteLater();
    m_socket = nullptr;
    m_file.close();

    emit finished(ok, ok ? tr("File sent successfully.") : message);
}
