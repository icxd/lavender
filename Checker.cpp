#include "Common.hpp"
#include "Checker.hpp"
#include <sstream>

Checker::Checker() noexcept {
    m_scope_stack = {};
    m_next_scope_id = 0;
    begin_scope();
}

ErrorOr<Vec<CheckedStmt *>> Checker::check(const Vec<Stmt *>& stmts) {
    Vec<CheckedStmt *> checked_stmts;
    for (Stmt *stmt : stmts) {
        if (stmt == nullptr) continue;
        CheckedStmt *checked_stmt = try$(statement(stmt));
        if (checked_stmt == nullptr)
            PANIC("COMPILER ERROR: CHECKER RETURNED NULLPTR FOR STATEMENT");
        checked_stmts.push_back(checked_stmt);
    }
    return checked_stmts;
}

ErrorOr<CheckedStmt *> Checker::statement(Stmt *stmt) {
    switch (static_cast<Stmt::Kind>(stmt->var.index())) {
        case Stmt::Kind::Object: {
            StmtDetails::Object *object = std::get<StmtDetails::Object *>(stmt->var);
            if (locate_symbol(object->id.value))
                return error(object->id.span, "redefinition of symbol `", object->id.value, "`");
            scope()->symbols.push_back(object->id.value);

            Vec<CheckedField> fields;
            for (const auto &field : object->fields) {
                if (locate_symbol(field.id.value))
                    return error(field.id.span, "redefinition of symbol `", field.id.value, "`");
                scope()->symbols.push_back(field.id.value);

                Type field_type = try$(type(field.type));
                Opt<CheckedExpr *> field_expr = std::nullopt;
                if (field.value.has_value())
                    field_expr = try$(expression(field.value.value()));

                fields.push_back(CheckedField{field.id.value, field_type, field_expr});
            }

            return new CheckedStmtDetails::CheckedObject(object->id.value, fields);
        }
        case Stmt::Kind::Fun: {
            StmtDetails::FunDecl *fun = std::get<StmtDetails::FunDecl *>(stmt->var);
            if (locate_symbol(fun->id.value))
                return error(fun->id.span, "redefinition of symbol `", fun->id.value, "`");
            scope()->symbols.push_back(fun->id.value);

            return new CheckedStmtDetails::CheckedFun(fun->id.value);
        }
        case Stmt::Kind::Var:
        case Stmt::Kind::Return:
        case Stmt::Kind::Expr:
            UNIMPLEMENTED;
    }

    return nullptr;
}

bool Checker::locate_symbol(Scope *scope, const Str& str) {
    if (scope == nullptr) return false;
    if (scope->symbols.empty()) return false; // FAST PATH
    for (const Str& symbol : scope->symbols) {
        if (symbol == str) return true;
    }
    if (scope->parent_id == 0) return false;
    return locate_symbol(m_scope_stack[scope->parent_id], str);
}

bool Checker::locate_symbol(const Str& str) {
    return locate_symbol(scope(), str);
}

ErrorOr<CheckedExpr *> Checker::expression(Expr *expr) {
    switch (static_cast<Expr::Kind>(expr->var.index())) {
        case Expr::Kind::Id: {
            auto id = std::get<ExprDetails::Id *>(expr->var);
            if (locate_symbol(id->id.value))
                return new CheckedExprDetails::CheckedId(id->id.value);
            return error(id->id.span, "unknown symbol `", id->id.value, "`");
        }
        case Expr::Kind::Int: {
            auto int_ = std::get<ExprDetails::Int *>(expr->var);
            return new CheckedExprDetails::CheckedInt(int_->value);
        }
        case Expr::Kind::String:
        case Expr::Kind::Call:
            UNIMPLEMENTED;
    }

    return nullptr;
}

ErrorOr<::Type> Checker::type(::Type *ty) {
    // TODO: make sure all the id types for example actually exist and aren't just made up
    return *ty;
}

Scope *Checker::scope() const {
    return m_scope_stack.at(m_scope_stack.size() - 1);
}

void Checker::begin_scope() {
    Scope *scope = new Scope{m_next_scope_id++, 0, {}};
    if (m_scope_stack.empty()) {
        scope->parent_id = 0;
    } else {
        scope->parent_id = m_scope_stack.at(m_scope_stack.size() - 1)->id;
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