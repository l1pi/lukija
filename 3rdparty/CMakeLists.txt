# libpiper (Piper TTS C API) from OHF-Voice/piper1-gpl, with espeak-ng built
# statically into it and a prebuilt onnxruntime next to it.
#
# The archives are downloaded by default. Builds without network access
# (OBS) pass local copies, e.g. -DLUKIJA_PIPER_URL=/path/to/v1.8.0.tar.gz; the
# checksums still apply. Archive timestamps aren't restored on extraction: the
# Sailfish SDK builds in a VirtualBox shared folder, which can't set the times
# of symlinks.
include(ExternalProject)

set(PIPER_VERSION 1.8.0)
set(LUKIJA_PIPER_URL
    https://github.com/OHF-Voice/piper1-gpl/archive/refs/tags/v${PIPER_VERSION}.tar.gz
    CACHE STRING "libpiper source archive (URL or local file)")
set(PIPER_SHA256 0a22987a6157f14e2fb7cf4aa702b547467fd0448bea9f233abd503c3ff4a0e1)

# The espeak-ng commit libpiper 1.8.0 pins.
set(ESPEAK_COMMIT 212928b394a96e8fd2096616bfd54e17845c48f6)
set(LUKIJA_ESPEAK_URL
    https://github.com/espeak-ng/espeak-ng/archive/${ESPEAK_COMMIT}.tar.gz
    CACHE STRING "espeak-ng source archive (URL or local file)")
set(ESPEAK_SHA256 1f201cabc73e569a7cb434d40d3b30980f923010f8ecd4d1c4ae94691ac2888a)

# The sonic commit that espeak-ng fetches. Unused (USE_LIBSONIC is off), but
# espeak-ng's build compiles it anyway.
set(SONIC_COMMIT fbf75c3d6d846bad3bb3d456cbc5d07d9fd8c104)
set(LUKIJA_SONIC_URL
    https://github.com/waywardgeek/sonic/archive/${SONIC_COMMIT}.tar.gz
    CACHE STRING "sonic source archive (URL or local file)")
set(SONIC_SHA256 715827b5a39b79e56e44397d7b845910df996d4cca74777b3b61629b1ddc98c1)

set(ONNXRUNTIME_VERSION 1.22.0)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    set(onnxruntime_arch x64)
    set(ONNXRUNTIME_SHA256 8344d55f93d5bc5021ce342db50f62079daf39aaafb5d311a451846228be49b3)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(onnxruntime_arch aarch64)
    set(ONNXRUNTIME_SHA256 bb76395092d150b52c7092dc6b8f2fe4d80f0f3bf0416d2f269193e347e24702)
else()
    message(FATAL_ERROR "No prebuilt onnxruntime for ${CMAKE_SYSTEM_PROCESSOR}; "
                        "configure with -DLUKIJA_WITH_PIPER=OFF")
endif()
set(LUKIJA_ONNXRUNTIME_URL
    https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-${onnxruntime_arch}-${ONNXRUNTIME_VERSION}.tgz
    CACHE STRING "Prebuilt onnxruntime archive (URL or local file)")

set(LUKIJA_ESPEAK_LANGUAGES fi en CACHE STRING "espeak-ng dictionaries to install")

set(PIPER_INSTALL_DIR ${CMAKE_BINARY_DIR}/3rdparty/piper-install)
set(PIPER_LIBRARY ${PIPER_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}piper${CMAKE_SHARED_LIBRARY_SUFFIX})
set(ONNXRUNTIME_DIR ${CMAKE_BINARY_DIR}/3rdparty/onnxruntime)
set(SONIC_DIR ${CMAKE_BINARY_DIR}/3rdparty/sonic)

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

# Archives that are only extracted for libpiper's build to use.
function(lukija_extract name url sha256 directory)
    ExternalProject_Add(${name}
        URL ${url}
        URL_HASH SHA256=${sha256}
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        PREFIX ${CMAKE_BINARY_DIR}/3rdparty/${name}
        SOURCE_DIR ${directory}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )
endfunction()
lukija_extract(onnxruntime_external ${LUKIJA_ONNXRUNTIME_URL} ${ONNXRUNTIME_SHA256} ${ONNXRUNTIME_DIR})
lukija_extract(sonic_external ${LUKIJA_SONIC_URL} ${SONIC_SHA256} ${SONIC_DIR})

ExternalProject_Add(piper_external
    URL ${LUKIJA_PIPER_URL}
    URL_HASH SHA256=${PIPER_SHA256}
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    PREFIX ${CMAKE_BINARY_DIR}/3rdparty/piper
    BINARY_DIR ${PIPER_BINARY_DIR}
    SOURCE_SUBDIR libpiper
    PATCH_COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=<SOURCE_DIR>
                  -P ${CMAKE_CURRENT_SOURCE_DIR}/patch-libpiper.cmake
    CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=Release
        -DCMAKE_INSTALL_PREFIX:PATH=${PIPER_INSTALL_DIR}
        -DCMAKE_INSTALL_LIBDIR:STRING=lib
        # libpiper finds libonnxruntime next to itself once installed.
        -DCMAKE_INSTALL_RPATH:STRING=$ORIGIN
        -DCMAKE_POLICY_VERSION_MINIMUM:STRING=3.5
        -DONNXRUNTIME_DIR:PATH=${ONNXRUNTIME_DIR}
        -DLUKIJA_ESPEAK_URL:STRING=${LUKIJA_ESPEAK_URL}
        -DLUKIJA_ESPEAK_SHA256:STRING=${ESPEAK_SHA256}
        -DLUKIJA_SONIC_DIR:PATH=${SONIC_DIR}
    DEPENDS onnxruntime_external sonic_external
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

# Installed privately with the app: libpiper and onnxruntime under
# share/harbour-lukija/lib, and only the espeak-ng data the voices need.
install(DIRECTORY ${PIPER_INSTALL_DIR}/lib/
    DESTINATION ${LUKIJA_PRIVATE_LIBDIR}
    USE_SOURCE_PERMISSIONS
    FILES_MATCHING
        PATTERN "libpiper.so"
        PATTERN "libonnxruntime.so.1*"
        PATTERN "cmake" EXCLUDE
        PATTERN "pkgconfig" EXCLUDE
)
set(espeak_data ${PIPER_INSTALL_DIR}/share/espeak-ng-data)
set(espeak_files ${espeak_data}/phondata ${espeak_data}/phonindex ${espeak_data}/phontab
                 ${espeak_data}/intonations)
foreach(language IN LISTS LUKIJA_ESPEAK_LANGUAGES)
    list(APPEND espeak_files ${espeak_data}/${language}_dict)
endforeach()
install(FILES ${espeak_files} DESTINATION ${LUKIJA_DATADIR}/espeak-ng-data)
install(DIRECTORY ${espeak_data}/lang ${espeak_data}/voices
    DESTINATION ${LUKIJA_DATADIR}/espeak-ng-data)
