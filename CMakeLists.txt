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

if(MSVC)
    # musxdom's template-heavy factories can exceed the default COFF section
    # limit in any translation unit that instantiates them. Apply this to the
    # entire directory tree so no MSVC object is built without it.
    add_compile_options(/bigobj)
    # Sources carry UTF-8 string literals and no byte-order mark: the legacy text
    # tests compare against real decoded names such as "Célino" and "ヒラギノ明朝".
    # Without this MSVC reads them in the machine's active code page and silently
    # produces different bytes, so this is a directory-wide invariant too.
    add_compile_options(/utf-8)
endif()

option(FINALE_MUS_READER_BUILD_TESTING "Build Legacy Finale MUS Reader tests" ON)
option(FINALE_MUS_READER_BUILD_TOOLS
    "Build on-demand corpus probe tools in tools/" OFF)
option(FINALE_MUS_READER_USE_SYSTEM_ZLIB
    "Use an installed zlib when a parent project has not already provided one" OFF)
set(FINALE_MUS_READER_MUSXDOM_SOURCE_DIR "" CACHE PATH
    "Use an existing musxdom source tree instead of fetching the pinned revision")

include(FetchContent)

# A unity build is this project's invariant, not a dependency's. -DCMAKE_UNITY_BUILD=ON is a
# smoke test of project-owned translation units, and it must not impose an amalgamation policy
# on code this repository does not own: a dependency that does not amalgamate would otherwise
# turn the smoke test into a build failure that says nothing about this project. zlib is the
# standing example -- its inftrees.h is included by several of its sources and is not
# idempotent -- but the rule is about ownership rather than about zlib.
#
# Stated once here and applied at each dependency, so a target added later is opted out by
# calling this rather than by remembering a policy written somewhere else.
function(finale_mus_reader_keep_out_of_unity)
    foreach(finale_mus_reader_foreign_target IN LISTS ARGN)
        if(TARGET ${finale_mus_reader_foreign_target})
            set_target_properties(${finale_mus_reader_foreign_target} PROPERTIES
                UNITY_BUILD OFF)
        endif()
    endforeach()
endfunction()

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

    finale_mus_reader_keep_out_of_unity(zlib zlibstatic)

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

if(TARGET musx)
    get_target_property(FINALE_MUS_READER_MUSXDOM_SOURCE musx SOURCE_DIR)
    set(FINALE_MUS_READER_MUSXDOM_ORIGIN "existing musx target")
else()
    set(musx_BUILD_TESTING OFF CACHE BOOL "Do not build musxdom tests" FORCE)

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
        set(FINALE_MUS_READER_MUSXDOM_SOURCE
            "${FINALE_MUS_READER_MUSXDOM_SOURCE_DIR}")
        set(FINALE_MUS_READER_MUSXDOM_ORIGIN "local source tree")
    else()
        FetchContent_Declare(musxdom
            GIT_REPOSITORY https://github.com/rpatters1/musxdom.git
            GIT_TAG 2fe21eacc9df526f0fce4215a06238641520da3a
            GIT_SHALLOW FALSE)
        FetchContent_MakeAvailable(musxdom)
        set(FINALE_MUS_READER_MUSXDOM_SOURCE "${musxdom_SOURCE_DIR}")
        set(FINALE_MUS_READER_MUSXDOM_ORIGIN "fetched pinned revision")
    endif()
endif()
message(STATUS
    "Legacy Finale MUS Reader musxdom: ${FINALE_MUS_READER_MUSXDOM_ORIGIN} at ${FINALE_MUS_READER_MUSXDOM_SOURCE}")

finale_mus_reader_keep_out_of_unity(musx)
if(CMAKE_UNITY_BUILD)
    message(STATUS
        "Legacy Finale MUS Reader: unity build limited to project-owned translation units")
endif()

add_library(finale_mus_reader_blast OBJECT
    third_party/blast/blast.c)

# Pinned upstream source rather than this project's, so it follows the same rule as the
# fetched dependencies even though the target is declared here.
finale_mus_reader_keep_out_of_unity(finale_mus_reader_blast)

if(MSVC)
    # C4244 remains enabled at /W3. Suppress it for the unmodified upstream
    # source while retaining useful third-party diagnostics below /W4.
    target_compile_options(finale_mus_reader_blast PRIVATE /W3 /WX- /wd4244)
else()
    target_compile_options(finale_mus_reader_blast PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(finale_mus_reader STATIC
    src/container/mus_container.cpp
    src/container/product_banner.cpp
    src/defaults/default_document.cpp
    src/defaults/embedded_default.cpp
    src/import/details/measure_graphic_assign.cpp
    src/import/header/header.cpp
    src/import/options/clef_options.cpp
    src/import/options/font_options.cpp
    src/import/options/music_spacing_options.cpp
    src/import/options/stem_options.cpp
    src/import/others/font_definitions.cpp
    src/import/others/graphic_assignments.cpp
    src/import/others/layer_attributes.cpp
    src/import/others/shape_definitions.cpp
    src/import/support/embedded_graphics.cpp
    src/import/support/legacy_mapping.cpp
    src/import/support/text_encoding.cpp
    src/reader/document_factory.cpp
    src/reader/reader.cpp
    src/records/legacy_others.cpp
    src/records/legacy_record_index.cpp
    $<TARGET_OBJECTS:finale_mus_reader_blast>)
add_library(finale_mus_reader::finale_mus_reader ALIAS finale_mus_reader)

target_include_directories(finale_mus_reader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/blast")

target_link_libraries(finale_mus_reader
    PUBLIC musx
    PRIVATE ${FINALE_MUS_READER_ZLIB_TARGET})

# Legacy MUS stores text in whatever encoding the saving machine used, so the reader has to
# convert it to the UTF-8 that musxdom expects. Each platform gets the most accurate
# converter it has rather than a common subset: Windows uses its own APIs, macOS uses
# CoreFoundation, which is the only one of the three that resolves the classic Mac encodings
# natively, and everything else uses iconv. CoreFoundation is pure C, so no Objective-C
# toolchain is involved.
#
# iconv is not linked, probed for, or compiled on Apple: macOS ships its own libiconv that
# would otherwise be found and silently linked, and the source guards the iconv path behind
# the same condition. That leaves iconv exercised only by the Linux CI, which is the
# accepted cost of decoding classic Mac encodings accurately on the platform that wrote
# most of them.
if(WIN32)
    # MultiByteToWideChar and WideCharToMultiByte are in the standard Win32 libraries.
elseif(APPLE)
    target_link_libraries(finale_mus_reader PRIVATE "-framework CoreFoundation")
else()
    find_package(Iconv)
    if(Iconv_FOUND)
        target_link_libraries(finale_mus_reader PRIVATE Iconv::Iconv)
    else()
        message(FATAL_ERROR
            "iconv is required to convert legacy MUS text to UTF-8 on this platform.")
    endif()
endif()

if(FINALE_MUS_READER_BUILD_TESTING OR FINALE_MUS_READER_BUILD_TOOLS)
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
        finale_mus_reader_keep_out_of_unity(pugixml pugixml-static)
        set(FINALE_MUS_READER_PUGIXML_TARGET pugixml)
    endif()
endif()

if(FINALE_MUS_READER_BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(FINALE_MUS_READER_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
