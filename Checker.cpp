#include "Checker.hpp"
#include <sstream>

Checker::Checker() noexcept {
    m_scopes = {};
    m_scopes.insert({0, new Scope});
    m_next_scope_id = m_scopes.size();
}

ErrorOr<Void> Checker::check(const Vec<Stmt *>& stmts) {
    for (Stmt *stmt : stmts) {
        if (stmt == nullptr) continue;
        try$(statement(stmt));
    }

    return Void{};
}

ErrorOr<CheckedStmt *> Checker::statement(Stmt *stmt) {
    switch (static_cast<Stmt::Kind>(stmt->var.index())) {
        case Stmt::Kind::Object:
        case Stmt::Kind::Fun:
        case Stmt::Kind::Var:
        case Stmt::Kind::Return: break;
        case Stmt::Kind::Expr: {

        } break;
    }

    return nullptr;
}

bool Checker::locate_symbol(Scope *scope, Str str) {
    if (scope->symbols.empty()) return false; // FAST PATH
    if (std::find(scope->symbols.begin(), scope->symbols.end(), str) != scope->symbols.end())
        return true;
    return false;
}

bool Checker::locate_symbol(Str str) {
}

ErrorOr<CheckedExpr *> Checker::expression(Expr *expr) {
    switch (static_cast<Expr::Kind>(expr->var.index())) {
        case Expr::Kind::Id: {
            auto id = std::get<ExprDetails::Id *>(expr->var);
            if (locate_symbol(id->id.value)) {}
        } break;
        case Expr::Kind::Int:
        case Expr::Kind::String:
        case Expr::Kind::Call: break;
    }

    return nullptr;
}

ErrorOr<::Type> Checker::type(::Type *ty) {
    // TODO: make sure all the id types for example actually exist and aren't just made up
    return *ty;
}

void Checker::begin_scope() {

}

void Checker::end_scope() {
}

template <typename... Args> Error Checker::error(const Span &span, Args... args) {
    std::stringstream ss;
    (ss << ... << args);
    return {ss.str(), span};
}