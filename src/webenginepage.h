#pragma once

#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebChannel>

class NotificationBridge;
class QWebEngineDownloadRequest;

/**
 * WebEnginePage
 *
 * Custom QWebEnginePage that:
 *  - uses a Chrome-like user agent (WhatsApp Web requires it)
 *  - grants camera / microphone / geolocation / notification permissions via
 *    KDE dialogs so the user is aware
 *  - intercepts new-window navigations and opens them in the system browser
 *  - bridges the JS Notification API to KNotification via QWebChannel
 *  - handles downloads through Qt's download system
 */
class WebEnginePage : public QWebEnginePage
{
    Q_OBJECT

public:
    explicit WebEnginePage(QWebEngineProfile *profile, QObject *parent = nullptr);
    ~WebEnginePage() override;

    NotificationBridge *notificationBridge() const { return m_bridge; }

Q_SIGNALS:
    void downloadRequested(QWebEngineDownloadRequest *download);

protected:
    bool acceptNavigationRequest(const QUrl &url,
                                 NavigationType type,
                                 bool isMainFrame) override;

    QWebEnginePage *createWindow(WebWindowType type) override;

    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override;

private Q_SLOTS:
    void onLoadFinished(bool ok);

private:
    void setupWebChannel();

    QWebChannel       *m_channel = nullptr;
    NotificationBridge *m_bridge = nullptr;
};
