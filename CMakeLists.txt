cmake_minimum_required(VERSION 3.16)
project(lnotes VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Default install to ~/.local so `cmake --install build` puts the binary
# in ~/.local/bin/notes without needing sudo.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local"
        CACHE PATH "Install prefix" FORCE)
endif()

include(GNUInstallDirs)

add_executable(notes
    src/main.c
    src/utils.c
    src/notes.c
    src/search.c
)

target_include_directories(notes PRIVATE src)

target_compile_options(notes PRIVATE -Wall -Wextra -Wpedantic)

# clock_gettime and POSIX regex require _POSIX_C_SOURCE >= 200809L on Linux.
# macOS exposes them unconditionally so we only set it on non-Apple Unix.
if(UNIX AND NOT APPLE)
    target_compile_definitions(notes PRIVATE _POSIX_C_SOURCE=200809L)
endif()

install(TARGETS notes RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- Tests ----
enable_testing()

add_executable(test_notes tests/test_main.c src/utils.c src/notes.c)
target_include_directories(test_notes PRIVATE src)
target_compile_options(test_notes PRIVATE -Wall -Wextra)
if(UNIX AND NOT APPLE)
    target_compile_definitions(test_notes PRIVATE _POSIX_C_SOURCE=200809L)
endif()

add_test(NAME unit_tests COMMAND test_notes)
