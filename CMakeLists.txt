cmake_minimum_required(VERSION 3.20)

project(finale_mus_reader VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
if(CMAKE_CXX_STANDARD LESS 20)
    message(FATAL_ERROR "Legacy Finale MUS Reader requires at least C++20")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FINALE_MUS_READER_BUILD_TESTING "Build Legacy Finale MUS Reader tests" ON)
option(FINALE_MUS_READER_USE_SYSTEM_ZLIB
    "Use an installed zlib when a parent project has not already provided one" OFF)
set(FINALE_MUS_READER_MUSXDOM_SOURCE_DIR "" CACHE PATH
    "Use an existing musxdom source tree instead of fetching the pinned revision")

include(FetchContent)

if(TARGET ZLIB::ZLIB)
    set(FINALE_MUS_READER_ZLIB_TARGET ZLIB::ZLIB)
elseif(TARGET zlibstatic)
    set(FINALE_MUS_READER_ZLIB_TARGET zlibstatic)
elseif(TARGET zlib)
    set(FINALE_MUS_READER_ZLIB_TARGET zlib)
elseif(FINALE_MUS_READER_USE_SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
    set(FINALE_MUS_READER_ZLIB_TARGET ZLIB::ZLIB)
else()
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Do not build zlib examples")
    FetchContent_Declare(zlib
        URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
        URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

    set(FINALE_MUS_READER_SAVED_DEV_WARNINGS
        "${CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "")
    FetchContent_MakeAvailable(zlib)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
        "${FINALE_MUS_READER_SAVED_DEV_WARNINGS}" CACHE INTERNAL "")

    if(TARGET zlib)
        set_target_properties(zlib PROPERTIES
            EXCLUDE_FROM_ALL TRUE
            EXCLUDE_FROM_DEFAULT_BUILD TRUE)
    endif()

    if(TARGET zlibstatic)
        set(FINALE_MUS_READER_ZLIB_TARGET zlibstatic)
    elseif(TARGET zlib)
        set(FINALE_MUS_READER_ZLIB_TARGET zlib)
    elseif(TARGET ZLIB::ZLIB)
        set(FINALE_MUS_READER_ZLIB_TARGET ZLIB::ZLIB)
    else()
        message(FATAL_ERROR "Unable to determine the zlib target")
    endif()
endif()

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS ${FINALE_MUS_READER_ZLIB_TARGET})
endif()
message(STATUS "Legacy Finale MUS Reader zlib target: ${FINALE_MUS_READER_ZLIB_TARGET}")

if(TARGET pugixml)
    set(FINALE_MUS_READER_PUGIXML_TARGET pugixml)
elseif(TARGET pugixml::pugixml)
    set(FINALE_MUS_READER_PUGIXML_TARGET pugixml::pugixml)
else()
    set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "Do not build pugixml tests")
    FetchContent_Declare(pugixml
        URL https://github.com/zeux/pugixml/releases/download/v1.15/pugixml-1.15.tar.gz
        URL_HASH SHA256=655ade57fa703fb421c2eb9a0113b5064bddb145d415dd1f88c79353d90d511a
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(pugixml)
    set(FINALE_MUS_READER_PUGIXML_TARGET pugixml)
endif()
message(STATUS
    "Legacy Finale MUS Reader pugixml target: ${FINALE_MUS_READER_PUGIXML_TARGET}")

if(NOT TARGET musx)
    set(musx_BUILD_TESTING OFF CACHE BOOL "Do not build musxdom tests" FORCE)
    set(MUSX_USE_TINYXML2 OFF CACHE BOOL "Do not enable musxdom TinyXML2 support" FORCE)
    set(MUSX_USE_RAPIDXML OFF CACHE BOOL "Do not enable musxdom RapidXML support" FORCE)
    set(MUSX_USE_PUGIXML OFF CACHE BOOL "Do not propagate musxdom pugixml support" FORCE)
    set(MUSX_USE_QTXML OFF CACHE BOOL "Do not enable musxdom QtXML support" FORCE)

    if(FINALE_MUS_READER_MUSXDOM_SOURCE_DIR)
        get_filename_component(FINALE_MUS_READER_MUSXDOM_SOURCE_DIR
            "${FINALE_MUS_READER_MUSXDOM_SOURCE_DIR}" ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        if(NOT EXISTS "${FINALE_MUS_READER_MUSXDOM_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR
                "FINALE_MUS_READER_MUSXDOM_SOURCE_DIR is not a musxdom source tree")
        endif()
        add_subdirectory("${FINALE_MUS_READER_MUSXDOM_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/_deps/musxdom-build")
    else()
        FetchContent_Declare(musxdom
            GIT_REPOSITORY https://github.com/rpatters1/musxdom.git
            GIT_TAG 56241cc67eff340b1899be029aaae6927824593b
            GIT_SHALLOW FALSE)
        FetchContent_MakeAvailable(musxdom)
    endif()
endif()

add_library(finale_mus_reader STATIC
    src/container/mus_container.cpp
    src/defaults/default_document.cpp
    src/defaults/embedded_default.cpp
    src/import/reader.cpp
    src/records/legacy_others.cpp
    third_party/blast/blast.c)
add_library(finale_mus_reader::finale_mus_reader ALIAS finale_mus_reader)

target_include_directories(finale_mus_reader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/blast")

target_link_libraries(finale_mus_reader
    PUBLIC musx
    PRIVATE
        ${FINALE_MUS_READER_PUGIXML_TARGET}
        ${FINALE_MUS_READER_ZLIB_TARGET})

if(MSVC)
    target_compile_options(finale_mus_reader PRIVATE /W4 /WX)
else()
    target_compile_options(finale_mus_reader PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(FINALE_MUS_READER_BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
