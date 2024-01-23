#pragma once

#include "Common.hpp"
#include "Ast.hpp"

enum class SymbolType {
    Object,
    Function,
    Variable,
    Unknown
};

struct Scope {
    Map<Str, SymbolType> symbol_table;
//    Map<Str, Object *> object_table;
};

class Checker {
public:
    Checker() noexcept;

    ErrorOr<Void> check(Vec<Stmt *>);

private:
    ErrorOr<Void> statement(Stmt *);
    ErrorOr<Type> expression(Expr *);
    ErrorOr<Type> type(Type *);

    void begin_scope();
    void end_scope();

    ErrorOr<SymbolType> lookup_symbol(const SpannedStr&);
    ErrorOr<SymbolType> lookup_symbol(Scope *, const SpannedStr&);

    template <typename... Args> Error error(const Span &, Args...);

private:
    Scope *m_global_scope{};
    Scope *m_top{};
    Vec<Scope *> m_scope_stack{};
};