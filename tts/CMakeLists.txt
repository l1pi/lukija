add_library(lukija-tts STATIC
    ttsengine.h
    voicelibrary.h voicelibrary.cpp
    synthesisqueue.h synthesisqueue.cpp
)
target_include_directories(lukija-tts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lukija-tts PUBLIC Qt5::Core)

if(LUKIJA_WITH_PIPER)
    # Relative to the installed executables, which all go to bin/.
    file(RELATIVE_PATH installed_espeak_data ${CMAKE_INSTALL_FULL_BINDIR}
         ${CMAKE_INSTALL_PREFIX}/${LUKIJA_DATADIR}/espeak-ng-data)
    target_sources(lukija-tts PRIVATE piperengine.h piperengine.cpp)
    target_link_libraries(lukija-tts PUBLIC piper)
    target_compile_definitions(lukija-tts
        PUBLIC LUKIJA_HAVE_PIPER
        PRIVATE LUKIJA_PIPER_ESPEAK_DATA="${PIPER_ESPEAK_DATA_DIR}"
                LUKIJA_INSTALLED_ESPEAK_DATA="${installed_espeak_data}")
endif()
