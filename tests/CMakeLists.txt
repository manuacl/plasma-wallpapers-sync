# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

# Unit tests for the core layer — display-server-free (QTEST_GUILESS_MAIN).

add_executable(tst_DesktopSurface unit/tst_DesktopSurface.cpp)

target_link_libraries(tst_DesktopSurface
    PRIVATE
        pws_core
        Qt6::Core
        Qt6::Test
        KF6::ConfigCore
)

target_compile_definitions(tst_DesktopSurface PRIVATE
    PWS_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/unit/fixtures"
)

add_test(NAME tst_DesktopSurface COMMAND tst_DesktopSurface)
