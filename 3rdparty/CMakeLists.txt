# libpiper (Piper TTS C API) from OHF-Voice/piper1-gpl. Its own build downloads
# a prebuilt onnxruntime and builds espeak-ng statically.
include(ExternalProject)

set(PIPER_VERSION 1.8.0)
set(PIPER_INSTALL_DIR ${CMAKE_BINARY_DIR}/3rdparty/piper-install)
set(PIPER_LIBRARY ${PIPER_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}piper${CMAKE_SHARED_LIBRARY_SUFFIX})

# espeak-ng builds file paths in a 160-byte buffer: its data path plus up to
# 36 characters of source file name ("/../phsource/ustop/tsh_pzd_unasp.wav").
# Longer paths fail with confusing compile errors, so keep libpiper's build
# tree shallow and stop early with a clear message.
set(PIPER_BINARY_DIR ${CMAKE_BINARY_DIR}/pb)
set(espeak_build_data ${PIPER_BINARY_DIR}/espeak_ng/src/espeak_ng_external-build/espeak-ng-data)
string(LENGTH "${espeak_build_data}" espeak_path_length)
if(espeak_path_length GREATER 120)
    message(FATAL_ERROR "The build directory path is too long for espeak-ng's build "
                        "(${espeak_path_length} > 120 characters in ${espeak_build_data}). "
                        "Use a build directory with a shorter path.")
endif()

ExternalProject_Add(piper_external
    URL https://github.com/OHF-Voice/piper1-gpl/archive/refs/tags/v${PIPER_VERSION}.tar.gz
    URL_HASH SHA256=0a22987a6157f14e2fb7cf4aa702b547467fd0448bea9f233abd503c3ff4a0e1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PREFIX ${CMAKE_BINARY_DIR}/3rdparty/piper
    BINARY_DIR ${PIPER_BINARY_DIR}
    SOURCE_SUBDIR libpiper
    CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=Release
        -DCMAKE_INSTALL_PREFIX:PATH=${PIPER_INSTALL_DIR}
        -DCMAKE_INSTALL_LIBDIR:STRING=lib
        # libpiper finds libonnxruntime next to itself once installed.
        -DCMAKE_INSTALL_RPATH:STRING=$ORIGIN
        -DCMAKE_POLICY_VERSION_MINIMUM:STRING=3.5
    BUILD_BYPRODUCTS ${PIPER_LIBRARY}
)

# The include directory must exist at configure time for the imported target.
file(MAKE_DIRECTORY ${PIPER_INSTALL_DIR}/include)

add_library(piper SHARED IMPORTED GLOBAL)
set_target_properties(piper PROPERTIES
    IMPORTED_LOCATION ${PIPER_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${PIPER_INSTALL_DIR}/include
)
add_dependencies(piper piper_external)

set(PIPER_ESPEAK_DATA_DIR ${PIPER_INSTALL_DIR}/share/espeak-ng-data PARENT_SCOPE)
