#pragma once

#include <QObject>
#include <QString>

class KStatusNotifierItem;
class QAction;

/**
 * TrayManager
 *
 * Manages the Plasma / SNI system-tray icon via KStatusNotifierItem.
 * It provides a context menu and updates the badge with unread message counts.
 */
class TrayManager : public QObject
{
    Q_OBJECT

public:
    explicit TrayManager(QObject *parent = nullptr);
    ~TrayManager() override;

    void setUnreadCount(int count);

Q_SIGNALS:
    void showWindowRequested();
    void newChatRequested();
    void toggleDoNotDisturbRequested();

private:
    void buildContextMenu();

    KStatusNotifierItem *m_sni    = nullptr;
    QAction             *m_dndAct = nullptr;
    int                  m_unread = 0;
};
