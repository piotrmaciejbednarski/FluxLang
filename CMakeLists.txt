cmake_minimum_required(VERSION 3.10)
project(flux)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable warnings
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add source files
set(SOURCES
    src/main.cpp
    src/error.cpp
    src/ast.cpp
    src/lexer.cpp
    src/parser.cpp
    #src/builtins.cpp
    #src/typechecker.cpp
    #src/runtime.cpp
    #src/runtime_utils.cpp
    #src/interpreter.cpp
    # Add more source files here as they are created
)

# Add header files
set(HEADERS
    src/include/error.h
    src/include/ast.h
    src/include/lexer.h
    src/include/parser.h
    #src/include/builtins.h
    #src/include/typechecker.h
    #src/include/runtime.h
    #src/include/interpreter.h
    # Add more header files here as they are created
)

# Create executable
add_executable(flux ${SOURCES} ${HEADERS})

# Install target
install(TARGETS flux DESTINATION bin)
