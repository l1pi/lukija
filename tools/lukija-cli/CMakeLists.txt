add_executable(lukija-cli
    main.cpp
    speaker.h speaker.cpp
)
target_link_libraries(lukija-cli PRIVATE lukija-core lukija-audio lukija-tts)
