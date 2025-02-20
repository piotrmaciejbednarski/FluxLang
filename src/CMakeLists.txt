# src/CMakeLists.txt - Build configuration for the Flux language source files

# Add include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Define source files for the lexer
set(LEXER_SOURCES
    lexer/lexer.cpp
    lexer/token.hpp
)

# Define source files for the parser
set(PARSER_SOURCES
    parser/parser.cpp
    parser/parser.hpp
    parser/parser_implementations.cpp
    parser/parser_methods.cpp
    parser/ast.hpp
    parser/expressions.hpp
    parser/statements.hpp
)

# Define source files for the interpreter
#set(INTERPRETER_SOURCES
#    interpreter/interpreter.cpp
#    interpreter/environment.hpp
#    interpreter/value.hpp
#)

# Create the flux library
add_library(flux
    ${LEXER_SOURCES}
    ${PARSER_SOURCES}
    forwards.hpp
    main.cpp
)

# Define executable
add_executable(flux_exe main.cpp)
set_target_properties(flux_exe PROPERTIES OUTPUT_NAME "flux")

# Link the flux library to the executable
target_link_libraries(flux_exe flux)

# Specify C++17 standard
set_target_properties(flux flux_exe PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
