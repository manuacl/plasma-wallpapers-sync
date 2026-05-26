# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0

# Unit tests for the core layer — display-server-free (QTEST_GUILESS_MAIN).

# Extra Q_OBJECT-bearing headers (fakes/mocks) must be passed in so
# AUTOMOC discovers them and emits the corresponding vtables.
function(pws_add_core_test name)
    add_executable(${name} unit/${name}.cpp ${ARGN})
    target_link_libraries(${name}
        PRIVATE
            pws_core
            Qt6::Core
            Qt6::Test
            KF6::ConfigCore
    )
    target_compile_definitions(${name} PRIVATE
        PWS_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/unit/fixtures"
    )
    add_test(NAME ${name} COMMAND ${name})
endfunction()

pws_add_core_test(tst_DesktopSurface)
pws_add_core_test(tst_LockscreenSurface)
pws_add_core_test(tst_LoginSurface unit/FakePrivilegedWriter.h)
