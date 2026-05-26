# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

add_library(pws_reload STATIC
    PlasmaReloader.cpp
)

target_link_libraries(pws_reload
    PUBLIC
        Qt6::Core
        Qt6::DBus
)

target_include_directories(pws_reload PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
