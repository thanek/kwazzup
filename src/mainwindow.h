#pragma once

#include <KXmlGuiWindow>

#include <QTimer>
#include <QPointer>

class QWebEngineView;
class WebEnginePage;
class TrayManager;
class LockWidget;
class QWebEngineDownloadRequest;
class QProgressBar;
class QLabel;
class QSlider;

/**
 * MainWindow
 *
 * The application's primary window — a KXmlGuiWindow loaded from
 * data/kwhatsappui.rc.  It embeds a QWebEngineView that renders
 * WhatsApp Web and orchestrates all other subsystems:
 *
 *   - TrayManager    (KStatusNotifierItem)
 *   - LockWidget     (fullscreen overlay)
 *   - KGlobalAccel   (global keyboard shortcuts)
 *   - KConfig        (settings persistence)
 *   - KNotification  (via NotificationBridge inside WebEnginePage)
 *   - Download manager (KIO-based save dialog)
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Bring the window to front (called from tray / notification click)
    void bringToFront();

    // Navigate WhatsApp Web to a new-chat URL
    void openNewChat(const QString &phoneNumber = {});

    // Lock the application immediately
    void lockApp();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    // WebEngine slots
    void onTitleChanged(const QString &title);
    void onLoadStarted();
    void onLoadProgress(int progress);
    void onLoadFinished(bool ok);
    void onIconChanged(const QIcon &icon);

    // Download
    void onDownloadRequested(QWebEngineDownloadRequest *download);

    // UI actions (wired to KActionCollection)
    void slotReload();
    void slotZoomIn();
    void slotZoomOut();
    void slotZoomReset();
    void slotNewChat();
    void slotToggleFullscreen();
    void slotToggleDoNotDisturb();
    void slotLockApp();
    void slotClearCache();
    void slotShowSettings();
    void slotAbout();
    void slotCloseWindow();

    // Auto-lock
    void onAutoLockTimeout();
    void resetAutoLockTimer();

private:
    void setupWebEngine();
    void setupActions();
    void setupStatusBar();
    void setupAutoLock();
    void applyColorScheme();
    void saveSettings();
    void loadSettings();

    // ── Widgets ──────────────────────────────────────────────────────────────
    QWebEngineView  *m_webView    = nullptr;
    WebEnginePage   *m_page       = nullptr;
    TrayManager     *m_tray       = nullptr;
    LockWidget      *m_lockWidget = nullptr;

    // Status-bar widgets
    QProgressBar    *m_progressBar = nullptr;
    QLabel          *m_zoomLabel   = nullptr;

    // ── State ────────────────────────────────────────────────────────────────
    double  m_zoomFactor      = 1.0;
    bool    m_closeToTray     = true;
    bool    m_doNotDisturb    = false;
    int     m_autoLockMinutes = 0;    // 0 = disabled
    QTimer  m_autoLockTimer;
};
