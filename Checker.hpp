#pragma once

#include <utility>
#include "Common.hpp"
#include "Ast.hpp"

class CheckedStmt;
class CheckedExpr;

class CheckedField {
    Str m_id;
    Type m_type;
    Opt<CheckedExpr *> m_value;

public:
    CheckedField(Str id, Type type, Opt<CheckedExpr *> value)
        : m_id(std::move(id)), m_type(std::move(type)), m_value(value) {}

    [[nodiscard]] Str id() const { return m_id; }
    [[nodiscard]] Type type() const { return m_type; }
    [[nodiscard]] Opt<CheckedExpr *> value() const { return m_value; }
};

class CheckedType {
    Type m_type;

public:
    explicit CheckedType(Type type) : m_type(std::move(type)) {}
};

class CheckedStmt {
public:
    enum class Type { Object, Fun };

    virtual ~CheckedStmt() = default;

    [[nodiscard]] virtual Type type() const = 0;
};

namespace CheckedStmtDetails {

    class CheckedObject : public CheckedStmt {
        Str m_id;
        Vec<CheckedField> m_fields;
        // TODO: add the rest of the fields and whatever else i need.

    public:
        CheckedObject(Str id, const Vec<CheckedField>& fields)
            : m_id(std::move(id)), m_fields(fields) {}

        [[nodiscard]] Type type() const override { return Type::Object; }
    };

    class CheckedFun : public CheckedStmt {
        Str m_id;
        // TODO: add the rest of the fields and whatever else i need.

    public:
        explicit CheckedFun(Str id) : m_id(std::move(id)) {}

        [[nodiscard]] Type type() const override { return Type::Fun; }
    };

}

class CheckedExpr {
public:
    virtual ~CheckedExpr() = default;
};

namespace CheckedExprDetails {

    class CheckedId : public CheckedExpr {
        Str m_id;

    public:
        explicit CheckedId(Str id) : m_id(std::move(id)) {}
    };

    class CheckedInt : public CheckedExpr {
        int m_value;

    public:
        explicit CheckedInt(int value) : m_value(value) {}
    };

    class CheckedString: public CheckedExpr {
        Str m_value;

    public:
        explicit CheckedString(Str value) : m_value(std::move(value)) {}
    };

    class CheckedCall : public CheckedExpr {
        Unique<CheckedExpr> m_callee;
        Vec<CheckedExpr> m_arguments;

    public:
        CheckedCall(Unique<CheckedExpr> callee, Vec<CheckedExpr> arguments)
                : m_callee(std::move(callee)), m_arguments(std::move(arguments)) {}
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

    ErrorOr<Vec<CheckedStmt *>> check(const Vec<Statement *>&);

private:
    ErrorOr<CheckedStmt *> statement(Statement *);
    ErrorOr<CheckedExpr *> expression(Expression *);
    ErrorOr<::Type> type(::Type *);

    [[nodiscard]] Scope *scope() const;
    void begin_scope();
    void end_scope();

    bool locate_symbol(Scope *, const Str&);
    bool locate_symbol(const Str&);

    template <typename... Args> Error error(const Span &, Args...);

private:
    Map<ScopeId, Scope *> m_scope_stack{};
    ScopeId m_next_scope_id = 0;
};