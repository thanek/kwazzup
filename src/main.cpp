#include "mainwindow.h"

#include <KAboutData>
#include <KCrash>
#include <KLocalizedString>
#include <KDBusService>

#include <KSharedConfig>
#include <KConfigGroup>

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
    // If a second instance is launched it will activate the first one
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

    // ── Main window ──────────────────────────────────────────────────────────
    auto *window = new MainWindow;

    // Handle second-instance activation: KDBusService will call activate()
    QObject::connect(&service, &KDBusService::activateRequested,
                     window, [window](const QStringList &, const QString &) {
                         window->bringToFront();
                     });

    // Apply CLI options after the event loop starts
    QTimer::singleShot(0, window, [&parser, window]() {
        if (parser.isSet(QStringLiteral("lock"))) {
            window->lockApp();
        }
        if (parser.isSet(QStringLiteral("open-chat"))) {
            window->openNewChat(parser.value(QStringLiteral("open-chat")));
        }
        if (parser.isSet(QStringLiteral("show"))) {
            window->bringToFront();
        }
    });

    // Start minimised to tray if configured
    KConfigGroup grp(KSharedConfig::openConfig(), QStringLiteral("MainWindow"));
    if (!grp.readEntry(QStringLiteral("StartMinimized"), false))
        window->show();

    return app.exec();
}
