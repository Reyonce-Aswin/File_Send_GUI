#include "filereceiver.h"
#include "common.h"

#include <QTcpSocket>
#include <QFileInfo>
#include <QDir>
#include <QtEndian>

FileReceiver::FileReceiver(QObject *parent) : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &FileReceiver::onNewConnection);
}

bool FileReceiver::start(const QString &saveDir)
{
    m_saveDir = saveDir;
    return m_server.isListening() || m_server.listen(QHostAddress::Any, Protocol::TransferPort);
}

void FileReceiver::stop()
{
    if (m_client) finishTransfer(false, tr("Receiver stopped."));
    m_server.close();
}

void FileReceiver::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *c = m_server.nextPendingConnection();
        if (m_client) {                     // one transfer at a time
            c->close();
            c->deleteLater();
            continue;
        }
        m_client = c;
        m_buffer.clear();
        m_haveHeader = false;
        connect(c, &QTcpSocket::readyRead, this, &FileReceiver::onReadyRead);
        connect(c, &QTcpSocket::disconnected, this, &FileReceiver::onDisconnected);
    }
}

void FileReceiver::onReadyRead()
{
    if (!m_client) return;
    m_buffer.append(m_client->readAll());

    if (!m_haveHeader && !parseHeader()) return;
    if (m_client && m_haveHeader) writeBody();
}

// Returns true once the header has been fully parsed (or the transfer was aborted).
bool FileReceiver::parseHeader()
{
    if (m_buffer.size() < 4) return false;

    const quint32 nameLen = qFromBigEndian<quint32>(m_buffer.constData());
    if (nameLen == 0 || nameLen > quint32(Protocol::MaxNameBytes)) {
        finishTransfer(false, tr("Invalid file name length."));
        return true;
    }
    if (m_buffer.size() < qsizetype(4 + nameLen + 8)) return false;

    const QString rawName = QString::fromUtf8(m_buffer.constData() + 4, int(nameLen));
    m_expected = qint64(qFromBigEndian<quint64>(m_buffer.constData() + 4 + nameLen));
    m_buffer.remove(0, int(4 + nameLen + 8));

    // Never trust the sender's path: keep only the base name
    const QString name = QFileInfo(rawName).fileName();
    if (name.isEmpty() || name == QLatin1String(".") || name == QLatin1String("..")) {
        finishTransfer(false, tr("Invalid file name."));
        return true;
    }

    m_path = uniquePath(m_saveDir, name);
    m_file.setFileName(m_path);
    if (!m_file.open(QIODevice::WriteOnly)) {
        const QString err = m_file.errorString();
        m_path.clear();
        finishTransfer(false, tr("Cannot create file: %1").arg(err));
        return true;
    }

    m_received = 0;
    m_haveHeader = true;
    emit transferStarted(QFileInfo(m_path).fileName(), m_expected,
                         m_client->peerAddress().toString());
    return true;
}

void FileReceiver::writeBody()
{
    if (!m_buffer.isEmpty()) {
        const qint64 take = qMin<qint64>(m_buffer.size(), m_expected - m_received);
        if (take > 0 && m_file.write(m_buffer.constData(), take) != take) {
            finishTransfer(false, tr("Write error: %1").arg(m_file.errorString()));
            return;
        }
        m_received += take;
        m_buffer.clear();
    }

    emit progress(m_received, m_expected);
    if (m_received >= m_expected)
        finishTransfer(true, QString());
}

void FileReceiver::onDisconnected()
{
    if (!m_client) return;
    if (m_client->bytesAvailable()) onReadyRead();   // drain what arrived with the FIN
    if (m_client) finishTransfer(false, tr("Connection lost during transfer."));
}

void FileReceiver::finishTransfer(bool ok, const QString &message)
{
    if (m_file.isOpen()) m_file.close();
    if (!ok && !m_path.isEmpty()) QFile::remove(m_path);   // drop partial files

    const QString path = m_path;
    m_path.clear();
    m_buffer.clear();
    m_haveHeader = false;

    if (m_client) {
        m_client->disconnect(this);
        m_client->close();
        m_client->deleteLater();
        m_client = nullptr;
    }
    emit transferFinished(ok, ok ? path : message);
}

// "photo.jpg" -> "photo (1).jpg" if it already exists (never overwrite silently)
QString FileReceiver::uniquePath(const QString &dir, const QString &name)
{
    const QDir d(dir);
    if (!d.exists(name)) return d.filePath(name);

    const QFileInfo fi(name);
    const QString base = fi.completeBaseName();
    const QString suffix = fi.suffix().isEmpty() ? QString() : "." + fi.suffix();
    for (int i = 1;; ++i) {
        const QString candidate = QString("%1 (%2)%3").arg(base).arg(i).arg(suffix);
        if (!d.exists(candidate)) return d.filePath(candidate);
    }
}
