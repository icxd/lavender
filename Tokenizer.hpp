#pragma once

#include "Common.hpp"
#include "Token.hpp"

struct TokenizeResult {
    Vec<Token> tokens;
    Vec<Error> errors;
};

TokenizeResult tokenize(const char *filename, Str source);
Vec<Token> normalize(Vec<Token> tokens);