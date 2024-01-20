#include <iostream>
#include <sstream>
#include "Ast.hpp"
#include "Common.hpp"

void AstPrinter::print(const Vec<Stmt *>& stmts) {
    for (auto stmt : stmts)
        statement(stmt);
}

void AstPrinter::field(Field field) {
    std::cout << type(field.type) << " " << field.id;
    if (field.value.has_value()) {
        std::cout << " = ";
        expression(field.value.value());
    }
    std::cout << "\n";
}

void AstPrinter::statement(Stmt *stmt) {
    struct Visitor {
        void operator()(const StmtDetails::Object *object) const {
            std::cout << "object " << object->id;
            if (object->parent.has_value())
                std::cout << " > " << object->parent.value();
            std::cout << "\n";
            for (const auto& f : object->fields) {
                std::cout << "\t";
                field(f);
            }
        }

        void operator()(const StmtDetails::FunDecl *) const { }

        void operator()(const StmtDetails::VarDecl *) const { }

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
        void operator()(const ExprDetails::Int *integer) const {
            std::cout << std::to_string(integer->value);
        }

        void operator()(const ExprDetails::String *string) const {
            std::cout << string->value;
        }
    };

    std::visit(Visitor{}, expr->var);
}

Str AstPrinter::type(Type *ty) {
    switch (ty->type) {
        case Type::Kind::Str: return "str";
        case Type::Kind::Int: return "int";
        case Type::Kind::Array: {
            std::stringstream ss;
            ss << "[" << type(ty->subtype) << "]";
            return ss.str();
        }
    }
}
