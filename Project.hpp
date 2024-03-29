#pragma once

#include "Common.hpp"
#include "Ast.hpp"

class Project;

enum : usz {
    UNKNOWN_TYPE_ID = 0,
    UNIT_TYPE_ID,
    BOOL_TYPE_ID,
    INT_TYPE_ID,
    UINT_TYPE_ID,
    FLOAT_TYPE_ID,
    STRING_TYPE_ID,
};

using RecordId = usz;
using FunctionId = usz;
using ScopeId = usz;
using TypeId = usz;

enum class SafetyContext { Safe, Unsafe };

struct CheckedType {
    enum class Tag { Builtin, TypeVariable, GenericInstance, Record, RawPtr };

    Tag tag{};
    struct { Str variable; } type_variable;
    struct { RecordId record_id; Vec<TypeId> generic_arguments; } generic_instance;
    struct { RecordId record_id; } record{};
    struct { TypeId subtype; } rawptr{};

    static CheckedType Builtin() { return CheckedType{Tag::Builtin}; }

    static CheckedType TypeVariable(Str var) {
        return CheckedType{.tag = Tag::TypeVariable, .type_variable = {var}};
    }

    static CheckedType GenericInstance(RecordId id, Vec<TypeId> types) {
        return CheckedType{.tag = Tag::GenericInstance, .generic_instance = {id, types}};
    }

    static CheckedType Record(RecordId id) {
        return CheckedType{.tag = Tag::Record, .record = {id}};
    }

    static CheckedType RawPtr(TypeId subtype) {
        return CheckedType{.tag = Tag::RawPtr, .rawptr = {subtype}};
    }

    bool operator==(const CheckedType& other) const { return this->tag == other.tag; }
    bool operator!=(const CheckedType& other) const { return this->tag != other.tag; }
};

struct CheckedVarDecl {
    Str name;
    TypeId type_id;
    Span span;
};

struct CheckedVariable {
    Str name;
    TypeId type_id;
};

struct CheckedRecord {
    Str name;
    Vec<TypeId> generic_parameters;
    Vec<CheckedVarDecl> fields;
    ScopeId scope_id;
};

struct CheckedParameter {
    bool requires_label{};
    CheckedVariable variable;
};

struct CheckedStatement;

struct CheckedBlock {
    Vec<CheckedStatement *> statements;
};

struct CheckedFunction {
    Str name;
    // TODO: visibility (such as public and private)
    TypeId return_type_id;
    Vec<CheckedParameter> parameters;
    Vec<TypeId> generic_parameters;
    ScopeId scope_id;
    CheckedBlock block;
};

// Might need to be a tagged union later…
enum CheckedUnaryOperator {
    Dereference,
    AddressOf,
};

struct CheckedExpression {
    enum class Tag {
        Null,
        Int,
        String,
        Var,
        If,
        BinaryOp,
        UnaryOp,
    };

    Tag tag{};

    struct { TypeId type_id; } null;
    struct { Spanned<int> value; } integer;
    struct { Spanned<Str> value; } string;
    struct { Spanned<CheckedVariable> var; } var;
    struct { CheckedExpression *condition, *then, *else_; } if_;
    struct {
        CheckedExpression *left;
        ExpressionDetails::Binary::Operation op;
        CheckedExpression *right;
        Span span;
        TypeId type_id;
    } binary_op;
    struct UnaryOp {
        CheckedExpression *left{};
        CheckedUnaryOperator op{};
        Span span{};
        TypeId type_id;
    } unary_op;

    static CheckedExpression Null(TypeId type_id) {
        return CheckedExpression{.tag = Tag::Null, .null = {type_id}};
    }

    static CheckedExpression Int(Spanned<int> value) {
        return CheckedExpression{.tag=Tag::Int, .integer={value}};
    }

    static CheckedExpression String(Spanned<Str> value) {
        return CheckedExpression{.tag=Tag::String, .string={value}};
    }

    static CheckedExpression Var(Spanned<CheckedVariable> var) {
        return CheckedExpression{.tag=Tag::Var, .var={var}};
    }

    static CheckedExpression If(CheckedExpression *cond, CheckedExpression *then, CheckedExpression *else_) {
        return CheckedExpression{.tag=Tag::If, .if_={cond, then, else_}};
    }

    static CheckedExpression BinaryOp(
            CheckedExpression *left,
            ExpressionDetails::Binary::Operation op,
            CheckedExpression *right,
            Span span,
            TypeId type_id
    ) {
        return CheckedExpression{.tag=Tag::BinaryOp, .binary_op={left, op, right, span, type_id}};
    }

