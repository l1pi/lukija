# Patches libpiper 1.8.0. Run as PATCH_COMMAND with -DSOURCE_DIR=<piper1-gpl source>.
#
# Build without git or network access (Sailfish SDK targets have no git, OBS
# builds have no network):
#   - espeak-ng comes from ${LUKIJA_ESPEAK_URL}, which may be a local file.
#   - espeak-ng's own git fetch of sonic is pointed at ${LUKIJA_SONIC_DIR}.
#
# onnxruntime threads and graph optimisation, fixed at 1 thread and "basic" in
# libpiper, come from $LUKIJA_PIPER_THREADS and $LUKIJA_PIPER_OPTIMIZE=all.

function(patch_file file original patched)
    file(READ "${file}" contents)
    string(FIND "${contents}" "${patched}" already_patched)
    if(already_patched GREATER -1)
        return()
    endif()
    string(FIND "${contents}" "${original}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "patch-libpiper.cmake: ${file} has changed; update the patch")
    endif()
    string(REPLACE "${original}" "${patched}" contents "${contents}")
    file(WRITE "${file}" "${contents}")
endfunction()

set(cmakelists ${SOURCE_DIR}/libpiper/CMakeLists.txt)
patch_file(${cmakelists}
"    GIT_REPOSITORY https://github.com/espeak-ng/espeak-ng.git
    GIT_TAG 212928b394a96e8fd2096616bfd54e17845c48f6  # 2025-Mar-22
"
"    URL \${LUKIJA_ESPEAK_URL}
    URL_HASH SHA256=\${LUKIJA_ESPEAK_SHA256}
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
")
patch_file(${cmakelists}
"        -DCMAKE_INSTALL_PREFIX=\${ESPEAKNG_INSTALL_DIR}
"
"        -DCMAKE_INSTALL_PREFIX=\${ESPEAKNG_INSTALL_DIR}
        -DFETCHCONTENT_SOURCE_DIR_SONIC-GIT=\${LUKIJA_SONIC_DIR}
")

set(source ${SOURCE_DIR}/libpiper/src/piper.cpp)
patch_file(${source}
"#include <cstddef>
"
"#include <cstddef>
#include <cstdlib>
#include <cstring>
")
patch_file(${source}
"  synth->session_options.SetIntraOpNumThreads(1);
  synth->session_options.SetInterOpNumThreads(1);
  synth->session_options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_BASIC);
"
"  // Lukija: thread count and optimisation level from the environment.
  const char *lukija_threads = std::getenv(\"LUKIJA_PIPER_THREADS\");
  const char *lukija_optimize = std::getenv(\"LUKIJA_PIPER_OPTIMIZE\");
  synth->session_options.SetIntraOpNumThreads(
      lukija_threads && std::atoi(lukija_threads) > 0 ? std::atoi(lukija_threads) : 1);
  synth->session_options.SetInterOpNumThreads(1);
  synth->session_options.SetGraphOptimizationLevel(
      lukija_optimize && std::strcmp(lukija_optimize, \"all\") == 0
          ? GraphOptimizationLevel::ORT_ENABLE_ALL
          : GraphOptimizationLevel::ORT_ENABLE_BASIC);
")
