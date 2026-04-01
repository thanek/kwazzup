#include "mainwindow.h"

#include <KAboutData>
#include <KCrash>
#include <KLocalizedString>
#include <KDBusService>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KWindowSystem>

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QTimer>

int main(int argc, char *argv[])
{
    // ── Qt application ───────────────────────────────────────────────────────
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kwazzup"));
    app.setOrganizationName(QStringLiteral("KDE"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    // Make the bundled icon available via QIcon::fromTheme() even before
    // the application is installed into the system icon theme.
    QIcon::setFallbackSearchPaths(QIcon::fallbackSearchPaths() << QStringLiteral(":/"));

    // ── KAboutData ───────────────────────────────────────────────────────────
    KAboutData aboutData(
        QStringLiteral("kwazzup"),                      // component name
        i18n("KWazzup"),                                // display name
        QStringLiteral(KWAZZUP_VERSION),                // version
        i18n("An unofficial WhatsApp client for KDE Plasma"), // short description
        KAboutLicense::GPL_V3,
        i18n("© 2024 KWazzup contributors"),
        QString(),                                      // other text
        QStringLiteral("https://github.com/thanek/kwazzup"),
        QStringLiteral("https://github.com/thanek/kwazzup/issues")
    );
    aboutData.addAuthor(
        i18n("KWazzup Contributors"),
        i18n("Development"),
        QString());
    aboutData.setProductName("kwazzup");
    aboutData.setDesktopFileName(QStringLiteral("org.kde.kwazzup"));

    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kwazzup")));

    // ── Crash handler ────────────────────────────────────────────────────────
    KCrash::initialize();
    KCrash::setFlags(KCrash::AutoRestart);

    // ── Single-instance enforcement via D-Bus ────────────────────────────────
    // Registered early so any concurrent second instance exits before
    // allocating heavy resources (WebEngine profile).
    KDBusService service(KDBusService::Unique);

    // ── Command-line options ─────────────────────────────────────────────────
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    QCommandLineOption showOpt(
        QStringLiteral("show"),
        i18n("Bring the main window to the front"));
    parser.addOption(showOpt);

    QCommandLineOption lockOpt(
        QStringLiteral("lock"),
        i18n("Lock the application immediately"));
    parser.addOption(lockOpt);

    QCommandLineOption openChatOpt(
        QStringList{QStringLiteral("open-chat"), QStringLiteral("c")},
        i18n("Open a chat with the given phone number (international format)"),
        QStringLiteral("phone"));
    parser.addOption(openChatOpt);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // ── D-Bus activation handler ─────────────────────────────────────────────
    // window may still be null when the first activateRequested fires:
    // D-Bus delivers the Activate() call as soon as the event loop starts,
    // which is before the deferred MainWindow construction completes.
    // In that case we only set the XDG token; the window will be shown
    // normally by the QTimer below.  For subsequent activations (dock click
    // while already running) window is non-null and bringToFront() is called.
    MainWindow *window = nullptr;

    QObject::connect(&service, &KDBusService::activateRequested,
                     qApp, [&window](const QStringList &, const QString &startupId) {
                         // Honour the XDG activation token so Wayland grants
                         // focus.  Must be set before activateWindow().
                         if (!startupId.isEmpty())
                             KWindowSystem::setCurrentXdgActivationToken(startupId);
                         if (window)
                             window->bringToFront();
                     });

    // ── Main window (deferred) ───────────────────────────────────────────────
    // MainWindow initialises QtWebEngine, which blocks the main thread for
    // several seconds.  Deferring to the first event-loop tick lets
    // KDBusService reply to any pending D-Bus Activate() call immediately
    // on startup, preventing the "Did not receive a reply" timeout that
    // the Plasma launcher reports when DBusActivatable=true is set.
    //
    // Note: socket notifiers (D-Bus) have higher priority than zero-timers
    // in Qt's event loop, so the Activate() reply is dispatched before the
    // window is created.
    QTimer::singleShot(0, qApp, [&window, &parser]() {
        window = new MainWindow;

        if (parser.isSet(QStringLiteral("lock")))
            window->lockApp();
        if (parser.isSet(QStringLiteral("open-chat")))
            window->openNewChat(parser.value(QStringLiteral("open-chat")));
        if (parser.isSet(QStringLiteral("show")))
            window->bringToFront();

        KConfigGroup grp(KSharedConfig::openConfig(), QStringLiteral("MainWindow"));
        if (!grp.readEntry(QStringLiteral("StartMinimized"), false))
            window->show();
    });

    return app.exec();
}
