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

            begin_scope();
            for (auto &param : function->parameters)
                this->scope()->symbols.push_back(param.id.value);

            auto result = check(function->body.elems);
            if (not result.has_value())
                return result.error();

            end_scope();

            this->m_project->functions[function_id].id = function->id;
            for (auto &param : function->parameters)
                this->m_project->functions[function_id].parameters.push_back(param.id);
            this->m_project->functions[function_id].ret_type = function->ret_type;
            this->m_project->functions[function_id].body = function->body;
            this->m_project->functions[function_id].unsafe = function->unsafe;

            this->m_project->current_function_id = UINT64_MAX;
        } break;
        case Statement::Kind::Return: {
            auto ret = std::get<StatementDetails::Return *>(statement->var);
            if (ret->value.has_value()) {
                Type type = try$(check_expression(ret->value.value()));
                if (type != *this->m_project->functions[this->m_project->current_function_id].ret_type.value())
                    return error(ret->)
            }
        } break;
        case Statement::Kind::Object:
        case Statement::Kind::Var:
        case Statement::Kind::Expr:
            UNIMPLEMENTED;
    }
    return Void{};
}

ErrorOr<Type> Checker::check_expression(Expression *) { }

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
    }
    m_scope_stack[scope->id] = scope;
}

void Checker::end_scope() {
    Scope *scope = m_scope_stack.at(m_scope_stack.size() - 1);
    m_scope_stack.erase(scope->id);
    delete scope;
}

template <typename... Args> Error Checker::error(const Span &span, Args... args) {
    std::stringstream ss;
    (ss << ... << args);
    return {ss.str(), span};
}