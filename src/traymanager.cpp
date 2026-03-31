#include "traymanager.h"

#include <KStatusNotifierItem>
#include <KLocalizedString>

#include <QAction>
#include <QMenu>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QFont>

// ─────────────────────────────────────────────────────────────────────────────
// Helper: draw a badge overlay with the unread count on top of the app icon
// ─────────────────────────────────────────────────────────────────────────────
static QIcon badgedIcon(const QIcon &base, int count)
{
    if (count <= 0)
        return base;

    const int sz = 22;
    QPixmap pix = base.pixmap(sz, sz);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Red badge circle
    const QString text = count > 99 ? QStringLiteral("99+") : QString::number(count);
    QFont f = p.font();
    f.setPixelSize(count > 9 ? 7 : 9);
    f.setBold(true);
    p.setFont(f);

    QFontMetrics fm(f);
    const int tw = fm.horizontalAdvance(text);
    const int bw = qMax(tw + 4, 10);
    const int bh = 10;
    const QRect badge(sz - bw, 0, bw, bh);

    p.setBrush(QColor(0xE5, 0x39, 0x35));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(badge, bh / 2.0, bh / 2.0);

    p.setPen(Qt::white);
    p.drawText(badge, Qt::AlignCenter, text);
    p.end();

    return QIcon(pix);
}

// ─────────────────────────────────────────────────────────────────────────────

TrayManager::TrayManager(QObject *parent)
    : QObject(parent)
{
    m_sni = new KStatusNotifierItem(QStringLiteral("kwazzup"), this);
    m_sni->setCategory(KStatusNotifierItem::Communications);
    m_sni->setStatus(KStatusNotifierItem::Active);
    m_sni->setTitle(i18n("KWazzup"));

    // Use the icon from the Qt resource as pixmap so it works before installation.
    // fromTheme() finds it via the fallback search path set in main().
    const QIcon appIcon = QIcon::fromTheme(QStringLiteral("kwazzup"),
                                           QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/kwazzup.svg")));
    m_sni->setIconByPixmap(appIcon);
    m_sni->setToolTipIconByPixmap(appIcon);
    m_sni->setToolTip(QString(), i18n("KWazzup"), i18n("An unofficial WhatsApp client for KDE"));

    // Activate (left-click / double-click) → show the main window
    connect(m_sni, &KStatusNotifierItem::activateRequested,
            this, [this](bool /*active*/, const QPoint &) {
                Q_EMIT showWindowRequested();
            });

    buildContextMenu();
}

TrayManager::~TrayManager() = default;

void TrayManager::buildContextMenu()
{
    auto *menu = new QMenu();

    auto *showAct = menu->addAction(QIcon::fromTheme(QStringLiteral("window-restore")),
                                    i18n("&Show Window"));
    connect(showAct, &QAction::triggered, this, &TrayManager::showWindowRequested);

    auto *newChatAct = menu->addAction(QIcon::fromTheme(QStringLiteral("document-new")),
                                       i18n("&New Chat…"));
    connect(newChatAct, &QAction::triggered, this, &TrayManager::newChatRequested);

    menu->addSeparator();

    m_dndAct = menu->addAction(QIcon::fromTheme(QStringLiteral("notification-inactive")),
                               i18n("&Do Not Disturb"));
    m_dndAct->setCheckable(true);
    connect(m_dndAct, &QAction::toggled, this, &TrayManager::toggleDoNotDisturbRequested);

    // KStatusNotifierItem adds its own "Quit" action automatically — no need
    // to add a duplicate here.

    m_sni->setContextMenu(menu);
}

void TrayManager::setUnreadCount(int count)
{
    if (m_unread == count)
        return;
    m_unread = count;

    const QIcon base = QIcon::fromTheme(QStringLiteral("kwazzup"),
                                        QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/kwazzup.svg")));
    m_sni->setIconByPixmap(badgedIcon(base, count));

    if (count > 0) {
        m_sni->setStatus(KStatusNotifierItem::NeedsAttention);
        m_sni->setToolTip(QString(),
                          i18n("KWazzup"),
                          i18np("1 unread message", "%1 unread messages", count));
    } else {
        m_sni->setStatus(KStatusNotifierItem::Active);
        m_sni->setToolTip(QString(),
                          i18n("KWazzup"),
                          i18n("An unofficial WhatsApp client for KDE"));
    }
}
