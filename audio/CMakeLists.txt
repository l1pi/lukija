add_library(lukija-audio STATIC
    pcm.h pcm.cpp
    audiosink.h audiosink.cpp
)
target_include_directories(lukija-audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lukija-audio PUBLIC Qt5::Core Qt5::Multimedia)
