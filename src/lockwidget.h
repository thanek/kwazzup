#pragma once

#include <QWidget>
#include <QTimer>

class QLabel;
class QLineEdit;
class QPushButton;

#ifdef HAVE_KWALLET
namespace KWallet { class Wallet; }
#endif

/**
 * LockWidget
 *
 * Full-window overlay shown when the application is locked.
 * The password is stored in KWallet when available, otherwise in
 * an obfuscated KConfig entry (not secure — KWallet is strongly preferred).
 */
class LockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LockWidget(QWidget *parent = nullptr);
    ~LockWidget() override;

    bool isLocked() const { return m_locked; }

    // Saves a new lock password.  Pass an empty string to disable locking.
    void setPassword(const QString &password);

    // Returns true when a non-empty password has been set
    bool hasPassword() const;

public Q_SLOTS:
    void lock();
    void unlock();

Q_SIGNALS:
    void unlocked();
    void locked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void onUnlockClicked();

private:
    QString storedPassword() const;
    void savePassword(const QString &password);

    bool        m_locked = false;
    QLabel      *m_iconLabel   = nullptr;
    QLabel      *m_titleLabel  = nullptr;
    QLabel      *m_errorLabel  = nullptr;
    QLineEdit   *m_passEdit    = nullptr;
    QPushButton *m_unlockBtn   = nullptr;

#ifdef HAVE_KWALLET
    KWallet::Wallet *m_wallet  = nullptr;
    void openWallet();
#endif
};
