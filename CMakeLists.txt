cmake_minimum_required(VERSION 3.14)
project(Flux VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    src/output/writer.h
    src/output/writer.cpp
    src/common/error.h
    src/common/error.cpp
    src/common/arena.h
    src/common/arena.cpp
    src/common/source.h
    src/common/source.cpp
    src/lexer/tokenizer.cpp
    src/lexer/tokenizer.h
    src/lexer/token.cpp
    src/lexer/token.h
    src/parser/ast.h
    src/parser/ast.cpp
    src/parser/parser.h
    src/parser/parser.cpp
)

# Source files for the runtime
set(RUNTIME_SOURCES
    #src/runtime/fluxvalue.cpp
    #src/runtime/fluxobject.cpp
    #src/runtime/fluxclass.cpp
    #src/runtime/interpreter.cpp
)

# Source files for utils
set(UTILS_SOURCES
    #src/utils/stringutils.cpp
)

# Main source file
set(MAIN_SOURCE src/main.cpp)

# Add the executable
add_executable(Flux ${MAIN_SOURCE} ${SOURCES} ${PARSER_SOURCES} ${RUNTIME_SOURCES} ${UTILS_SOURCES})