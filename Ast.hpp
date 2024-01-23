#pragma once

#include "Common.hpp"

struct Stmt;
struct Expr;
struct Type;

struct Field {
    Type *type;
    SpannedStr id;
    Opt<::Expr *> value;
};

template <typename T> struct Block { Vec<T> elems; };

namespace StmtDetails {

struct Object {
    SpannedStr id;
    Opt<SpannedStr> parent;
    Vec<SpannedStr> implements;
    Vec<Field> fields;
};

struct VarDecl {
    Type *type;
    SpannedStr id;
    ::Expr *expr;
};

struct FunDecl {
    SpannedStr id;
    Vec<Field> parameters;
    Opt<Type *> ret_type;
    Block<Stmt *> body;
};

struct Return { Opt<::Expr *> value; };
struct Expr { ::Expr *expr; };

} // namespace StmtDetails

struct Stmt {
    enum class Kind { Object, Fun, Var, Return, Expr };

    Var<
            StmtDetails::Object *,
            StmtDetails::FunDecl *,
            StmtDetails::VarDecl *,
            StmtDetails::Return *,
            StmtDetails::Expr *> var;
};

struct Argument {
    Opt<SpannedStr> id;
    ::Expr *expr;
};

namespace ExprDetails {

struct Id { SpannedStr id; };
struct Int { int value; };
struct String { SpannedStr value; };
struct Call {
    ::Expr *callee;
    Vec<Argument> arguments;
};

} // namespace ExprDetails

struct Expr {
    enum class Kind { Id, Int, String, Call };

    Var<
            ExprDetails::Id *,
            ExprDetails::Int *,
            ExprDetails::String *,
            ExprDetails::Call *> var;
};

struct Type {
    enum class Kind { Id, Str, Int, Array };
    Kind type;
    SpannedStr id;
    Type *subtype; // only for array
};

class AstPrinter {
public:
    void print(const Vec<Stmt *>&);

private:
    static void statement(Stmt *);
    static void expression(Expr *);
    static Str type(Type *);
    static void field(Field);
};