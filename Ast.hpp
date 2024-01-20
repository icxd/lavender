#pragma once

#include "Common.hpp"

struct Stmt;
struct Expr;
struct Type;

struct Field {
    Type *type;
    Str id;
    Opt<::Expr *> value;
};

template <typename T> struct Block { Vec<T> elems; };

namespace StmtDetails {

struct Object {
    Str id;
    Opt<Str> parent;
    Vec<Str> implements;
    Vec<Field> fields;
};

struct VarDecl {
    Type *type;
    Str id;
    ::Expr *expr;
};

struct FunDecl {
    Str id;
    Vec<Field> parameters;
    Opt<Type *> ret_type;
    Block<Stmt *> body;
};

struct Return { Opt<::Expr *> value; };
struct Expr { ::Expr *expr; };

} // namespace StmtDetails

struct Stmt {
    Var<
            StmtDetails::Object *,
            StmtDetails::FunDecl *,
            StmtDetails::VarDecl *,
            StmtDetails::Return *,
            StmtDetails::Expr *> var;
};

namespace ExprDetails {

struct Int { int value; };
struct String { Str value; };

} // namespace ExprDetails

struct Expr {
    Var<ExprDetails::Int *, ExprDetails::String *> var;
};

struct Type {
    enum class Kind { Str, Int, Array };
    Kind type;
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