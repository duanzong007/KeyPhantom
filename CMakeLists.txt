cmake_minimum_required(VERSION 3.21)

project(KeyPhantom VERSION 1.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR "KeyPhantom supports Windows and macOS only.")
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

qt_add_executable(KeyPhantom
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    ui/mainwindow.ui
    resources/resources.qrc
)

target_link_libraries(KeyPhantom PRIVATE Qt6::Widgets)

set_target_properties(KeyPhantom PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(WIN32)
    enable_language(RC)
    set(KEYPHANTOM_WINDOWS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/keyphantom_logo.ico")
    configure_file(cmake/KeyPhantom.rc.in "${CMAKE_CURRENT_BINARY_DIR}/KeyPhantom.rc" @ONLY)
    target_sources(KeyPhantom PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/KeyPhantom.rc")
    target_link_libraries(KeyPhantom PRIVATE user32)
elseif(APPLE)
    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices REQUIRED)
    find_library(CARBON_FRAMEWORK Carbon REQUIRED)
    target_link_libraries(KeyPhantom PRIVATE
        "${APPLICATION_SERVICES_FRAMEWORK}"
        "${CARBON_FRAMEWORK}"
    )

    set(MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/keyphantom_logo.icns")
    target_sources(KeyPhantom PRIVATE "${MACOS_ICON}")
    set_source_files_properties("${MACOS_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(KeyPhantom PROPERTIES
        MACOSX_BUNDLE_ICON_FILE keyphantom_logo.icns
        MACOSX_BUNDLE_BUNDLE_NAME KeyPhantom
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
endif()

include(GNUInstallDirs)
install(TARGETS KeyPhantom
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
