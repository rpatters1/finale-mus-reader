add_executable(finale_mus_reader_tests reader_tests.cpp)

target_link_libraries(finale_mus_reader_tests PRIVATE
    finale_mus_reader::finale_mus_reader
    ${FINALE_MUS_READER_TEST_PUGIXML_TARGET}
    ZLIB::ZLIB)

target_compile_definitions(finale_mus_reader_tests PRIVATE
    FINALE_MUS_READER_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
    target_compile_options(finale_mus_reader_tests PRIVATE /W4 /WX)
else()
    target_compile_options(finale_mus_reader_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME finale_mus_reader_tests COMMAND finale_mus_reader_tests)
