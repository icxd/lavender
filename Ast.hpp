#pragma once

#include "Common.hpp"

struct Statement;
struct Expression;
struct Type;
struct Pattern;

struct Field {
    Type *type;
    SpannedStr id;
    Opt<::Expression *> value;
};

template <typename T> struct Block { Vec<T> elems; };

namespace StatementDetails {

    struct Object {
        SpannedStr id;
        Opt<SpannedStr> parent;
        Vec<SpannedStr> implements;
        Vec<Field> fields;
    };

    struct VarDecl {
        Type *type;
        SpannedStr id;
        ::Expression *expr;
    };

    struct FunDecl {
        SpannedStr id;
        Vec<Field> parameters;
        Opt<Type *> ret_type;
        Block<Statement *> body;
        bool unsafe{false};
    };

    struct Return { Opt<::Expression *> value; };
    struct Expression { ::Expression *expr; };

} // namespace StatementDetails

struct Statement {
    enum class Kind { Object, Fun, Var, Return, Expr };

    Var<
            StatementDetails::Object *,
            StatementDetails::FunDecl *,
            StatementDetails::VarDecl *,
            StatementDetails::Return *,
            StatementDetails::Expression *> var;
};

struct Argument {
    Opt<SpannedStr> id;
    ::Expression *expr;
};

namespace ExpressionDetails {

    struct Id { SpannedStr id; };
    struct Int { int value; };
    struct String { SpannedStr value; };
    struct Call {
        ::Expression *callee;
        Vec<Argument> arguments;
    };

    struct Switch {
        ::Expression *condition;
        Vec<Pattern *> patterns;
        Pattern *default_pattern;
    };

    struct UnsafeBlock { Block<::Expression *> body; };

} // namespace ExpressionDetails

struct Expression {
    enum class Kind { Id, Int, String, Call, Switch, UnsafeBlock };

    Var<
            ExpressionDetails::Id *,
            ExpressionDetails::Int *,
            ExpressionDetails::String *,
            ExpressionDetails::Call *,
            ExpressionDetails::Switch *,
            ExpressionDetails::UnsafeBlock *> var;
};

struct Type {
    enum class Kind { Id, Str, Int, Array };
    Kind type;
    SpannedStr id;
    Type *subtype{}; // only for array

    bool operator==(const Type& other) const {
        if (type != other.type) return false;
        if (type == Kind::Id) return id.value == other.id.value;
        if (type == Kind::Array) return subtype == other.subtype;
        return true;
    }

    bool operator!=(const Type& other) const { return not (*this == other); }
};

enum class PatternUnaryOperation {
    LessThan,
    GreaterThan,
};

namespace PatternDetails {

    struct Wildcard { };
    struct Expression { ::Expression *expr; };
    struct Range { ::Expression *from, *to; bool inclusive; };
    struct Unary { ::Expression *value; PatternUnaryOperation operation; };

} // namespace PatternDetails

using PatternCondition = Var<
        PatternDetails::Wildcard *,
        PatternDetails::Expression *,
        PatternDetails::Range *,
        PatternDetails::Unary *>;

struct Pattern {
    enum class Kind { Wildcard, Expression, Range, Unary };

    PatternCondition condition;
    ::Expression *body;
};

class AstPrinter {
public:
    void print(const Vec<Statement *>&);

private:
    static void statement(Statement *);
    static void expression(Expression *, bool = true);
    static Str type(Type *);
    static void field(Field);
};