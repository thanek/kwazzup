#include "utils.h"

#include <QApplication>
#include <QPalette>
#include <QStyleHints>
#include <QRegularExpression>
#include <QDir>
#include <QDirIterator>

namespace Utils {

int parseUnreadCount(const QString &pageTitle)
{
    // WhatsApp Web sets title to "(N) WhatsApp" when there are N unread messages
    static const QRegularExpression re(QStringLiteral(R"(\((\d+)\))"));
    const auto match = re.match(pageTitle);
    if (match.hasMatch())
        return match.captured(1).toInt();
    return 0;
}

bool isDarkMode()
{
    // Qt 6.5+ exposes the colour scheme; fall back to luminance heuristic
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    const QColor bg = qApp->palette().color(QPalette::Window);
    return bg.lightness() < 128;
#endif
}

QString defaultUserAgent()
{
    // Looks like a recent Chrome on Linux — WhatsApp Web requires a modern browser UA
    return QStringLiteral(
        "Mozilla/5.0 (X11; Linux x86_64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/132.0.0.0 Safari/537.36");
}

void clearWebEngineCache(const QString &storagePath)
{
    if (storagePath.isEmpty())
        return;

    // Remove the entire storage directory tree (cache, Local Storage, IndexedDB …)
    QDir dir(storagePath);
    if (dir.exists())
        dir.removeRecursively();
}

QString formatBytes(qint64 bytes)
{
    if (bytes < 0)
        return QStringLiteral("0 B");

    const QStringList units{QStringLiteral("B"), QStringLiteral("KiB"),
                            QStringLiteral("MiB"), QStringLiteral("GiB")};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < units.size() - 1) {
        value /= 1024.0;
        ++unit;
    }
    return QStringLiteral("%1 %2").arg(value, 0, 'f', unit == 0 ? 0 : 1).arg(units[unit]);
}

} // namespace Utils
