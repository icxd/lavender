#pragma once

#include <llvm/IR/Value.h>
#include "Common.hpp"
#include "Ast.hpp"

using namespace llvm;

class CheckedStmt {
public:
    virtual ~CheckedStmt() = default;
    virtual Value *generate() = 0;
};

class CheckedExpr {
public:
    virtual ~CheckedExpr() = default;
    virtual Value *generate() = 0;
};

namespace CheckedExprDetails {

    class CheckedIdExpr : public CheckedExpr {
        Str m_id;

    public:
        explicit CheckedIdExpr(Str id) : m_id(std::move(id)) {}
        Value *generate() override;
    };

    class CheckedIntExpr : public CheckedExpr {
        int m_value;

    public:
        explicit CheckedIntExpr(int value) : m_value(value) {}
        Value *generate() override;
    };

    class CheckedStringExpr : public CheckedExpr {
        Str m_value;

    public:
        explicit CheckedStringExpr(Str value) : m_value(std::move(value)) {}
        Value *generate() override;
    };

    class CheckedCallExpr : public CheckedExpr {
        Unique<CheckedExpr> m_callee;
        Vec<CheckedExpr> m_arguments;

    public:
        CheckedCallExpr(Unique<CheckedExpr> callee, Vec<CheckedExpr> arguments)
                : m_callee(std::move(callee)), m_arguments(std::move(arguments)) {}
        Value *generate() override;
    };

}

using ScopeId = usz;

struct Scope {
    ScopeId id, parent_id;
    Vec<Str> symbols;
};

class Checker {
public:
    Checker() noexcept;

    ErrorOr<Void> check(const Vec<Stmt *>&);

private:
    ErrorOr<CheckedStmt *> statement(Stmt *);
    ErrorOr<CheckedExpr *> expression(Expr *);
    ErrorOr<::Type> type(::Type *);

    void begin_scope();
    void end_scope();

    bool locate_symbol(Scope *, Str);
    bool locate_symbol(Str);

    template <typename... Args> Error error(const Span &, Args...);

private:
    Map<ScopeId, Scope *> m_scopes;
    ScopeId m_next_scope_id;
};