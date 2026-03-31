#include "webenginepage.h"
#include "notificationbridge.h"
#include "utils.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDesktopServices>
#include <QFileDialog>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>
#include <QWebEngineDownloadRequest>
#include <QFile>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
// JS injected at DocumentCreation to override window.Notification
// and wire it to the C++ NotificationBridge via QWebChannel.
// ─────────────────────────────────────────────────────────────────────────────
static const char s_bridgeSetupJs[] = R"JS(
(function() {
    "use strict";

    // qwebchannel.js is injected separately (DocumentCreation, before this runs)
    function setupBridge(bridge) {
        // ── Notification API override ──────────────────────────────────────
        var OrigNotification = window.Notification;

        function KWazzupNotification(title, options) {
            options = options || {};
            var body    = options.body  || "";
            var icon    = options.icon  || "";
            var tag     = options.tag   || "";
            bridge.showNotification(title, body, icon, tag);

            // Keep a close() stub so WhatsApp's internal code doesn't break
            this.close = function() { bridge.closeNotification(tag); };
            this.addEventListener = function() {};
            this.removeEventListener = function() {};
            this.dispatchEvent = function() { return true; };
        }

        Object.defineProperty(KWazzupNotification, "permission", {
            get: function() { return "granted"; },
            configurable: true
        });

        KWazzupNotification.requestPermission = function(callback) {
            var result = Promise.resolve("granted");
            if (typeof callback === "function") callback("granted");
            return result;
        };

        window.Notification = KWazzupNotification;

        // ── Unread badge via title observation ────────────────────────────
        var lastCount = 0;
        var titleObserver = new MutationObserver(function() {
            var title = document.title;
            var m = title.match(/\((\d+)\)/);
            var count = m ? parseInt(m[1], 10) : 0;
            if (count !== lastCount) {
                lastCount = count;
                bridge.updateUnreadCount(count);
            }
        });

        titleObserver.observe(document.querySelector("title") || document.head, {
            subtree: true,
            characterData: true,
            childList: true
        });
    }

    // Wait for QWebChannel transport to become available
    function waitForChannel(retries) {
        if (typeof QWebChannel !== "undefined" && typeof qt !== "undefined" && qt.webChannelTransport) {
            new QWebChannel(qt.webChannelTransport, function(channel) {
                if (channel.objects && channel.objects.kwazzupBridge) {
                    setupBridge(channel.objects.kwazzupBridge);
                }
            });
        } else if (retries > 0) {
            setTimeout(function() { waitForChannel(retries - 1); }, 100);
        }
    }

    waitForChannel(20);
})();
)JS";

// ─────────────────────────────────────────────────────────────────────────────

WebEnginePage::WebEnginePage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
{
    // Chrome-like UA so WhatsApp Web doesn't fall back to mobile/unsupported UI
    setUrl(QUrl(QStringLiteral("about:blank")));
    profile->setHttpUserAgent(Utils::defaultUserAgent());

    // Grant all media/notification permissions automatically via the Qt 6.8+ API
    connect(this, &QWebEnginePage::permissionRequested,
            this, [](QWebEnginePermission permission) {
                permission.grant();
            });

    connect(this, &QWebEnginePage::loadFinished,
            this, &WebEnginePage::onLoadFinished);

    // Forward download requests to the main window's download manager
    connect(profile, &QWebEngineProfile::downloadRequested,
            this, &WebEnginePage::downloadRequested);

    setupWebChannel();

    // Page settings
    auto *s = settings();
    s->setAttribute(QWebEngineSettings::JavascriptEnabled,           true);
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled,         true);
    s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    s->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
    s->setAttribute(QWebEngineSettings::ScreenCaptureEnabled,        true);
    s->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly,  false);
    s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,       true);
}

WebEnginePage::~WebEnginePage() = default;

// ─────────────────────────────────────────────────────────────────────────────

void WebEnginePage::setupWebChannel()
{
    m_bridge  = new NotificationBridge(this);
    m_channel = new QWebChannel(this);
    m_channel->registerObject(QStringLiteral("kwazzupBridge"), m_bridge);
    setWebChannel(m_channel);

    // Inject qwebchannel.js (from Qt resources) at document creation
    QFile f(QStringLiteral(":/qtwebchannel/qwebchannel.js"));
    if (f.open(QIODevice::ReadOnly)) {
        QWebEngineScript qwcScript;
        qwcScript.setName(QStringLiteral("kwazzup_qwebchannel"));
        qwcScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
        qwcScript.setWorldId(QWebEngineScript::MainWorld);
        qwcScript.setSourceCode(QString::fromUtf8(f.readAll()));
        scripts().insert(qwcScript);
    } else {
        qWarning() << "[KWazzup] Could not open :/qtwebchannel/qwebchannel.js";
    }

    // Inject the bridge setup script at DocumentReady (after DOM is available)
    QWebEngineScript bridgeScript;
    bridgeScript.setName(QStringLiteral("kwazzup_bridge"));
    bridgeScript.setInjectionPoint(QWebEngineScript::DocumentReady);
    bridgeScript.setWorldId(QWebEngineScript::MainWorld);
    bridgeScript.setSourceCode(QString::fromUtf8(s_bridgeSetupJs));
    scripts().insert(bridgeScript);
}

// ─────────────────────────────────────────────────────────────────────────────


bool WebEnginePage::acceptNavigationRequest(const QUrl &url,
                                            NavigationType /*type*/,
                                            bool isMainFrame)
{
    if (!isMainFrame)
        return true;

    const QString host = url.host();

    // Allow WhatsApp Web domains
    if (host.endsWith(QStringLiteral("web.whatsapp.com")) ||
        host.endsWith(QStringLiteral("whatsapp.com")) ||
        host.endsWith(QStringLiteral("whatsapp.net")) ||
        url.scheme() == QStringLiteral("about") ||
        url.scheme() == QStringLiteral("data") ||
        url.scheme() == QStringLiteral("blob")) {
        return true;
    }

    // Open anything else in the system's default browser
    QDesktopServices::openUrl(url);
    return false;
}

QWebEnginePage *WebEnginePage::createWindow(WebWindowType /*type*/)
{
    // Instead of creating a popup, open external URLs in the system browser.
    // We return nullptr — Qt will call acceptNavigationRequest on the new URL
    // which will redirect to QDesktopServices::openUrl().
    return nullptr;
}

void WebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                             const QString &message,
                                             int lineNumber,
                                             const QString &sourceID)
{
    Q_UNUSED(sourceID)
    if (level == ErrorMessageLevel)
        qWarning() << "[JS]" << lineNumber << message;
}

void WebEnginePage::onLoadFinished(bool ok)
{
    if (!ok)
        qWarning() << "[KWazzup] Page load failed";
}
