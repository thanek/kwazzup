#pragma once

#include <QString>
#include <QColor>

namespace Utils {

// Returns the number parsed from WhatsApp Web's page title, e.g. "(3) WhatsApp" → 3
int parseUnreadCount(const QString &pageTitle);

// True when KDE / Plasma colour scheme is currently dark
bool isDarkMode();

// Suitable Chrome-like user-agent that WhatsApp Web accepts
QString defaultUserAgent();

// Remove all QtWebEngine cache / storage for the given profile storage path
void clearWebEngineCache(const QString &storagePath);

// Human-readable storage size string, e.g. 1 048 576 → "1.0 MiB"
QString formatBytes(qint64 bytes);

} // namespace Utils
