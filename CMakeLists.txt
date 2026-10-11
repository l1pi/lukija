cmake_minimum_required(VERSION 3.16)
project(lukija VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SAILFISHAPP QUIET IMPORTED_TARGET sailfishapp)

option(LUKIJA_WITH_PIPER "Build the Piper TTS engine (downloads and builds libpiper)" ON)
option(LUKIJA_BUILD_TESTS "Build the unit and corpus tests" ON)
option(LUKIJA_BUILD_APP "Build the Sailfish OS app (needs libsailfishapp)" ${SAILFISHAPP_FOUND})

# Sailfish OS ships Qt 5.6: code must not use Qt APIs newer than that,
# even though desktop builds use a newer Qt 5.
find_package(Qt5 5.6 REQUIRED COMPONENTS Core Gui Multimedia)
pkg_check_modules(POPPLER_QT5 REQUIRED IMPORTED_TARGET poppler-qt5)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()
add_compile_definitions(QT_NO_FOREACH QT_DEPRECATED_WARNINGS)

# Installed layout (Sailfish): the app's own files, including the bundled
# libpiper and onnxruntime, live under share/harbour-lukija.
include(GNUInstallDirs)
set(LUKIJA_DATADIR ${CMAKE_INSTALL_DATADIR}/harbour-lukija)
set(LUKIJA_PRIVATE_LIBDIR ${LUKIJA_DATADIR}/lib)
set(CMAKE_INSTALL_RPATH "$ORIGIN/../${LUKIJA_PRIVATE_LIBDIR}")

if(LUKIJA_WITH_PIPER)
    add_subdirectory(3rdparty)
endif()

add_subdirectory(core)
add_subdirectory(audio)
add_subdirectory(tts)
add_subdirectory(app)
add_subdirectory(tools/lukija-cli)

if(LUKIJA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
