# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>

cmake_minimum_required(VERSION 3.16)

project(cyanpdf VERSION 0.9.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DESKTOP_ID "graphics.cyan.pdf")

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Pdf Svg)

find_package(PkgConfig QUIET)
pkg_search_module(LCMS2 REQUIRED lcms2)

add_definitions(-DCYANPDF_VERSION="${PROJECT_VERSION}")
add_definitions(-DCYANPDF_ID="${DESKTOP_ID}")

if(CMAKE_BUILD_TYPE MATCHES "^(release|Release|RELEASE)$")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

set(PROJECT_SOURCES
    main.cpp
    cyanpdf.cpp
    cyanpdf.h
    cyanpdf.qrc
)

qt_add_executable(cyanpdf MANUAL_FINALIZATION ${PROJECT_SOURCES})

target_include_directories(cyanpdf PRIVATE ${LCMS2_INCLUDE_DIRS})

target_link_libraries(cyanpdf PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Pdf Qt${QT_VERSION_MAJOR}::Svg)
target_link_libraries(cyanpdf PRIVATE ${LCMS2_LIBRARIES} ${LCMS2_LDFLAGS})

set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER ${DESKTOP_ID})
set_target_properties(cyanpdf PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS cyanpdf
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/${DESKTOP_ID}.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/${DESKTOP_ID}.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/${DESKTOP_ID}.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME}-${PROJECT_VERSION})

qt_finalize_executable(cyanpdf)
