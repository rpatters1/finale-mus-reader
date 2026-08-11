include(FetchContent)

set(CATCH_BUILD_TESTING OFF CACHE BOOL "Do not build Catch2 tests" FORCE)
set(CATCH_BUILD_EXAMPLES OFF CACHE BOOL "Do not build Catch2 examples" FORCE)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "Do not install Catch2 documentation" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "Do not install Catch2 extras" FORCE)
FetchContent_Declare(Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.15.3.tar.gz
    URL_HASH SHA256=b0299ae552918220a7a6e21e7de5b714777f4e8c883fb70c4bb23fe01df8c6e3
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(Catch2)

# Catch2 is intentionally consumed as a normal library, but its implementation is split
# across many translation units. Unity compilation keeps those upstream files together
# without merging this project's independent test files, where separate diagnostics and
# translation-unit boundaries are useful.
set_target_properties(Catch2 PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_BATCH_SIZE 15)

add_executable(finale_mus_reader_tests reader_tests.cpp mapping_tests.cpp text_encoding_tests.cpp)

target_link_libraries(finale_mus_reader_tests PRIVATE
    finale_mus_reader::finale_mus_reader
    Catch2::Catch2WithMain
    ${FINALE_MUS_READER_PUGIXML_TARGET}
    ZLIB::ZLIB)

# The mapping framework is internal, so its tests reach into the private source tree
# rather than driving every case through a synthetic MUS file.
target_include_directories(finale_mus_reader_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src")

target_compile_definitions(finale_mus_reader_tests PRIVATE
    FINALE_MUS_READER_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
    target_compile_options(finale_mus_reader_tests PRIVATE /W4 /WX)
else()
    target_compile_options(finale_mus_reader_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(finale_mus_reader_tests)
