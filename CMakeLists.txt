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
set(FINALE_MUS_READER_MUSXDOM_SOURCE_DIR "" CACHE PATH
    "Use an existing musxdom source tree instead of fetching the pinned revision")

find_package(ZLIB REQUIRED)

if(NOT TARGET musx)
    set(musx_BUILD_TESTING OFF CACHE BOOL "Do not build musxdom tests" FORCE)
    set(MUSX_USE_RAPIDXML ON CACHE BOOL "Use musxdom's bundled RapidXML backend" FORCE)

    if(FINALE_MUS_READER_MUSXDOM_SOURCE_DIR)
        get_filename_component(FINALE_MUS_READER_MUSXDOM_SOURCE_DIR
            "${FINALE_MUS_READER_MUSXDOM_SOURCE_DIR}" ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        if(NOT EXISTS "${FINALE_MUS_READER_MUSXDOM_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR
                "FINALE_MUS_READER_MUSXDOM_SOURCE_DIR is not a musxdom source tree")
        endif()
        set(FINALE_MUS_READER_MUSXDOM_ROOT "${FINALE_MUS_READER_MUSXDOM_SOURCE_DIR}")
        add_subdirectory("${FINALE_MUS_READER_MUSXDOM_ROOT}"
            "${CMAKE_CURRENT_BINARY_DIR}/_deps/musxdom-build")
    else()
        include(FetchContent)
        FetchContent_Declare(musxdom
            GIT_REPOSITORY https://github.com/rpatters1/musxdom.git
            GIT_TAG 56241cc67eff340b1899be029aaae6927824593b
            GIT_SHALLOW FALSE)
        FetchContent_MakeAvailable(musxdom)
        set(FINALE_MUS_READER_MUSXDOM_ROOT "${musxdom_SOURCE_DIR}")
    endif()
elseif(MUSX_ROOT_DIR)
    set(FINALE_MUS_READER_MUSXDOM_ROOT "${MUSX_ROOT_DIR}")
elseif(NOT FINALE_MUS_READER_MUSXDOM_SOURCE_DIR)
    message(FATAL_ERROR
        "A pre-existing musx target must expose MUSX_ROOT_DIR or be accompanied by "
        "FINALE_MUS_READER_MUSXDOM_SOURCE_DIR so RapidXML headers can be located")
else()
    get_filename_component(FINALE_MUS_READER_MUSXDOM_ROOT
        "${FINALE_MUS_READER_MUSXDOM_SOURCE_DIR}" ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set(FINALE_MUS_READER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FINALE_MUS_READER_EMBEDDED_HEADER
    "${FINALE_MUS_READER_GENERATED_DIR}/embedded_default.h")
set(FINALE_MUS_READER_EMBEDDED_SOURCE
    "${FINALE_MUS_READER_GENERATED_DIR}/embedded_default.cpp")

add_custom_command(
    OUTPUT
        "${FINALE_MUS_READER_EMBEDDED_HEADER}"
        "${FINALE_MUS_READER_EMBEDDED_SOURCE}"
    COMMAND "${CMAKE_COMMAND}"
        -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/resources/defaults/finale27-macos-new-document-without-libraries.enigmaxml.gz
        -DOUTPUT_HEADER=${FINALE_MUS_READER_EMBEDDED_HEADER}
        -DOUTPUT_SOURCE=${FINALE_MUS_READER_EMBEDDED_SOURCE}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    DEPENDS
        resources/defaults/finale27-macos-new-document-without-libraries.enigmaxml.gz
        cmake/embed_binary.cmake
    COMMENT "Embedding the pinned Finale 27 macOS default"
    VERBATIM)

add_library(finale_mus_reader STATIC
    src/container/mus_container.cpp
    src/defaults/default_document.cpp
    src/import/reader.cpp
    src/records/legacy_others.cpp
    third_party/blast/blast.c
    "${FINALE_MUS_READER_EMBEDDED_SOURCE}")
add_library(finale_mus_reader::finale_mus_reader ALIAS finale_mus_reader)

target_include_directories(finale_mus_reader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/blast"
        "${FINALE_MUS_READER_GENERATED_DIR}"
        "${FINALE_MUS_READER_MUSXDOM_ROOT}/third_party/rapidxml")

target_link_libraries(finale_mus_reader PUBLIC musx PRIVATE ZLIB::ZLIB)
target_compile_definitions(finale_mus_reader PRIVATE MUSX_USE_RAPIDXML)

if(MSVC)
    target_compile_options(finale_mus_reader PRIVATE /W4 /WX)
else()
    target_compile_options(finale_mus_reader PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(FINALE_MUS_READER_BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
