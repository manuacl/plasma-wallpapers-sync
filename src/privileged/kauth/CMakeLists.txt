# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

add_library(pws_privileged_kauth STATIC
    KAuthPrivilegedWriter.cpp
)

target_link_libraries(pws_privileged_kauth
    PUBLIC
        pws_core
        Qt6::Core
        KF6::AuthCore
)

target_include_directories(pws_privileged_kauth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
