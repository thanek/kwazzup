#include "mainwindow.h"
#include "webenginepage.h"
#include "notificationbridge.h"
#include "traymanager.h"
#include "settingsdialog.h"
#include "utils.h"

#include <KActionCollection>
#include <KStandardAction>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KGlobalAccel>
#include <KWindowSystem>
#include <KMessageBox>
#include <KAboutData>
#include <KColorSchemeManager>
#include <KSelectAction>

#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>
#include <QWebEngineSettings>

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QStyle>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

static constexpr char kWazzupUrl[] = "https://web.whatsapp.com";
static constexpr char kCfgGroup[]    = "MainWindow";

// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    setObjectName(QStringLiteral("MainWindow"));
    setWindowTitle(i18n("KWazzup"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kwazzup")));
    setMinimumSize(800, 600);

    // ── Web engine ───────────────────────────────────────────────────────────
    setupWebEngine();

    // ── Tray ─────────────────────────────────────────────────────────────────
    m_tray = new TrayManager(this);
    connect(m_tray, &TrayManager::showWindowRequested,         this, &MainWindow::bringToFront);
    connect(m_tray, &TrayManager::newChatRequested,            this, [this]() { openNewChat(); });
    connect(m_tray, &TrayManager::toggleDoNotDisturbRequested, this, &MainWindow::slotToggleDoNotDisturb);

    // ── Actions / menus / shortcuts ──────────────────────────────────────────
    setupActions();
    setupGUI(Keys | Save | Create, QStringLiteral("kwazzupui.rc"));

    // KXmlGuiWindow auto-adds "Find Command…" (KCommandBar); remove it — it
    // serves no purpose in a single-page web-app.
    if (auto *act = actionCollection()->action(QStringLiteral("open_kcommand_bar")))
        actionCollection()->removeAction(act);

    // ── Palette / theme changes ───────────────────────────────────────────────
    // palette changes handled in changeEvent()
    applyColorScheme();

    // ── Load settings ────────────────────────────────────────────────────────
    loadSettings();

    // ── Navigate to WhatsApp Web ──────────────────────────────────────────────
    m_webView->load(QUrl(QStringLiteral("https://web.whatsapp.com")));

}

MainWindow::~MainWindow()
{
    saveSettings();
}

// ─────────────────────────────────────────────────────────────────────────────
// Web engine setup
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupWebEngine()
{
    // Use a persistent profile (stores session / cookies across restarts)
    const QString profilePath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/profile");
    QDir().mkpath(profilePath);

    auto *profile = new QWebEngineProfile(QStringLiteral("kwazzup"), this);
    profile->setPersistentStoragePath(profilePath);
    profile->setCachePath(profilePath + QStringLiteral("/cache"));
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpUserAgent(Utils::defaultUserAgent());

    // Spell checking
    profile->setSpellCheckEnabled(true);

    m_page    = new WebEnginePage(profile, this);
    m_webView = new QWebEngineView(this);
    m_webView->setPage(m_page);
    setCentralWidget(m_webView);

    // Wire WebEngine signals
    connect(m_webView, &QWebEngineView::titleChanged, this, &MainWindow::onTitleChanged);
    connect(m_webView, &QWebEngineView::iconChanged,  this, &MainWindow::onIconChanged);

    // Downloads
    connect(m_page, &WebEnginePage::downloadRequested,
            this, &MainWindow::onDownloadRequested);

    // Note: bridge → tray connections are set up in the constructor
    // after TrayManager is created (m_tray is still null at this point).
}

// ─────────────────────────────────────────────────────────────────────────────
// Actions
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupActions()
{
    auto *ac = actionCollection();

    // ── Standard KDE actions ─────────────────────────────────────────────────
    KStandardAction::quit(qApp, &QApplication::quit, ac);
    KStandardAction::preferences(this, &MainWindow::slotShowSettings, ac);
    KStandardAction::fullScreen(this, &MainWindow::slotToggleFullscreen, this, ac);

    // KStandardAction::close() maps to Ctrl+W by default (configurable in
    // System Settings → Shortcuts → KDE Standard Shortcuts).
    // For a tray application "close window" means "hide to tray", not "quit".
    KStandardAction::close(this, &MainWindow::slotCloseWindow, ac);

    // ── Custom actions ───────────────────────────────────────────────────────
    auto *reloadAct = ac->addAction(QStringLiteral("reload_page"), this, &MainWindow::slotReload);
    reloadAct->setText(i18n("&Reload"));
    reloadAct->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    ac->setDefaultShortcut(reloadAct, QKeySequence(Qt::CTRL | Qt::Key_R));

    auto *zoomInAct = ac->addAction(QStringLiteral("zoom_in"), this, &MainWindow::slotZoomIn);
    zoomInAct->setText(i18n("Zoom &In"));
    zoomInAct->setIcon(QIcon::fromTheme(QStringLiteral("zoom-in")));
    ac->setDefaultShortcut(zoomInAct, QKeySequence(Qt::CTRL | Qt::Key_Plus));

    auto *zoomOutAct = ac->addAction(QStringLiteral("zoom_out"), this, &MainWindow::slotZoomOut);
    zoomOutAct->setText(i18n("Zoom &Out"));
    zoomOutAct->setIcon(QIcon::fromTheme(QStringLiteral("zoom-out")));
    ac->setDefaultShortcut(zoomOutAct, QKeySequence(Qt::CTRL | Qt::Key_Minus));

    auto *zoomResetAct = ac->addAction(QStringLiteral("zoom_reset"), this, &MainWindow::slotZoomReset);
    zoomResetAct->setText(i18n("Reset &Zoom"));
    zoomResetAct->setIcon(QIcon::fromTheme(QStringLiteral("zoom-original")));
    ac->setDefaultShortcut(zoomResetAct, QKeySequence(Qt::CTRL | Qt::Key_0));

    auto *newChatAct = ac->addAction(QStringLiteral("new_chat"), this, &MainWindow::slotNewChat);
    newChatAct->setText(i18n("&New Chat…"));
    newChatAct->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    ac->setDefaultShortcut(newChatAct, QKeySequence(Qt::CTRL | Qt::Key_N));

    auto *dndAct = ac->addAction(QStringLiteral("toggle_dnd"), this, &MainWindow::slotToggleDoNotDisturb);
    dndAct->setText(i18n("&Do Not Disturb"));
    dndAct->setIcon(QIcon::fromTheme(QStringLiteral("notification-inactive")));
    dndAct->setCheckable(true);

    auto *clearAct = ac->addAction(QStringLiteral("clear_cache"), this, &MainWindow::slotClearCache);
    clearAct->setText(i18n("Clear &Cache"));
    clearAct->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));

    // ── Global shortcut: Show/hide window from anywhere ───────────────────────
    auto *globalShowAct = ac->addAction(QStringLiteral("global_show_window"));
    globalShowAct->setText(i18n("Show/Hide KWazzup"));
    globalShowAct->setIcon(QIcon::fromTheme(QStringLiteral("kwazzup")));
    KGlobalAccel::setGlobalShortcut(globalShowAct, QKeySequence(Qt::META | Qt::Key_W));
    connect(globalShowAct, &QAction::triggered, this, [this]() {
        if (isVisible() && !isMinimized())
            hide();
        else
            bringToFront();
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Settings persistence
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::loadSettings()
{
    KConfigGroup grp(KSharedConfig::openConfig(), QLatin1String(kCfgGroup));
    m_closeToTray     = grp.readEntry(QStringLiteral("CloseToTray"), true);

    KConfigGroup notifGrp(KSharedConfig::openConfig(), QStringLiteral("Notifications"));
    m_doNotDisturb = notifGrp.readEntry(QStringLiteral("DoNotDisturb"), false);

    NotificationBridge *bridge = m_page->notificationBridge();
    bridge->setDoNotDisturb(m_doNotDisturb);
    bridge->setEnabled(notifGrp.readEntry(QStringLiteral("EnableNotifications"), true));
    bridge->setSoundEnabled(notifGrp.readEntry(QStringLiteral("NotificationSound"), true));

    const int zoom = grp.readEntry(QStringLiteral("ZoomFactor"), 100);
    m_zoomFactor   = zoom / 100.0;
    m_webView->setZoomFactor(m_zoomFactor);

    const QString ua = grp.readEntry(QStringLiteral("UserAgent"), QString());
    if (!ua.isEmpty())
        m_page->profile()->setHttpUserAgent(ua);

    const QString lang = grp.readEntry(QStringLiteral("SpellCheckLanguage"), QString());
    if (!lang.isEmpty())
        m_page->profile()->setSpellCheckLanguages({lang});

    if (grp.readEntry(QStringLiteral("StartMinimized"), false))
        QTimer::singleShot(0, this, [this]() { hide(); });
}

void MainWindow::saveSettings()
{
    KConfigGroup grp(KSharedConfig::openConfig(), QLatin1String(kCfgGroup));
    grp.writeEntry(QStringLiteral("CloseToTray"), m_closeToTray);
    grp.writeEntry(QStringLiteral("ZoomFactor"),  qRound(m_zoomFactor * 100));

    KConfigGroup notifGrp(KSharedConfig::openConfig(), QStringLiteral("Notifications"));
    notifGrp.writeEntry(QStringLiteral("DoNotDisturb"), m_doNotDisturb);
    KSharedConfig::openConfig()->sync();
}

// ─────────────────────────────────────────────────────────────────────────────
// Theme integration
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::applyColorScheme()
{
    // WhatsApp Web respects prefers-color-scheme; Qt WebEngine passes the
    // application's Qt::ColorScheme through automatically (Qt 6.5+).
    // Nothing extra needed — palette changes will be picked up by the engine.
}

// ─────────────────────────────────────────────────────────────────────────────
// WebEngine signal handlers
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onTitleChanged(const QString &title)
{
    const int unread = Utils::parseUnreadCount(title);
    m_tray->setUnreadCount(unread);

    // Strip the count prefix for the window title
    QString clean = title;
    static const QRegularExpression re(QStringLiteral(R"(\(\d+\)\s*)"));
    clean.remove(re);
    setWindowTitle(clean.isEmpty() ? i18n("KWazzup") : clean);
}

void MainWindow::onIconChanged(const QIcon &icon)
{
    if (!icon.isNull())
        setWindowIcon(icon);
}

// ─────────────────────────────────────────────────────────────────────────────
// Download manager
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onDownloadRequested(QWebEngineDownloadRequest *download)
{
    // Ask the user where to save the file
    const QString defaultDir =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString suggestedName = download->suggestedFileName();
    const QString path = QFileDialog::getSaveFileName(
        this,
        i18n("Save File"),
        QDir(defaultDir).filePath(suggestedName));

    if (path.isEmpty()) {
        download->cancel();
        return;
    }

    download->setDownloadDirectory(QFileInfo(path).absolutePath());
    download->setDownloadFileName(QFileInfo(path).fileName());
    download->accept();
}

// ─────────────────────────────────────────────────────────────────────────────
// Action slots
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::slotReload()
{
    m_webView->reload();
}

void MainWindow::slotZoomIn()
{
    m_zoomFactor = qMin(m_zoomFactor + 0.1, 2.0);
    m_webView->setZoomFactor(m_zoomFactor);
}

void MainWindow::slotZoomOut()
{
    m_zoomFactor = qMax(m_zoomFactor - 0.1, 0.5);
    m_webView->setZoomFactor(m_zoomFactor);
}

void MainWindow::slotZoomReset()
{
    m_zoomFactor = 1.0;
    m_webView->setZoomFactor(m_zoomFactor);
}

void MainWindow::slotNewChat()
{
    openNewChat();
}

void MainWindow::slotToggleFullscreen()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}

void MainWindow::slotToggleDoNotDisturb()
{
    m_doNotDisturb = !m_doNotDisturb;
    m_page->notificationBridge()->setDoNotDisturb(m_doNotDisturb);
    if (auto *act = actionCollection()->action(QStringLiteral("toggle_dnd")))
        act->setChecked(m_doNotDisturb);
}

void MainWindow::slotClearCache()
{
    m_page->profile()->clearAllVisitedLinks();
    m_page->profile()->clearHttpCache();
}

void MainWindow::slotShowSettings()
{
    if (auto *dlg = SettingsDialog::showDialog(m_page->profile(), this))
        connect(dlg, &SettingsDialog::settingsChanged, this, &MainWindow::loadSettings);
}

void MainWindow::slotCloseWindow()
{
    // "Close window" (Ctrl+W, KStandardAction::close) hides to tray when
    // close-to-tray is enabled, or quits when it is not.
    if (m_closeToTray)
        hide();
    else
        qApp->quit();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::bringToFront()
{
    show();
    setWindowState(windowState() & ~Qt::WindowMinimized);
    raise();
    activateWindow();
    KWindowSystem::activateWindow(windowHandle());
}

void MainWindow::openNewChat(const QString &phoneNumber)
{
    bringToFront();
    const QString url = phoneNumber.isEmpty()
        ? QStringLiteral("https://web.whatsapp.com/#new-chat")
        : QStringLiteral("https://wa.me/%1").arg(phoneNumber);
    // Run JS to open the new-chat panel (WhatsApp Web API)
    const QString js = QStringLiteral(
        "window.location.href = '%1';").arg(url);
    m_page->runJavaScript(js);
}

// ─────────────────────────────────────────────────────────────────────────────
// Event handling
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange)
        applyColorScheme();
    KXmlGuiWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_closeToTray) {
        hide();
        event->ignore();
    } else {
        saveSettings();
        qApp->quit();
    }
}

