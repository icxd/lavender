#include "Common.hpp"
#include "Codegen.hpp"
#include "Checker.hpp"
#include <iostream>
#include <sstream>

static LLVMContext s_context;
static IRBuilder<> s_builder(s_context);
static Unique<Module> s_module;
static Map<Str, Value *> s_named_values;
static usz str_count = 0;

namespace CheckedExprDetails {

    Value *CheckedIdExpr::generate() {
        Value *id = s_named_values[m_id];
        if (not id)
            std::cerr << "COMPILER ERROR: id `" << m_id << "` USED BUT NOT DEFINED FOR CODEGEN";
        return id;
    }

    Value *CheckedIntExpr::generate() { return ConstantInt::get(s_context, APInt(32, m_value)); }

    Value *CheckedStringExpr::generate() {
        std::stringstream ss;
        ss << "__str_" << str_count;
        return s_builder.CreateGlobalString(m_value, ss.str(), 0, s_module.get());
    }

}
