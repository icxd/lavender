cmake_minimum_required(VERSION 3.26)
project(compiler)

set(CMAKE_CXX_STANDARD 23)

include_directories(.)

add_executable(compiler
        Ast.hpp
        Common.hpp
        main.cpp
        Parser.cpp
        Parser.hpp
        Token.hpp
        Tokenizer.cpp
        Tokenizer.hpp
        Ast.cpp
        Ast.cpp)
