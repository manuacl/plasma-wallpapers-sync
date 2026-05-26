# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.22)

project(plasma-wallpapers-sync
    VERSION 0.1.0
    DESCRIPTION "Unified GUI to view and sync Plasma 6 wallpapers across desktop, lock, login and splash"
    HOMEPAGE_URL "https://github.com/manuacl/plasma-wallpapers-sync"
    LANGUAGES CXX
)

set(QT_MIN_VERSION  "6.6.0")
set(KF6_MIN_VERSION "6.0.0")

# Single source of truth for the app/D-Bus/desktop id.
# Will flip to org.kde.plasmawallpapersync if/when the project is
# accepted into KDE proper after Review Board.
set(APP_ID "dev.manuacl.plasmawallpapersync")

# --- Extra-CMake-Modules ----------------------------------------------------
find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQmlModule)
include(FeatureSummary)

# --- Qt & KF6 ---------------------------------------------------------------
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    DBus
    Test
)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Config
    CoreAddons
    I18n
    Kirigami
    Auth
)

# --- Subdirectories ---------------------------------------------------------
# Wired up as layers materialize. Each subdir owns its own CMakeLists.txt
# so the layering rule (see CLAUDE.md) stays visible in the build graph:
# core/ must not link KAuth/Kirigami/DBus.
#
# add_subdirectory(src/core)
# add_subdirectory(src/privileged/kauth)
# add_subdirectory(src/reload)
# add_subdirectory(src/helper)
# add_subdirectory(src/shell/standalone)
# add_subdirectory(src/qml)
# add_subdirectory(data)
# add_subdirectory(po)

enable_testing()
# add_subdirectory(tests)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
