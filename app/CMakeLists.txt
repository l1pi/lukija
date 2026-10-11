# The app's logic, without Sailfish dependencies: builds and tests on desktop.
find_package(Qt5 5.6 REQUIRED COMPONENTS Concurrent Sql)

add_library(lukija-app STATIC
    library.h library.cpp
    playbackcontroller.h playbackcontroller.cpp
    sentencemodel.h sentencemodel.cpp
    librarymodel.h librarymodel.cpp
    reader.h reader.cpp
)
target_include_directories(lukija-app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lukija-app PUBLIC lukija-core lukija-audio lukija-tts Qt5::Concurrent Qt5::Sql)

if(NOT LUKIJA_BUILD_APP)
    return()
endif()

# The Sailfish OS app.
find_package(Qt5 5.6 REQUIRED COMPONENTS Qml Quick)

add_executable(harbour-lukija main.cpp)
target_compile_definitions(harbour-lukija PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
    LUKIJA_VERSION="${PROJECT_VERSION}")
target_link_libraries(harbour-lukija PRIVATE lukija-app Qt5::Quick PkgConfig::SAILFISHAPP)

set(source_dir ${PROJECT_SOURCE_DIR})
install(TARGETS harbour-lukija RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${source_dir}/qml DESTINATION ${LUKIJA_DATADIR})
install(FILES ${source_dir}/LICENSE DESTINATION ${LUKIJA_DATADIR})
install(FILES ${source_dir}/harbour-lukija.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
foreach(size 86 108 128 172)
    install(FILES ${source_dir}/icons/${size}x${size}/harbour-lukija.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps)
endforeach()

# Lets Qt Creator find the executable on the device.
file(WRITE ${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt
    "${CMAKE_INSTALL_PREFIX}\n${CMAKE_CURRENT_BINARY_DIR}/harbour-lukija:bin\n")
