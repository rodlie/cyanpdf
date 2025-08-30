# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>

cmake_minimum_required(VERSION 3.16)

project(CyanPDF VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent)

find_package(PkgConfig QUIET)
pkg_search_module(LCMS2 REQUIRED lcms2)

set(PROJECT_SOURCES
    src/main.cpp
    src/cyanpdf.cpp
    src/cyanpdf.h
)

qt_add_executable(CyanPDF MANUAL_FINALIZATION ${PROJECT_SOURCES})

target_include_directories(CyanPDF PRIVATE ${LCMS2_INCLUDE_DIRS})

target_link_libraries(CyanPDF PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(CyanPDF PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(CyanPDF PRIVATE ${LCMS2_LIBRARIES} ${LCMS2_LDFLAGS})

set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER graphics.cyan.pdf)
set_target_properties(CyanPDF PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS CyanPDF
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(CyanPDF)
