#include "settingsdialog.h"
#include "utils.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KMessageBox>

#include <QApplication>
#include <QStandardPaths>
#include <QFile>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDirIterator>
#include <QDir>
#include <QDialogButtonBox>
#include <QWebEngineProfile>

// ─────────────────────────────────────────────────────────────────────────────

SettingsDialog *SettingsDialog::showDialog(QWebEngineProfile *profile, QWidget *parent)
{
    // Ensure only one instance is open at a time
    for (auto *w : QApplication::topLevelWidgets()) {
        if (auto *dlg = qobject_cast<SettingsDialog *>(w)) {
            dlg->raise();
            dlg->activateWindow();
            return nullptr;
        }
    }
    auto *dlg = new SettingsDialog(profile, parent);
    dlg->show();
    return dlg;
}

SettingsDialog::SettingsDialog(QWebEngineProfile *profile, QWidget *parent)
    : KPageDialog(parent)
    , m_profile(profile)
{
    setWindowTitle(i18n("KWazzup Settings"));
    setFaceType(KPageDialog::List);
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    resize(540, 440);
    setAttribute(Qt::WA_DeleteOnClose);

    auto addIconPage = [this](QWidget *w, const QString &name, const QString &icon) {
        auto *item = addPage(w, name);
        item->setIcon(QIcon::fromTheme(icon));
    };
    addIconPage(makeGeneralPage(),       i18n("General"),       QStringLiteral("configure"));
    addIconPage(makeNotificationsPage(), i18n("Notifications"), QStringLiteral("notifications"));
    addIconPage(makePrivacyPage(),       i18n("Privacy"),       QStringLiteral("security-low"));

    loadSettings();

    connect(button(QDialogButtonBox::Ok),    &QPushButton::clicked, this, [this]() { saveSettings(); accept(); });
    connect(button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::saveSettings);
}

// ── General ──────────────────────────────────────────────────────────────────

QWidget *SettingsDialog::makeGeneralPage()
{
    auto *w   = new QWidget;
    auto *lay = new QFormLayout(w);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(10);

    m_closeToTray = new QCheckBox(i18n("Minimize to tray on close"), w);
    lay->addRow(i18n("Window:"), m_closeToTray);

    m_startMinimized = new QCheckBox(i18n("Start minimized to tray"), w);
    lay->addRow(QString(), m_startMinimized);

    m_autoStart = new QCheckBox(i18n("Launch at login"), w);
    lay->addRow(QString(), m_autoStart);

    m_spellLang = new QComboBox(w);
    m_spellLang->addItem(i18n("System default"), QString());
    const QStringList locales{
        QStringLiteral("en_US"), QStringLiteral("en_GB"),
        QStringLiteral("pl_PL"), QStringLiteral("de_DE"),
        QStringLiteral("fr_FR"), QStringLiteral("es_ES"),
        QStringLiteral("pt_BR"), QStringLiteral("ru_RU"),
    };
    for (const auto &loc : locales)
        m_spellLang->addItem(loc, loc);
    lay->addRow(i18n("Spell-check language:"), m_spellLang);

    m_zoomSpin = new QSpinBox(w);
    m_zoomSpin->setRange(50, 200);
    m_zoomSpin->setSingleStep(10);
    m_zoomSpin->setSuffix(QStringLiteral(" %"));
    lay->addRow(i18n("Default zoom:"), m_zoomSpin);

    return w;
}

// ── Notifications ─────────────────────────────────────────────────────────────

QWidget *SettingsDialog::makeNotificationsPage()
{
    auto *w   = new QWidget;
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(8);

    m_enableNotif = new QCheckBox(i18n("Enable notifications"), w);
    lay->addWidget(m_enableNotif);

    m_notifSound = new QCheckBox(i18n("Play notification sound"), w);
    lay->addWidget(m_notifSound);

    m_dnd = new QCheckBox(i18n("Do Not Disturb (suppress all notifications)"), w);
    lay->addWidget(m_dnd);

    lay->addStretch();
    return w;
}

// ── Privacy ───────────────────────────────────────────────────────────────────

QWidget *SettingsDialog::makePrivacyPage()
{
    auto *w   = new QWidget;
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(8);

    m_privateBrowsing = new QCheckBox(
        i18n("Private browsing (Off-the-Record) — do not store session data"), w);
    lay->addWidget(m_privateBrowsing);

    auto *uaBox = new QGroupBox(i18n("Custom User-Agent"), w);
    auto *uaLay = new QVBoxLayout(uaBox);
    m_uaEdit = new QLineEdit(uaBox);
    m_uaEdit->setPlaceholderText(Utils::defaultUserAgent());
    uaLay->addWidget(m_uaEdit);
    lay->addWidget(uaBox);

    auto *cacheBox = new QGroupBox(i18n("Cache"), w);
    auto *cacheLay = new QHBoxLayout(cacheBox);
    m_cacheSizeLabel = new QLabel(cacheBox);
    cacheLay->addWidget(m_cacheSizeLabel);
    cacheLay->addStretch();
    auto *clearBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-clear")),
                                     i18n("Clear cache"), cacheBox);
    cacheLay->addWidget(clearBtn);
    lay->addWidget(cacheBox);

    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        m_profile->clearAllVisitedLinks();
        m_profile->clearHttpCache();
        KMessageBox::information(this, i18n("Cache cleared."), i18n("KWazzup"));
        updateCacheSize();
    });

    updateCacheSize();
    lay->addStretch();
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────

