#include "lockwidget.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>

#ifdef HAVE_KWALLET
#include <KWallet>
#endif

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPainter>
#include <QIcon>
#include <QKeyEvent>
#include <QCryptographicHash>

static constexpr char kWalletFolder[] = "KWazzup";
static constexpr char kWalletKey[]    = "lockPassword";
static constexpr char kConfigGroup[]  = "Lock";
static constexpr char kConfigKey[]    = "PasswordHash";

// ─────────────────────────────────────────────────────────────────────────────

LockWidget::LockWidget(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_StyledBackground);
    setObjectName(QStringLiteral("LockWidget"));
    setStyleSheet(QStringLiteral(
        "#LockWidget { background: rgba(0,0,0,180); }"));

    // ── Centre column layout ─────────────────────────────────────────────────
    auto *outer = new QVBoxLayout(this);
    outer->setAlignment(Qt::AlignCenter);

    auto *card = new QWidget(this);
    card->setObjectName(QStringLiteral("LockCard"));
    card->setStyleSheet(QStringLiteral(
        "#LockCard { background: palette(window); border-radius: 12px; }"));
    card->setFixedWidth(320);

    auto *col = new QVBoxLayout(card);
    col->setSpacing(12);
    col->setContentsMargins(32, 32, 32, 32);

    // App icon
    m_iconLabel = new QLabel(card);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("kwhatsapp")).pixmap(64, 64));
    col->addWidget(m_iconLabel);

    // Title
    m_titleLabel = new QLabel(i18n("KWazzup is locked"), card);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont tf = m_titleLabel->font();
    tf.setPointSizeF(tf.pointSizeF() * 1.3);
    tf.setBold(true);
    m_titleLabel->setFont(tf);
    col->addWidget(m_titleLabel);

    // Password field
    m_passEdit = new QLineEdit(card);
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText(i18n("Enter password…"));
    col->addWidget(m_passEdit);

    // Error label (hidden by default)
    m_errorLabel = new QLabel(card);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet(QStringLiteral("color: red;"));
    m_errorLabel->hide();
    col->addWidget(m_errorLabel);

    // Unlock button
    m_unlockBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("unlock")),
                                  i18n("Unlock"), card);
    m_unlockBtn->setDefault(true);
    col->addWidget(m_unlockBtn);

    outer->addWidget(card);

    connect(m_unlockBtn, &QPushButton::clicked, this, &LockWidget::onUnlockClicked);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LockWidget::onUnlockClicked);

    hide();
}

LockWidget::~LockWidget() = default;

// ─────────────────────────────────────────────────────────────────────────────

void LockWidget::lock()
{
    if (!hasPassword()) return;
    m_locked = true;
    m_passEdit->clear();
    m_errorLabel->hide();
    show();
    raise();
    activateWindow();
    m_passEdit->setFocus();
    Q_EMIT locked();
}

void LockWidget::unlock()
{
    m_locked = false;
    hide();
    Q_EMIT unlocked();
}

void LockWidget::onUnlockClicked()
{
    const QString entered = m_passEdit->text();
    if (entered.isEmpty()) {
        m_errorLabel->setText(i18n("Please enter your password."));
        m_errorLabel->show();
        return;
    }

    const QString stored = storedPassword();
    // Compare SHA-256 hashes
    const QByteArray enteredHash =
        QCryptographicHash::hash(entered.toUtf8(), QCryptographicHash::Sha256).toHex();

    if (enteredHash == stored.toLatin1()) {
        m_errorLabel->hide();
        m_passEdit->clear();
        unlock();
    } else {
        m_errorLabel->setText(i18n("Wrong password. Try again."));
        m_errorLabel->show();
        m_passEdit->selectAll();
        m_passEdit->setFocus();
    }
}

// ─────────────────────────────────────────────────────────────────────────────

bool LockWidget::hasPassword() const
{
    return !storedPassword().isEmpty();
}

QString LockWidget::storedPassword() const
{
#ifdef HAVE_KWALLET
    if (KWallet::Wallet::isEnabled()) {
        // Try to retrieve from KWallet synchronously (best-effort)
        if (auto *w = KWallet::Wallet::openWallet(
                KWallet::Wallet::NetworkWallet(),
                parentWidget() ? parentWidget()->winId() : 0,
                KWallet::Wallet::Synchronous)) {
            w->setFolder(QLatin1String(kWalletFolder));
            QString pass;
            if (w->readPassword(QLatin1String(kWalletKey), pass) == 0)
                return pass;
        }
    }
#endif
    // Fallback: read SHA-256 hash from KConfig (not secret but prevents plain-text)
    KConfigGroup grp(KSharedConfig::openConfig(), QLatin1String(kConfigGroup));
    return grp.readEntry(QLatin1String(kConfigKey), QString());
}

void LockWidget::setPassword(const QString &password)
{
    const QByteArray hash =
        QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();

#ifdef HAVE_KWALLET
    if (KWallet::Wallet::isEnabled()) {
        if (auto *w = KWallet::Wallet::openWallet(
                KWallet::Wallet::NetworkWallet(),
                parentWidget() ? parentWidget()->winId() : 0,
                KWallet::Wallet::Synchronous)) {
            w->setFolder(QLatin1String(kWalletFolder));
            if (password.isEmpty())
                w->removeEntry(QLatin1String(kWalletKey));
            else
                w->writePassword(QLatin1String(kWalletKey), QString::fromLatin1(hash));
            return;
        }
    }
#endif
    KConfigGroup grp(KSharedConfig::openConfig(), QLatin1String(kConfigGroup));
    if (password.isEmpty())
        grp.deleteEntry(QLatin1String(kConfigKey));
    else
        grp.writeEntry(QLatin1String(kConfigKey), QString::fromLatin1(hash));
    grp.sync();
}

// ─────────────────────────────────────────────────────────────────────────────

void LockWidget::paintEvent(QPaintEvent *event)
{
    // Fill with a semi-transparent dark overlay
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 180));
    QWidget::paintEvent(event);
}

void LockWidget::keyPressEvent(QKeyEvent *event)
{
    // Swallow all key events while locked (prevent shortcuts bypassing lock)
    if (event->key() == Qt::Key_Escape)
        return; // ignore Escape
    QWidget::keyPressEvent(event);
}

void LockWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}
