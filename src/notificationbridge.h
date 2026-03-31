#pragma once

#include <QObject>
#include <QString>
#include <QHash>

class KNotification;

/**
 * NotificationBridge
 *
 * Exposed to WhatsApp Web's JavaScript context via QWebChannel under the key
 * "kwazzupBridge".  The injected JS overrides window.Notification so that
 * every WhatsApp notification is forwarded here and shown as a native
 * KNotification instead of a Chromium popup.
 */
class NotificationBridge : public QObject
{
    Q_OBJECT

public:
    explicit NotificationBridge(QObject *parent = nullptr);
    ~NotificationBridge() override;

public Q_SLOTS:
    // Called from JavaScript: new Notification(title, {body, icon, tag})
    void showNotification(const QString &title,
                          const QString &body,
                          const QString &iconUrl,
                          const QString &tag);

    // Called from JavaScript when a notification is closed programmatically
    void closeNotification(const QString &tag);

    // Called from JavaScript to update the unread badge count
    void updateUnreadCount(int count);

    // Called from C++ to suppress notifications while Do Not Disturb is on
    void setDoNotDisturb(bool enabled);

    // Called from C++ to enable/disable notifications entirely
    void setEnabled(bool enabled);

    // Called from C++ to enable/disable notification sounds
    void setSoundEnabled(bool enabled);

Q_SIGNALS:
    // Emitted when the user clicks a notification — main window should show itself
    void activationRequested();

    // Emitted when unread count changes
    void unreadCountChanged(int count);

private:
    QHash<QString, KNotification *> m_notifications;
    bool m_doNotDisturb  = false;
    bool m_enabled       = true;
    bool m_soundEnabled  = true;
};
