add_executable(lukija-cli
    main.cpp
    speaker.h speaker.cpp
)
target_link_libraries(lukija-cli PRIVATE lukija-app)

install(TARGETS lukija-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
