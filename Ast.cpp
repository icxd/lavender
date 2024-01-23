#include <iostream>
#include <sstream>
#include "Ast.hpp"
#include "Common.hpp"

void AstPrinter::print(const Vec<Stmt *>& stmts) {
    for (auto stmt : stmts) {
        statement(stmt);
        std::cout << "\n";
    }
}

void AstPrinter::field(Field field) {
    std::cout << type(field.type) << " " << field.id.value;
    if (field.value.has_value()) {
        std::cout << " = ";
        expression(field.value.value());
    }
    std::cout << "\n";
}

void AstPrinter::statement(Stmt *stmt) {
    struct Visitor {
        void operator()(const StmtDetails::Object *object) const {
            std::cout << "object " << object->id.value;
            if (object->parent.has_value())
                std::cout << " > " << object->parent.value().value;
            std::cout << "\n";
            for (const auto& f : object->fields) {
                std::cout << "\t";
                field(f);
            }
        }

        void operator()(const StmtDetails::FunDecl *fun) const {
            std::cout << "fun " << fun->id.value << "(";
            for (usz i = 0; i < fun->parameters.size(); ++i) {
                auto& param = fun->parameters[i];
                std::cout << type(param.type) << " " << param.id.value;
                if (i != fun->parameters.size() - 1)
                    std::cout << ", ";
            }
            std::cout << ")";
            if (fun->ret_type.has_value())
                std::cout << " > " << type(fun->ret_type.value());
            std::cout << "\n";
            for (auto stmt : fun->body.elems) {
                std::cout << "\t";
                statement(stmt);
            }
        }

        void operator()(const StmtDetails::VarDecl *var) const {
            std::cout << "var " << type(var->type) << " " << var->id.value << " = ";
            expression(var->expr);
            std::cout << "\n";
        }

        void operator()(const StmtDetails::Return *ret) const {
            std::cout << "return ";
            if (ret->value.has_value())
                expression(ret->value.value());
            std::cout << "\n";
        }

        void operator()(const StmtDetails::Expr *expr) const {
            expression(expr->expr);
            std::cout << "\n";
        }
    };

    std::visit(Visitor{}, stmt->var);
}

void AstPrinter::expression(Expr *expr) {
    struct Visitor {
        void operator()(const ExprDetails::Id *id) const {
            std::cout << id->id.value;
        }

        void operator()(const ExprDetails::Int *integer) const {
            std::cout << std::to_string(integer->value);
        }

        void operator()(const ExprDetails::String *string) const {
            std::cout << string->value.value;
        }

        void operator()(const ExprDetails::Call *call) const {
            expression(call->callee);
            std::cout << "(";
            for (usz i = 0; i < call->arguments.size(); ++i) {
                auto& arg = call->arguments[i];
                if (arg.id.has_value())
                    std::cout << arg.id.value().value << ": ";
                expression(arg.expr);
                if (i != call->arguments.size() - 1)
                    std::cout << ", ";
            }
            std::cout << ")";
        }
    };

    std::visit(Visitor{}, expr->var);
}

Str AstPrinter::type(Type *ty) {
    switch (ty->type) {
        case Type::Kind::Id: return ty->id.value;
        case Type::Kind::Str: return "str";
        case Type::Kind::Int: return "int";
        case Type::Kind::Array: {
            std::stringstream ss;
            ss << "[" << type(ty->subtype) << "]";
            return ss.str();
        }
    }

    return "";
}
