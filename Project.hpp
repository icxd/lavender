#pragma once

#include "Common.hpp"
#include "Ast.hpp"

using FunctionId = usz;

struct Function {
    SpannedStr id;
    Vec<SpannedStr> parameters;
    Opt<Type *> ret_type;
    Block<Statement *> body;
    bool unsafe{false};
};

struct Project {
    FunctionId current_function_id = UINT64_MAX;
    Vec<Function> functions{};
};