#include "Common.hpp"
#include "Checker.hpp"
#include <sstream>
#include <format>
#include <iostream>

Checker::Checker(Project *project) noexcept : m_project(project) {
    m_scope_stack = {};
    m_next_scope_id = 0;
    begin_scope();
}

ErrorOr<Void> Checker::check(const Vec<Statement *>& stmts) {
    for (Statement *stmt : stmts) {
        if (stmt == nullptr) continue;
        auto result = check_statement(stmt);
        if (not result.has_value())
            return result.error();
    }
    return Void{};
}

ErrorOr<Void> Checker::check_statement(Statement *statement) {
    switch (static_cast<Statement::Kind>(statement->var.index())) {
        case Statement::Kind::Fun: {
            auto function = std::get<StatementDetails::FunDecl *>(statement->var);

            FunctionId function_id = this->m_project->functions.size();
            this->scope()->functions.insert({ function->id.value, function_id });
            this->m_project->functions.emplace_back();
            this->m_project->current_function_id = function_id;

            this->m_project->functions[function_id].id = function->id;
            for (auto &param : function->parameters)
                this->m_project->functions[function_id].parameters.push_back(param.id);
            this->m_project->functions[function_id].ret_type = function->ret_type;
            this->m_project->functions[function_id].body = function->body;
            this->m_project->functions[function_id].unsafe = function->unsafe;

            begin_scope();
            for (auto &param : function->parameters)
                this->scope()->symbols.push_back(param.id.value);

            auto result = check(function->body.elems);
            if (not result.has_value())
                return result.error();

            end_scope();

            this->m_project->current_function_id = UINT64_MAX;
        } break;
        case Statement::Kind::Return: {
            auto ret = std::get<StatementDetails::Return *>(statement->var);
            if (ret->value.has_value()) {
                Type type = try$(check_expression(ret->value.value()));
                auto ret_type = this->m_project->functions[this->m_project->current_function_id].ret_type;
                if (not ret_type.has_value()) break;
                if (type != *ret_type.value())
                    return error(ret->span, "return type doesn't match the expected function return type. expected `",
                                 Type::repr(**this->m_project->functions[this->m_project->current_function_id].ret_type), "` but got `",
                                 Type::repr(type), "` instead");
            }
        } break;
        case Statement::Kind::Object:
        case Statement::Kind::Var:
        case Statement::Kind::Expr:
            UNIMPLEMENTED;
    }
    return Void{};
}

ErrorOr<Type> Checker::check_expression(Expression *expression) {
    switch (static_cast<Expression::Kind>(expression->var.index())) {
        case Expression::Kind::Null: return Type{ .type = Type::Kind::Optional, .subtype = new Type{Type::Kind::Undetermined} };
        case Expression::Kind::UnsafeBlock: {
            auto block = std::get<ExpressionDetails::UnsafeBlock *>(expression->var);
            begin_scope();
            scope()->context = SafetyContext::Unsafe;
            Vec<Expression *> exprs = block->body.elems;
            for (auto expr : exprs)
                try$(check_expression(expr));
            Type return_type = try$(check_expression(exprs.back()));
            end_scope();
            return return_type;
        }
        case Expression::Kind::Int: return Type{Type::Kind::Int};
        case Expression::Kind::String: return Type{Type::Kind::Str};
        case Expression::Kind::Call: {
            auto call = std::get<ExpressionDetails::Call *>(expression->var);
            // TODO: fix this
            if (static_cast<Expression::Kind>(call->callee->var.index()) != Expression::Kind::Id)
                return error(call->span, "TODO: make this not have to be an identifier");
            auto callee = std::get<ExpressionDetails::Id *>(call->callee->var);
            Str id = callee->id.value;
            std::cout << id << "\n";
            std::cout << "this->scope()->functions.size(): " << this->scope()->functions.size() << "\n";
            for (auto [ident, fn_id] : this->scope()->functions)
                std::cout << ident << " -> " << fn_id << "\n";
            if (not this->scope()->functions.contains(id))
                return error(callee->id.span, "attempted to call an undefined function `", id, "`");
            FunctionId function_id = this->scope()->functions[id];
            Function function = this->m_project->functions.at(function_id);
            if (function.unsafe and this->scope()->context == SafetyContext::Safe)
                return error(callee->id.span, "attempted to call unsafe function in safe execution context");
            return **function.ret_type;
        }
        case Expression::Kind::Id:
        case Expression::Kind::Switch:
            UNIMPLEMENTED;
    }

    return error(Span{"yes", 1, 1, 1}, "UNIMPLEMENTED");
}

ErrorOr<Type> Checker::check_type(Type *) { }

Scope *Checker::scope() const {
    return m_scope_stack.at(m_scope_stack.size() - 1);
}

void Checker::begin_scope() {
    auto *scope = new Scope{m_next_scope_id++, 0, SafetyContext::Safe, {}};
    if (m_scope_stack.empty()) {
        scope->parent_id = 0;
        scope->context = SafetyContext::Safe;
    } else {
        scope->parent_id = m_scope_stack.at(m_scope_stack.size() - 1)->id;
        scope->context = m_scope_stack.at(scope->parent_id)->context;
        for (auto item : m_scope_stack.at(scope->parent_id)->functions)
            scope->functions.insert(item);
    }
    m_scope_stack[scope->id] = scope;
}

void Checker::end_scope() {
    m_next_scope_id--;
    Scope *scope = m_scope_stack.at(m_scope_stack.size() - 1);
    m_scope_stack.erase(scope->id);
    delete scope;
}

template <typename... Args> Error Checker::error(const Span &span, Args... args) {
    std::stringstream ss;
    (ss << ... << args);
    return {ss.str(), span};
}