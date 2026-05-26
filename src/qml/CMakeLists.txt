# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

# STATIC because the standalone shell statically links the plugin
# rather than loading it from an installed QML import path — keeps
# the Flatpak bundle a single binary and removes the runtime
# QML_IMPORT_PATH ordering footgun.
qt_add_qml_module(pws_qml
    URI "PlasmaWallpaperSync"
    VERSION 1.0
    STATIC
    QML_FILES
        Main.qml
        WallpaperPicker.qml
)
