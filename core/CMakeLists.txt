# Platform-independent core: PDF text extraction and the text pipeline.
# Plain Qt (5.6 APIs only) and poppler-qt5, so it builds and tests on desktop.
add_library(lukija-core STATIC
    appinfo.h
    model/document.h model/document.cpp
    pdf/pdfdocument.h pdf/pdfdocument.cpp
    pdf/textextractor.h pdf/textextractor.cpp
    layout/layoutanalyzer.h layout/layoutanalyzer.cpp
    text/mappedtext.h text/mappedtext.cpp
    text/normalizer.h text/normalizer.cpp
    text/dehyphenator.h text/dehyphenator.cpp
    text/abbreviations.h text/abbreviations.cpp
    text/segmenter.h text/segmenter.cpp
    text/languagedetector.h text/languagedetector.cpp
    pipeline.h pipeline.cpp
)
target_include_directories(lukija-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lukija-core PUBLIC Qt5::Core Qt5::Gui PkgConfig::POPPLER_QT5)
