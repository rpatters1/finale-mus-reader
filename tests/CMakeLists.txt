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
# across many translation units. Unity compilation reduces the cost of building those
# upstream files just as it does for this project's test target below.
#
# This is a deliberate choice about one dependency's build time. Its companion target has no
# such reason, so it follows the general rule for code this repository does not own.
set_target_properties(Catch2 PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_BATCH_SIZE 15)
finale_mus_reader_keep_out_of_unity(Catch2WithMain)

add_executable(finale_mus_reader_tests
    reader_tests.cpp
    coverage_schema_tests.cpp
    ../tools/coverage/json.cpp
    classes/options/augmentation_dot_options_tests.cpp
    classes/options/accidental_options_tests.cpp
    classes/options/alternate_notation_options_tests.cpp
    classes/options/barline_options_tests.cpp
    classes/options/beam_options_tests.cpp
    classes/options/clef_options_tests.cpp
    classes/options/flag_options_tests.cpp
    classes/options/font_options_tests.cpp
    classes/options/grace_note_options_tests.cpp
    classes/options/key_signature_options_tests.cpp
    classes/options/line_curve_options_tests.cpp
    classes/others/fretboard_tests.cpp
    classes/others/graphic_assignments_tests.cpp
    classes/common/legacy_record_tests.cpp
    classes/options/lyric_options_tests.cpp
    classes/options/misc_options_tests.cpp
    classes/options/music_symbol_options_tests.cpp
    classes/options/note_rest_options_tests.cpp
    classes/options/page_format_options_tests.cpp
    classes/common/mapping_framework_tests.cpp
    classes/details/measure_graphic_assign_tests.cpp
    classes/options/multimeasure_rest_options_tests.cpp
    classes/options/piano_brace_bracket_options_tests.cpp
    classes/options/repeat_options_tests.cpp
    classes/others/shape_definitions_tests.cpp
    classes/others/smart_shape_custom_lines_tests.cpp
    classes/options/smart_shape_options_tests.cpp
    classes/options/staff_options_tests.cpp
    classes/options/stem_options_tests.cpp
    classes/others/text_blocks_tests.cpp
    classes/options/text_options_tests.cpp
    classes/options/tie_options_tests.cpp
    text_encoding_tests.cpp
    classes/texts/text_pool_tests.cpp)
finale_mus_reader_enable_unity(finale_mus_reader_tests)

target_link_libraries(finale_mus_reader_tests PRIVATE
    finale_mus_reader::finale_mus_reader
    Catch2::Catch2WithMain
    ${FINALE_MUS_READER_PUGIXML_TARGET}
    ZLIB::ZLIB)

# The class import framework is internal, so its tests reach into the private source tree
# rather than driving every case through a synthetic MUS file.
target_include_directories(finale_mus_reader_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/classes/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tools")

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
