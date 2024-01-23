#include "Common.hpp"
#include "Codegen.hpp"

static Unique<LLVMContext> s_context;
static Unique<IRBuilder<>> s_builder(s_context);
static Unique<Module> s_module;
static Map<Str, Value *> s_named_values;

Value *CheckedIntExpr::generate() {
    return ConstantFP::get(*s_context, APSInt(m_value));
}