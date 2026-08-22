# The one probe that must never regress: it walks every class the reader recovers, so it
# must stay registry-driven rather than repeating itself the way the rest of tools/ may.
# Adding a class means adding one file under surveyors/<pool>/ (options, others, details,
# texts -- matching src/import/) and nothing else; see registry.h.

file(GLOB_RECURSE COVERAGE_SURVEYOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/surveyors/*.cpp")

add_executable(recovery_coverage_probe
    recovery_coverage_probe.cpp
    registry.cpp
    context.cpp
    json.cpp
    support/musx_companion.cpp
    support/relationships.cpp
    ${COVERAGE_SURVEYOR_SOURCES}
)
target_link_libraries(recovery_coverage_probe PRIVATE
    finale_mus_reader::finale_mus_reader
    ${FINALE_MUS_READER_PUGIXML_TARGET}
    ${FINALE_MUS_READER_ZLIB_TARGET}
)
target_include_directories(recovery_coverage_probe PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/support")
# Lets every surveyor write #include "coverage/registry.h" regardless of how deep it sits
# under surveyors/, mirroring how src/import/ files include "import/options.h".
target_include_directories(recovery_coverage_probe PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
