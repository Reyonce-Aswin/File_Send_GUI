#pragma once
#include <QString>
#include <QtGlobal>

namespace Protocol {
constexpr quint16 DiscoveryPort = 6000;   // UDP
constexpr quint16 TransferPort  = 5000;   // TCP
constexpr int     MaxNameBytes  = 255;
constexpr int     ChunkSize     = 64 * 1024;
}

// Same rules as the old normalize_device_name(): keep letters, digits, '-', '_',
// turn spaces into '-', fall back to "Device" if nothing is left.
inline QString normalizeDeviceName(const QString &in)
{
    QString out;
    for (const QChar c : in) {
        if (c == QLatin1Char(' '))
            out += QLatin1Char('-');
        else if (c.isLetterOrNumber() || c == QLatin1Char('-') || c == QLatin1Char('_'))
            out += c;
    }
    return out.isEmpty() ? QStringLiteral("Device") : out.left(32);
}
