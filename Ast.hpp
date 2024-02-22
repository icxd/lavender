#pragma once

#include <format>
#include "Common.hpp"

struct ParsedStatement;
struct Expression;
struct Type;
struct Pattern;

template <typename T> struct Block { Vec<T> elems; };

struct ParsedField {
    Type *type;
    SpannedStr id;
    Opt<::Expression *> value;
};

struct ParsedMethod {
    SpannedStr id;
    Vec<ParsedField> parameters;
    Opt<Type *> ret_type;
    Block<ParsedStatement *> body;
    bool unsafe{false};
    bool static_{false};
};

struct ParsedObject {
    SpannedStr id;
    Vec<Type *> generic_params;
    Opt<SpannedStr> parent;
    Vec<SpannedStr> interfaces;
    Vec<ParsedField> fields;
    Vec<ParsedMethod> methods;
};

struct ParsedInterface {
    SpannedStr id;
    Vec<SpannedStr> interfaces;
    Vec<ParsedMethod> methods;
};

struct ParsedVariable {
    Type *type;
    SpannedStr id;
    ::Expression *expr;
};

struct ParsedFunction {
    SpannedStr id;
    Vec<ParsedField> parameters;
    Opt<Type *> ret_type;
    Block<ParsedStatement *> body;
    bool unsafe{false};
};

struct ParsedReturn { Span span{}; Opt<::Expression *> value; };
struct ParsedExpression { ::Expression *expr; };

struct ParsedStatement {
    enum class Kind { Object, Interface, Fun, Var, Return, Expr };

    Var<
            ParsedObject *,
            ParsedInterface *,
            ParsedFunction *,
            ParsedVariable *,
            ParsedReturn *,
            ParsedExpression *> var;
};

struct ParsedNamespace {
    Opt<Str> name;
    Vec<ParsedFunction *> functions;
    Vec<ParsedObject *> objects;
    Vec<ParsedNamespace *> namespaces;
};

struct Argument {
    Opt<SpannedStr> id;
    ::Expression *expr;
};

namespace ExpressionDetails {

    struct Null { Span span;};
    struct Id { SpannedStr id; };
    struct Int { Spanned<int> value; };
    struct String { SpannedStr value; };
    struct Call {
        Span span;
        ::Expression *callee;
        Vec<Type *> generic_params;
        Vec<Argument> arguments;
    };
    struct Index {
        ::Expression *expr;
        ::Expression *index;
    };
    struct GenericInstance {
        // For example, if you have an object `object Foo[A]: ...`
        // then if you write `Foo[int]`, that would be an instance
        ::Expression *expr;
        Vec<Type *> generic_args;
    };
    struct Unary {
        enum class Operation { Dereference, AddressOf };

        Operation operation;
        ::Expression *value;
    };

    struct Binary {
        enum class Operation { Equals };

        Operation operation;
        ::Expression *left, *right;
    };

    struct If {
        ::Expression *condition;
        ::Expression *then;
        ::Expression *else_;
    };

    struct Access {
        ::Expression *expr;
        ::Expression *member;
    };

    struct Switch {
        ::Expression *condition;
        Vec<Pattern *> patterns;
        Pattern *default_pattern;
    };

    struct UnsafeBlock { Block<::Expression *> body; };

} // namespace ExpressionDetails

struct Expression {
    enum class Kind {
        Null,
        Id,
        Int,
        String,
        Call,
        Index,
        GenericInstance,
        Unary,
        Binary,
        If,
        Access,
        Switch,
        UnsafeBlock
    };

    Var<
            ExpressionDetails::Null *,
            ExpressionDetails::Id *,
            ExpressionDetails::Int *,
            ExpressionDetails::String *,
            ExpressionDetails::Call *,
            ExpressionDetails::Index *,
            ExpressionDetails::GenericInstance *,
            ExpressionDetails::Unary *,
            ExpressionDetails::Binary *,
            ExpressionDetails::If *,
            ExpressionDetails::Access *,
            ExpressionDetails::Switch *,
            ExpressionDetails::UnsafeBlock *> var;

    [[nodiscard]] Span span() const {
        switch (static_cast<Kind>(var.index())) {
#define CASE(ID) case Kind::ID: { \
            auto *expr = std::get<ExpressionDetails::ID *>(var);
            CASE(Null) return expr->span; }
            CASE(Id) return expr->id.span; }
            CASE(Int) return expr->value.span; }
            CASE(String) return expr->value.span; }
            CASE(Call) return expr->span; }
            CASE(Index) return expr->expr->span(); }
            CASE(GenericInstance) return expr->expr->span(); }
            CASE(Unary) return expr->value->span(); }
            CASE(Binary) return expr->left->span(); }
            CASE(If) return expr->condition->span(); }
            CASE(Access) return expr->expr->span(); }
            CASE(Switch) return expr->condition->span(); }
            CASE(UnsafeBlock) return expr->body.elems[0]->span(); }
#undef CASE
        }
    }
};

struct Type {
    enum class Kind { Undetermined, Id, Str, Int, Array, Weak, Raw, Optional, Generic };
    Kind type;
    SpannedStr id;
    Type *subtype{}; // only for array, weak- and raw pointers, optional
    Vec<Type *> generic_args;

    bool operator==(const Type& other) const {
        // TODO: this is sort of a hack i guess to get around not having type inference maybe?
        if (type == Kind::Undetermined || other.type == Kind::Undetermined) return true;

        if (type != other.type) return false;
        if (type == Kind::Id) return id.value == other.id.value;
        if (type == Kind::Array) return subtype == other.subtype;
        if (type == Kind::Weak) return subtype == other.subtype;
        if (type == Kind::Raw) return subtype == other.subtype;
        if (type == Kind::Optional) return subtype == other.subtype;
        if (type == Kind::Generic) {
            if (id.value != other.id.value) return false;
            if (generic_args.size() != other.generic_args.size()) return false;
            for (usz i = 0; i < generic_args.size(); ++i) {
                if (*generic_args[i] != *other.generic_args[i]) return false;
            }
        }
        return true;
    }

    bool operator!=(const Type& other) const { return not (*this == other); }

    static Str repr(Type type) {
        switch (type.type) {
            case Kind::Undetermined: return "<?>";
            case Kind::Id: return type.id.value;
            case Kind::Str: return "str";
            case Kind::Int: return "int";
            case Kind::Array: return std::format("[{}]", Type::repr(*type.subtype));
            case Kind::Weak: return std::format("weak {}", Type::repr(*type.subtype));
            case Kind::Raw: return std::format("raw {}", Type::repr(*type.subtype));
            case Kind::Optional: return std::format("{}?", Type::repr(*type.subtype));
            case Kind::Generic: {
                Str repr = type.id.value;
                repr += "[";
                for (usz i = 0; i < type.generic_args.size(); ++i) {
                    repr += Type::repr(*type.generic_args[i]);
                    if (i != type.generic_args.size() - 1) repr += ", ";
                }
                repr += "]";
                return repr;
            }
        }
    }
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
    void print(const Vec<ParsedStatement *>&);

private:
    static void statement(ParsedStatement *);
    static void expression(Expression *, bool = true);
    static Str type(Type *);
    static void field(ParsedField);
    static void method(ParsedMethod);
};