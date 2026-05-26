# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

add_library(pws_core STATIC
    WallpaperSurface.cpp
    DesktopSurface.cpp
    LockscreenSurface.cpp
)

target_link_libraries(pws_core
    PUBLIC
        Qt6::Core
        KF6::ConfigCore
)

target_include_directories(pws_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# --- Layering guard (plasma-isolation rule from CLAUDE.md) -------------------
# core/ MUST NOT link to GUI, Kirigami, KAuth or DBus libraries — those are
# the swappable layers that allow a future KCM port and Flatpak distribution.
# Fail the configure step if any forbidden dependency appears in LINK_LIBRARIES.
function(_pws_enforce_core_isolation target)
    get_target_property(_libs ${target} LINK_LIBRARIES)
    if(NOT _libs)
        return()
    endif()
    foreach(_lib ${_libs})
        if(_lib MATCHES "^(Qt6::Gui|Qt6::Widgets|Qt6::Quick|Qt6::QuickControls2|Qt6::Qml|Qt6::DBus|KF6::Auth|KF6::Kirigami)$")
            message(FATAL_ERROR
                "Layering violation: ${target} links '${_lib}', but core/ "
                "must stay GUI-free, Kirigami-free, KAuth-free and DBus-free. "
                "See CLAUDE.md plasma-isolation rule."
            )
        endif()
    endforeach()
endfunction()

_pws_enforce_core_isolation(pws_core)
