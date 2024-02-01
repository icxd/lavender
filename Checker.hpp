#pragma once

#include <utility>
#include "Common.hpp"
#include "Ast.hpp"
#include "Project.hpp"

enum class SafetyContext { Safe, Unsafe };

using ScopeId = usz;

struct Scope {
    ScopeId id, parent_id;
    SafetyContext context{SafetyContext::Safe};
    Vec<Str> symbols;

    Map<Str, FunctionId> functions;
};

class Checker {
public:
    explicit Checker(Project *project) noexcept;

    ErrorOr<Void> check(const Vec<Statement *>&);

private:
    ErrorOr<Void> check_statement(Statement *);
    ErrorOr<Type> check_expression(Expression *);
    ErrorOr<Type> check_type(Type *);

    [[nodiscard]] Scope *scope() const;
    void begin_scope();
    void end_scope();

    template <typename... Args> Error error(const Span &, Args...);

private:
    Project *m_project;
    Map<ScopeId, Scope *> m_scope_stack{};
    ScopeId m_next_scope_id = 0;
};