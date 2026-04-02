#pragma once

#include <KXmlGuiWindow>

#include <QPointer>

class QWebEngineView;
class WebEnginePage;
class TrayManager;
class QWebEngineDownloadRequest;

/**
 * MainWindow
 *
 * The application's primary window — a KXmlGuiWindow loaded from
 * data/kwazzupui.rc.  It embeds a QWebEngineView that renders
 * WhatsApp Web and orchestrates all other subsystems:
 *
 *   - TrayManager    (KStatusNotifierItem)
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

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private Q_SLOTS:
    // WebEngine slots
    void onTitleChanged(const QString &title);
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
    void slotClearCache();
    void slotShowSettings();
    void slotCloseWindow();

private:
    void setupWebEngine();
    void setupActions();
    void applyColorScheme();
    void saveSettings();
    void loadSettings();

    // ── Widgets ──────────────────────────────────────────────────────────────
    QWebEngineView  *m_webView = nullptr;
    WebEnginePage   *m_page   = nullptr;
    TrayManager     *m_tray   = nullptr;

    // ── State ────────────────────────────────────────────────────────────────
    double  m_zoomFactor   = 1.0;
    bool    m_closeToTray  = true;
    bool    m_doNotDisturb = false;
};
