# On-demand corpus tools. Not built by default and not part of any test: they need a
# corpus that only their operator has.
#
# tools/coverage/       -- the single, always-current probe covering every class the
#                           reader recovers, used to catch regressions across a corpus
#                           run. Registry-driven (see coverage/registry.h) so adding a
#                           class means adding one file, not editing this probe.
# tools/investigations/ -- one-off, question-driven probes, often below the record
#                           layer. To add one, drop a .cpp here and add a line below. A
#                           probe that reads through the public reader needs the pugixml
#                           document type; one that works at the record layer needs the
#                           private src/ headers instead.

function(finale_mus_reader_add_tool name)
    cmake_parse_arguments(TOOL "PRIVATE_HEADERS;NEEDS_PUGIXML" "" "" ${ARGN})
    add_executable(${name} investigations/${name}.cpp)
    target_link_libraries(${name} PRIVATE finale_mus_reader::finale_mus_reader)
    if(TOOL_NEEDS_PUGIXML)
        target_link_libraries(${name} PRIVATE ${FINALE_MUS_READER_PUGIXML_TARGET})
    endif()
    if(TOOL_PRIVATE_HEADERS)
        # Probes that observe stored records rather than imported objects reach
        # into the same private headers the library builds against.
        target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/src")
    endif()
    # Deliberately not -Werror: a probe is meant to be edited quickly while a
    # question is live, and a warning should not stop it running.
endfunction()

finale_mus_reader_add_tool(clef_options_probe PRIVATE_HEADERS)
finale_mus_reader_add_tool(font_header_words PRIVATE_HEADERS)
finale_mus_reader_add_tool(globals_census PRIVATE_HEADERS)
finale_mus_reader_add_tool(record_dump PRIVATE_HEADERS)
finale_mus_reader_add_tool(text_pool_probe PRIVATE_HEADERS)
finale_mus_reader_add_tool(graphics_record_probe PRIVATE_HEADERS)

add_subdirectory(coverage)
