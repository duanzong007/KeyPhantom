cmake_minimum_required(VERSION 3.21)

project(KeyPhantom VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    add_compile_options(/utf-8)
endif()
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
    src/appearance.cpp
    src/appearance.h
    src/inputplan.cpp
    src/inputplan.h
    src/keysenderthread.cpp
    src/keysenderthread.h
    src/platforminput.cpp
    src/platforminput.h
    src/globalstophotkey.cpp
    src/globalstophotkey.h
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

option(KEYPHANTOM_BUILD_TESTS "Build input and window regression tests" OFF)
if(KEYPHANTOM_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 6.2 REQUIRED COMPONENTS Test)
    qt_add_executable(input_test
        tests/input_test.cpp
        src/inputplan.cpp
        src/keysenderthread.cpp
        src/platforminput.cpp
    )
    target_include_directories(input_test PRIVATE src)
    target_link_libraries(input_test PRIVATE Qt6::Test)
    qt_add_executable(window_test
        tests/window_test.cpp
        src/mainwindow.cpp
        src/inputplan.cpp
        src/keysenderthread.cpp
        src/platforminput.cpp
        src/globalstophotkey.cpp
        src/appearance.cpp
        ui/mainwindow.ui
        resources/resources.qrc
    )
    target_include_directories(window_test PRIVATE src)
    target_link_libraries(window_test PRIVATE Qt6::Test Qt6::Widgets)
    foreach(test_target input_test window_test)
        if(WIN32)
            target_link_libraries(${test_target} PRIVATE user32)
        elseif(APPLE)
            target_link_libraries(${test_target} PRIVATE
                "${APPLICATION_SERVICES_FRAMEWORK}" "${CARBON_FRAMEWORK}")
        endif()
        add_test(NAME ${test_target} COMMAND ${test_target})
    endforeach()
    set_tests_properties(window_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