    static CheckedExpression UnaryOp(
            CheckedExpression *left,
            CheckedUnaryOperator op,
            Span span,
            TypeId type_id
    ) {
        return CheckedExpression{.tag=Tag::UnaryOp, .unary_op={left, op, span, type_id}};
    }

    [[nodiscard]] TypeId type_id() const {
        switch (this->tag) {
            case Tag::Null: return this->null.type_id;
            case Tag::Int: return INT_TYPE_ID;
            case Tag::String: return STRING_TYPE_ID;
            case Tag::Var: return this->var.var.value.type_id;
            case Tag::If: return this->if_.condition->type_id();
            case Tag::BinaryOp: return this->binary_op.type_id;
            case Tag::UnaryOp: return this->unary_op.left->type_id();
        }
    }
};

struct CheckedStatement {
    enum class Tag {
        Expression,
        VarDecl,
        Return,
    };

    Tag tag{};

    struct { CheckedExpression *expr; } expression;
    struct { CheckedVarDecl decl; CheckedExpression *expr{}; } var_decl;
    struct { CheckedExpression *expr{}; } return_;

    static CheckedStatement Expression(CheckedExpression *expr) {
        return CheckedStatement{.tag=Tag::Expression, .expression={expr}};
    }

    static CheckedStatement VarDecl(CheckedVarDecl decl, CheckedExpression *expr) {
        return CheckedStatement{.tag=Tag::VarDecl, .var_decl={decl, expr}};
    }

    static CheckedStatement Return(CheckedExpression *expr) {
        return CheckedStatement{.tag=Tag::Return, .return_={expr}};
    }
};

struct Scope {
public:
    explicit Scope(Opt<ScopeId> parent = std::nullopt)
            : parent(parent) {}

    static bool can_access(ScopeId, ScopeId, const Project &);

public:
    Opt<Str> namespace_name = std::nullopt;
    Vec<CheckedVariable> variables{};
    Vec<Id<RecordId>> records{};
    Vec<Id<FunctionId>> functions{};
    Vec<Id<TypeId>> types{};
    Opt<ScopeId> parent = std::nullopt;
    Vec<ScopeId> children{};
};

class Project {
public:
    Project() {
        auto *project_global_scope = new Scope();
        this->scopes.push_back(project_global_scope);
    }

    TypeId find_or_add_type_id(const CheckedType&);
    ScopeId create_scope(ScopeId);
    ErrorOr<Void> add_var_to_scope(ScopeId, const CheckedVariable&, Span);
    Opt<CheckedVariable> find_var_in_scope(ScopeId, const Str&);
    ErrorOr<Void> add_type_to_scope(ScopeId, Str, TypeId, Span);
    Opt<TypeId> find_type_in_scope(ScopeId, const Str&);
    ErrorOr<Void> add_function_to_scope(ScopeId, Str, FunctionId, Span);
    Opt<FunctionId> find_function_in_scope(ScopeId, const Str&);
    ErrorOr<Void> add_record_to_scope(ScopeId, Str, RecordId, Span);
    Opt<RecordId> find_record_in_scope(ScopeId, const Str&);

    Str typename_for_type_id(TypeId type_id) {
        switch (this->types[type_id].tag) {
            case CheckedType::Tag::Builtin:
                switch (type_id) {
                    case UNIT_TYPE_ID: return "unit";
                    case BOOL_TYPE_ID: return "bool";
                    case INT_TYPE_ID: return "int";
                    case UINT_TYPE_ID: return "uint";
                    case FLOAT_TYPE_ID: return "float";
                    case STRING_TYPE_ID: return "str";
                    default: return "<invalid>";
                }
            case CheckedType::Tag::TypeVariable: return this->types[type_id].type_variable.variable;
            case CheckedType::Tag::GenericInstance: {
                Str output = this->records[this->types[type_id].generic_instance.record_id].name;
                output += "[";
                bool first = true;
                for (const auto &arg : this->types[type_id].generic_instance.generic_arguments) {
                    if (not first) output += ", ";
                    else first = false;
                    output += typename_for_type_id(arg);
                }
                output += "]";
                return output;
            }
            case CheckedType::Tag::Record: return this->records[this->types[type_id].record.record_id].name;
            case CheckedType::Tag::RawPtr: return std::format("raw {}", typename_for_type_id(this->types[type_id].rawptr.subtype));
        }
    }

public:
    Vec<CheckedFunction> functions{};
    Vec<CheckedRecord> records{};
    Vec<Scope *> scopes{};
    Vec<CheckedType> types{};

    Opt<FunctionId> current_function_index = std::nullopt;
};