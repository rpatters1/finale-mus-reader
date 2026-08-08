if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_HEADER OR NOT DEFINED OUTPUT_SOURCE)
    message(FATAL_ERROR "INPUT_FILE, OUTPUT_HEADER, and OUTPUT_SOURCE are required")
endif()

file(READ "${INPUT_FILE}" EMBEDDED_HEX HEX)
string(LENGTH "${EMBEDDED_HEX}" EMBEDDED_HEX_LENGTH)
math(EXPR EMBEDDED_SIZE "${EMBEDDED_HEX_LENGTH} / 2")
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1," EMBEDDED_BYTES
    "${EMBEDDED_HEX}")

get_filename_component(OUTPUT_DIRECTORY "${OUTPUT_HEADER}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")

file(WRITE "${OUTPUT_HEADER}" [=[
#pragma once

#include <cstddef>
#include <cstdint>

namespace finale_mus_reader {
namespace generated {

extern const std::uint8_t macosDefaultGzip[];
extern const std::size_t macosDefaultGzipSize;

} // namespace generated
} // namespace finale_mus_reader
]=])

file(WRITE "${OUTPUT_SOURCE}"
    "#include \"embedded_default.h\"\n\n"
    "namespace finale_mus_reader {\nnamespace generated {\n\n"
    "const std::uint8_t macosDefaultGzip[] = {${EMBEDDED_BYTES}};\n"
    "const std::size_t macosDefaultGzipSize = ${EMBEDDED_SIZE};\n\n"
    "} // namespace generated\n} // namespace finale_mus_reader\n")
