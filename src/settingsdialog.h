#pragma once

#include <KPageDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QLineEdit;
class QLabel;
class QPushButton;
class LockWidget;
class QWebEngineProfile;

/**
 * SettingsDialog
 *
 * Multi-page settings dialog (KPageDialog) with four pages:
 *   General       – startup, close-to-tray, spell check language, zoom
 *   Notifications – enable, DND, sound
 *   Privacy       – OTR, clear cache, UA override
 *   Security      – app lock password, auto-lock timeout
 */
class SettingsDialog : public KPageDialog
{
    Q_OBJECT

public:
    // Returns the dialog (new or existing).  Returns nullptr only if an existing
    // dialog was raised instead of creating a new one.
    static SettingsDialog *showDialog(QWebEngineProfile *profile, LockWidget *lockWidget, QWidget *parent = nullptr);

Q_SIGNALS:
    void settingsChanged();

private:
    explicit SettingsDialog(QWebEngineProfile *profile, LockWidget *lockWidget, QWidget *parent = nullptr);

    QWidget *makeGeneralPage();
    QWidget *makeNotificationsPage();
    QWidget *makePrivacyPage();
    QWidget *makeSecurityPage();

    void loadSettings();
    void saveSettings();
    void updateCacheSize();

    QCheckBox  *m_closeToTray     = nullptr;
    QCheckBox  *m_startMinimized  = nullptr;
    QCheckBox  *m_autoStart       = nullptr;
    QComboBox  *m_spellLang       = nullptr;
    QSpinBox   *m_zoomSpin        = nullptr;

    QCheckBox  *m_enableNotif     = nullptr;
    QCheckBox  *m_notifSound      = nullptr;
    QCheckBox  *m_dnd             = nullptr;

    QCheckBox  *m_privateBrowsing = nullptr;
    QLineEdit  *m_uaEdit          = nullptr;
    QLabel     *m_cacheSizeLabel  = nullptr;

    QCheckBox  *m_lockEnabled     = nullptr;
    QLineEdit  *m_passEdit        = nullptr;
    QLineEdit  *m_passConfirm     = nullptr;
    QSpinBox   *m_autoLockSpin    = nullptr;

    LockWidget         *m_lockWidget = nullptr;
    QWebEngineProfile  *m_profile    = nullptr;
};
