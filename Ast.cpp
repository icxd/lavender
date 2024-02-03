#include <iostream>
#include <sstream>
#include "Ast.hpp"
#include "Common.hpp"

static usz indent = 0;

void AstPrinter::print(const Vec<Statement *>& stmts) {
    for (auto stmt : stmts) {
        statement(stmt);
        std::cout << "\n";
    }
}

void AstPrinter::field(Field field) {
    for (usz i = 0; i < indent; i++)
        std::cout << "    ";

    std::cout << type(field.type) << " " << field.id.value;
    if (field.value.has_value()) {
        std::cout << " = ";
        expression(field.value.value(), false);
    }
    std::cout << "\n";
}

void AstPrinter::statement(Statement *stmt) {
    for (usz i = 0; i < indent; i++)
        std::cout << "    ";

    struct Visitor {
        void operator()(const StatementDetails::Object *object) const {
            std::cout << "object " << object->id.value;
            if (object->parent.has_value())
                std::cout << " > " << object->parent.value().value;
            std::cout << "\n";
            indent += 1;
            for (const auto& f : object->fields) {
                field(f);
            }
            indent -= 1;
        }

        void operator()(const StatementDetails::FunDecl *fun) const {
            std::cout << (fun->unsafe ? "unsafe " : "") << "fun " << fun->id.value << "(";
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
            indent += 1;
            for (auto stmt : fun->body.elems) {
                statement(stmt);
            }
            indent -= 1;
        }

        void operator()(const StatementDetails::VarDecl *var) const {
            std::cout << "var " << type(var->type) << " " << var->id.value << " = ";
            expression(var->expr, false);
            std::cout << "\n";
        }

        void operator()(const StatementDetails::Return *ret) const {
            std::cout << "return ";
            if (ret->value.has_value())
                expression(ret->value.value(), false);
            std::cout << "\n";
        }

        void operator()(const StatementDetails::Expression *expr) const {
            expression(expr->expr, false);
            std::cout << "\n";
        }
    };

    std::visit(Visitor{}, stmt->var);
}

void AstPrinter::expression(Expression *expr, bool print_indent) {
    if (print_indent) for (usz i = 0; i < indent; i++)
        std::cout << "    ";

    struct Visitor {
        void operator()(const ExpressionDetails::Null *) const {
            std::cout << "null";
        }

        void operator()(const ExpressionDetails::Id *id) const {
            std::cout << id->id.value;
        }

        void operator()(const ExpressionDetails::Int *integer) const {
            std::cout << std::to_string(integer->value);
        }

        void operator()(const ExpressionDetails::String *string) const {
            std::cout << string->value.value;
        }

        void operator()(const ExpressionDetails::Call *call) const {
            expression(call->callee, false);
            std::cout << "(";
            for (usz i = 0; i < call->arguments.size(); ++i) {
                auto& arg = call->arguments[i];
                if (arg.id.has_value())
                    std::cout << arg.id.value().value << ": ";
                expression(arg.expr, false);
                if (i != call->arguments.size() - 1)
                    std::cout << ", ";
            }
            std::cout << ")";
        }

        void operator()(const ExpressionDetails::Switch *switch_) const {
        }

        void operator()(const ExpressionDetails::UnsafeBlock *unsafe_block) const {
            std::cout << "unsafe\n";
            indent += 1;
            for (auto item : unsafe_block->body.elems) {
                expression(item);
            }
            indent -= 1;
        }
    };

    std::visit(Visitor{}, expr->var);
}

Str AstPrinter::type(Type *ty) { return Type::repr(*ty); }