void SettingsDialog::loadSettings()
{
    KConfigGroup gen(KSharedConfig::openConfig(), QStringLiteral("MainWindow"));
    m_closeToTray->setChecked(gen.readEntry(QStringLiteral("CloseToTray"),   true));
    m_startMinimized->setChecked(gen.readEntry(QStringLiteral("StartMinimized"), false));
    m_autoStart->setChecked(gen.readEntry(QStringLiteral("AutoStart"),       false));
    m_zoomSpin->setValue(gen.readEntry(QStringLiteral("ZoomFactor"),         100));
    const QString lang = gen.readEntry(QStringLiteral("SpellCheckLanguage"), QString());
    const int idx = m_spellLang->findData(lang);
    m_spellLang->setCurrentIndex(idx >= 0 ? idx : 0);
    m_uaEdit->setText(gen.readEntry(QStringLiteral("UserAgent"),             QString()));

    KConfigGroup notif(KSharedConfig::openConfig(), QStringLiteral("Notifications"));
    m_enableNotif->setChecked(notif.readEntry(QStringLiteral("EnableNotifications"), true));
    m_notifSound->setChecked(notif.readEntry(QStringLiteral("NotificationSound"),    true));
    m_dnd->setChecked(notif.readEntry(QStringLiteral("DoNotDisturb"),               false));

    KConfigGroup priv(KSharedConfig::openConfig(), QStringLiteral("Privacy"));
    m_privateBrowsing->setChecked(priv.readEntry(QStringLiteral("PrivateBrowsing"), false));

}

void SettingsDialog::saveSettings()
{
    KConfigGroup gen(KSharedConfig::openConfig(), QStringLiteral("MainWindow"));
    gen.writeEntry(QStringLiteral("CloseToTray"),        m_closeToTray->isChecked());
    gen.writeEntry(QStringLiteral("StartMinimized"),     m_startMinimized->isChecked());
    gen.writeEntry(QStringLiteral("AutoStart"),          m_autoStart->isChecked());
    gen.writeEntry(QStringLiteral("ZoomFactor"),         m_zoomSpin->value());
    gen.writeEntry(QStringLiteral("SpellCheckLanguage"), m_spellLang->currentData().toString());
    gen.writeEntry(QStringLiteral("UserAgent"),          m_uaEdit->text());

    KConfigGroup notif(KSharedConfig::openConfig(), QStringLiteral("Notifications"));
    notif.writeEntry(QStringLiteral("EnableNotifications"), m_enableNotif->isChecked());
    notif.writeEntry(QStringLiteral("NotificationSound"),   m_notifSound->isChecked());
    notif.writeEntry(QStringLiteral("DoNotDisturb"),        m_dnd->isChecked());

    KConfigGroup priv(KSharedConfig::openConfig(), QStringLiteral("Privacy"));
    priv.writeEntry(QStringLiteral("PrivateBrowsing"), m_privateBrowsing->isChecked());

    KSharedConfig::openConfig()->sync();

    // Spell check language — apply immediately to the profile
    {
        const QString lang = m_spellLang->currentData().toString();
        if (!lang.isEmpty())
            m_profile->setSpellCheckLanguages({lang});
    }

    // Autostart — create or remove ~/.config/autostart/org.kde.kwazzup.desktop
    {
        const QString autostartDir =
            QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
            + QStringLiteral("/autostart");
        const QString dst = autostartDir + QStringLiteral("/org.kde.kwazzup.desktop");
        if (m_autoStart->isChecked()) {
            const QString src = QStandardPaths::locate(
                QStandardPaths::ApplicationsLocation,
                QStringLiteral("org.kde.kwazzup.desktop"));
            if (!src.isEmpty()) {
                QDir().mkpath(autostartDir);
                QFile::copy(src, dst);
            }
        } else {
            QFile::remove(dst);
        }
    }

    Q_EMIT settingsChanged();
}

void SettingsDialog::updateCacheSize()
{
    if (!m_cacheSizeLabel) return;
    if (!m_profile) return;
    const QString path = m_profile->cachePath();
    qint64 total = 0;
    QDirIterator it(path, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isFile())
            total += it.fileInfo().size();
    }
    m_cacheSizeLabel->setText(i18n("Cache size: %1", Utils::formatBytes(total)));
}
