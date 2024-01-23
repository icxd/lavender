cmake_minimum_required(VERSION 3.26)
project(compiler)

set(CMAKE_CXX_STANDARD 23)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(.)

add_executable(compiler
        Common.hpp
        main.cpp
        Parser.cpp
        Parser.hpp
        Token.hpp
        Tokenizer.cpp
        Tokenizer.hpp
        Ast.hpp
        Ast.cpp
        Checker.cpp
        Checker.hpp
        Codegen.hpp
        Codegen.cpp)

llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(compiler ${llvm_libs})