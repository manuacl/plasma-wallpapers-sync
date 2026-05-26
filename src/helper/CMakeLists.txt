# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

# Standalone executable installed at $KAUTH_HELPER_INSTALL_DIR
# (typically /usr/libexec/kauth or /usr/libexec depending on distro).
# Polkit's D-Bus service activation invokes it on demand when the
# client (KAuthPrivilegedWriter in this project) calls KAuth::Action.
add_executable(plasma-wallpaper-sync-helper
    PlasmaLoginHelper.cpp
    PlasmaLoginHelper.h
)

target_link_libraries(plasma-wallpaper-sync-helper
    PRIVATE
        Qt6::Core
        Qt6::DBus
        KF6::AuthCore
)

# kauth_install_helper_files generates the D-Bus .service file for
# org.kde.kf6auth.<action_prefix>, installs it under
# $DBUS_INTERFACES_INSTALL_DIR/system-services/, and installs the
# helper binary at the right path. "root" is the user the helper
# runs as.
kauth_install_helper_files(plasma-wallpaper-sync-helper
    "${APP_ID}"
    "root"
)

# Install the polkit policy XML. The action id inside it
# ("dev.manuacl.plasmawallpapersync.writefile") must match what
# KAuthPrivilegedWriter passes to KAuth::Action.
kauth_install_actions(
    "${APP_ID}"
    "${CMAKE_SOURCE_DIR}/data/${APP_ID}.policy"
)
