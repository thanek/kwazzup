#include "notificationbridge.h"

#include <KNotification>
#include <KLocalizedString>

NotificationBridge::NotificationBridge(QObject *parent)
    : QObject(parent)
{
}

NotificationBridge::~NotificationBridge() = default;

void NotificationBridge::setDoNotDisturb(bool enabled)
{
    m_doNotDisturb = enabled;
}

void NotificationBridge::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void NotificationBridge::setSoundEnabled(bool enabled)
{
    m_soundEnabled = enabled;
}

void NotificationBridge::showNotification(const QString &title,
                                          const QString &body,
                                          const QString & /*iconUrl*/,
                                          const QString &tag)
{
    if (m_doNotDisturb || !m_enabled)
        return;

    // If there is already an active notification with the same tag, close it first
    if (!tag.isEmpty()) {
        if (auto *existing = m_notifications.value(tag)) {
            existing->close();
            m_notifications.remove(tag);
        }
    }

    const QString eventName = m_soundEnabled
        ? QStringLiteral("NewMessage")
        : QStringLiteral("NewMessageSilent");
    auto *notif = new KNotification(eventName, KNotification::CloseOnTimeout, this);
    notif->setTitle(title.isEmpty() ? i18n("WhatsApp") : title);
    notif->setText(body);
    notif->setIconName(QStringLiteral("kwazzup"));

    // Show with "Open" action that brings the main window to front
    auto *action = notif->addDefaultAction(i18n("Open"));
    connect(action, &KNotificationAction::activated, this, &NotificationBridge::activationRequested);
    connect(notif, &KNotification::closed, this, [this, tag, notif]() {
        if (!tag.isEmpty())
            m_notifications.remove(tag);
        notif->deleteLater();
    });

    if (!tag.isEmpty())
        m_notifications.insert(tag, notif);

    notif->sendEvent();
}

void NotificationBridge::closeNotification(const QString &tag)
{
    if (auto *notif = m_notifications.take(tag)) {
        notif->close();
    }
}

void NotificationBridge::updateUnreadCount(int count)
{
    Q_EMIT unreadCountChanged(count);
}
