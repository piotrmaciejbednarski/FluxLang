# tests/CMakeLists.txt - Build configuration for Flux language tests

# Include source directory for headers
include_directories(${CMAKE_SOURCE_DIR}/src)

# Add lexer tests
add_executable(lexer_tests lexer_tests.cpp)
target_link_libraries(lexer_tests flux)
add_test(NAME LexerTests COMMAND lexer_tests)

# Add parser tests
add_executable(parser_tests parser_tests.cpp)
target_link_libraries(parser_tests flux)
add_test(NAME ParserTests COMMAND parser_tests)

# Add interpreter tests
#add_executable(interpreter_tests interpreter_tests.cpp)
#target_link_libraries(interpreter_tests flux)
#add_test(NAME InterpreterTests COMMAND interpreter_tests)

# Specify C++17 standard for tests
set_target_properties(
    lexer_tests 
    parser_tests 
    #interpreter_tests 
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Define simple test for example files
file(GLOB EXAMPLE_FILES "${CMAKE_SOURCE_DIR}/examples/*.fx")
foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    add_test(
        NAME "Example_${EXAMPLE_NAME}"
        COMMAND flux_exe ${EXAMPLE_FILE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach()
