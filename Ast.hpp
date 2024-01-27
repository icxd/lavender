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

} // namespace ExpressionDetails

struct Expression {
    enum class Kind { Id, Int, String, Call };

    Var<
            ExpressionDetails::Id *,
            ExpressionDetails::Int *,
            ExpressionDetails::String *,
            ExpressionDetails::Call *> var;
};

struct Type {
    enum class Kind { Id, Str, Int, Array };
    Kind type;
    SpannedStr id;
    Type *subtype{}; // only for array
};

namespace PatternDetails {

    struct Wildcard { };
    struct Expression { ::Expression *expr; };
    struct Range { ::Expression *from, *to; bool inclusive; };

} // namespace PatternDetails

struct Pattern {
    enum class Kind { Wildcard, Expression, Range, };

    Var<
            PatternDetails::Wildcard *,
            PatternDetails::Expression *,
            PatternDetails::Range *> var;
    ::Expression *body;
};

class AstPrinter {
public:
    void print(const Vec<Statement *>&);

private:
    static void statement(Statement *);
    static void expression(Expression *);
    static Str type(Type *);
    static void field(Field);
};