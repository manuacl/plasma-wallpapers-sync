# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

qt_add_executable(plasma-wallpaper-sync
    main.cpp
)

target_link_libraries(plasma-wallpaper-sync
    PRIVATE
        pws_core
        pws_reload
        pws_privileged_kauth
        pws_qml
        pws_qmlplugin
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
)

# Static QML plugins need an explicit import call to register at
# QQmlEngine construction time — without it the QML engine never
# sees the PlasmaWallpaperSync module.
qt_import_qml_plugins(plasma-wallpaper-sync)

install(TARGETS plasma-wallpaper-sync ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
