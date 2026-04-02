cmake_minimum_required(VERSION 3.20)
project(kwazzup VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# KDE ECM modules
find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

# Qt6 dependencies
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineWidgets
    WebEngineCore
    WebChannel
    DBus
    Network
)

# KDE Frameworks 6 — required
find_package(KF6 REQUIRED COMPONENTS
    CoreAddons       # KAboutData, KPluginFactory
    Crash            # KCrash automatic crash handler
    DBusAddons       # KDBusService
    I18n             # KLocalizedString / i18n()
    WidgetsAddons    # KMessageBox, KPasswordDialog, etc.
    IconThemes       # KIconLoader / KIconEngine
    Notifications    # KNotification, KNotificationAction
    ConfigWidgets    # KConfigDialog, KColorSchemeManager
    Config           # KConfig, KConfigGroup
    GlobalAccel      # KGlobalAccel
    StatusNotifierItem  # KStatusNotifierItem (plasma tray)
    WindowSystem     # KWindowSystem / KWindowInfo
    XmlGui           # KXmlGuiWindow, KActionCollection, KStandardAction
    TextWidgets      # KTextEdit (spell check)
)

# KWallet — optional (for lock password storage)
find_package(KF6Wallet)
set_package_properties(KF6Wallet PROPERTIES TYPE OPTIONAL
    DESCRIPTION "Secure password storage via KWallet"
    PURPOSE "Enables storing the app lock password in KWallet"
)

set(KWAZZUP_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/webenginepage.cpp
    src/notificationbridge.cpp
    src/traymanager.cpp
    src/settingsdialog.cpp
    src/utils.cpp
    resources.qrc
)

add_executable(kwazzup ${KWAZZUP_SOURCES})

target_link_libraries(kwazzup PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::WebChannel
    Qt6::DBus
    Qt6::Network
    KF6::CoreAddons
    KF6::Crash
    KF6::DBusAddons
    KF6::I18n
    KF6::WidgetsAddons
    KF6::IconThemes
    KF6::Notifications
    KF6::ConfigWidgets
    KF6::ConfigCore
    KF6::GlobalAccel
    KF6::StatusNotifierItem
    KF6::WindowSystem
    KF6::XmlGui
    KF6::TextWidgets
)

if(KF6Wallet_FOUND)
    target_link_libraries(kwazzup PRIVATE KF6::Wallet)
    target_compile_definitions(kwazzup PRIVATE HAVE_KWALLET)
endif()

# Embed version and app ID
target_compile_definitions(kwazzup PRIVATE
    KWAZZUP_VERSION="${PROJECT_VERSION}"
    QT_NO_KEYWORDS
)

# ── Installation ──────────────────────────────────────────────────────────────
install(TARGETS kwazzup DESTINATION ${KDE_INSTALL_BINDIR})

install(FILES data/org.kde.kwazzup.desktop
    DESTINATION ${KDE_INSTALL_APPDIR})

install(FILES data/kwazzup.notifyrc
    DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

install(FILES data/kwazzupui.rc
    DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/kwazzup)

install(FILES data/org.kde.kwazzup.appdata.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR})

install(FILES icons/kwazzup.svg
    DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

install(FILES data/kwazzup.kcfg
    DESTINATION ${KDE_INSTALL_KCFGDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# ── Packaging (CPack) ─────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "kwazzup")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Unofficial WhatsApp Web client for KDE Plasma")
set(CPACK_PACKAGE_CONTACT "https://github.com/xis/kwazzup")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Source tarball
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\.git/"
    "/build/"
    "\\.user$"
)

# Binary packages
set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")

# DEB
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6webenginewidgets6, libkf6notifications6, libkf6xmlgui6")

# RPM
set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")

include(CPack)
