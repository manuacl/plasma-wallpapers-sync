# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

# Standalone executable installed at KAUTH_HELPER_INSTALL_ABSOLUTE_DIR.
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

if(NOT KAUTH_HELPER_INSTALL_DIR)
    message(FATAL_ERROR
        "KAUTH_HELPER_INSTALL_DIR is empty — KF6Auth didn't expose it. "
        "Set it manually or upgrade KF6Auth.")
endif()

# --- Flatpak detection ---------------------------------------------------
set(_pws_is_flatpak_install OFF)
if(CMAKE_INSTALL_PREFIX STREQUAL "/app")
    set(_pws_is_flatpak_install ON)
    message(STATUS
        "Flatpak install detected — skipping helper binary + polkit policy "
        "installs. KAuth is not usable from inside a Flatpak sandbox anyway; "
        "see docs for the hybrid host install used during dev testing.")
endif()

# --- Helper binary --------------------------------------------------------
# KF6Auth's KAUTH_HELPER_INSTALL_DIR is relative ("lib/kf6/kauth"), so
# install() resolves it against CMAKE_INSTALL_PREFIX. When the prefix is
# not /usr (e.g. /usr/local), the binary lands at a path the D-Bus
# activation service can never reach — the .service stub hardcodes
# KAUTH_HELPER_INSTALL_ABSOLUTE_DIR ("/usr/lib/kf6/kauth") as Exec.
# Force the absolute path so binary and D-Bus Exec always agree.
if(NOT _pws_is_flatpak_install)
    install(TARGETS plasma-wallpaper-sync-helper
        DESTINATION "${KAUTH_HELPER_INSTALL_ABSOLUTE_DIR}"
    )
endif()

# --- D-Bus service + policy config ----------------------------------------
# We bypass kauth_install_helper_files() because it installs D-Bus files
# relative to CMAKE_INSTALL_PREFIX. When the prefix is not /usr, D-Bus
# can't find them — it only reads from /usr/share/dbus-1/. Generate the
# files from KAuth stubs (same as the macro) but install to the absolute
# system paths.
set(_pws_dbus_conf_dir "/usr/share/dbus-1/system.d")
set(_pws_dbus_svc_dir  "/usr/share/dbus-1/system-services")

# Variables consumed by the KAuth stubs (@HELPER_ID@, @HELPER_TARGET@, @HELPER_USER@).
set(HELPER_ID "${APP_ID}")
set(HELPER_TARGET "plasma-wallpaper-sync-helper")
set(HELPER_USER "root")

configure_file(
    "${KAUTH_STUB_FILES_DIR}/dbus_service.stub"
    "${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.service"
)
configure_file(
    "${KAUTH_STUB_FILES_DIR}/dbus_policy.stub"
    "${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.conf"
)

if(NOT _pws_is_flatpak_install)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.service"
        DESTINATION "${_pws_dbus_svc_dir}"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.conf"
        DESTINATION "${_pws_dbus_conf_dir}"
    )
endif()

# --- Polkit policy --------------------------------------------------------
# kauth_install_actions() runs kauth-policy-gen to transform the source
# XML. On some distros (CachyOS / KF6Auth 6.28 + polkit >= 124) the
# generator silently produces an empty <policyconfig/>. Detect this and
# fall back to installing the source file directly — it is already valid
# polkit policy XML.
set(_pws_policy_src "${CMAKE_SOURCE_DIR}/data/${APP_ID}.policy")

if(NOT _pws_is_flatpak_install)
    set(_pws_policy_generated "${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.policy")

    kauth_install_actions("${APP_ID}" "${_pws_policy_src}")

    # Check whether kauth-policy-gen produced a usable policy.
    # A broken generator emits only the XML/DOCTYPE wrapper with no
    # <action> elements — detect that and install the source directly.
    file(READ "${_pws_policy_generated}" _pws_policy_content)
    string(FIND "${_pws_policy_content}" "<action" _pws_has_actions)
    if(_pws_has_actions EQUAL -1)
        message(WARNING
            "kauth-policy-gen produced an empty policy — installing source "
            "file directly as fallback: ${_pws_policy_src}")
        install(FILES "${_pws_policy_src}"
            DESTINATION "${KAUTH_POLICY_FILES_INSTALL_DIR}"
        )
    endif()
endif()
