#include "Checker.hpp"
#include <sstream>

Checker::Checker() noexcept {
    m_scope_stack.push_back(new Scope);
    m_top = m_scope_stack.back();
    m_global_scope = m_scope_stack.back();
}

ErrorOr<Void> Checker::check(Vec<Stmt *> stmts) {
    for (Stmt *stmt : stmts) {
        if (stmt == nullptr) continue;
        try$(statement(stmt));
    }

    return Void{};
}

ErrorOr<Void> Checker::statement(Stmt *stmt) {
    switch (static_cast<Stmt::Kind>(stmt->var.index())) {
        case Stmt::Kind::Object: {
            auto node = std::get<StmtDetails::Object *>(stmt->var);
            auto result = lookup_symbol(node->id);
            if (result.has_value()) {
                return error(node->id.span, "redefinition of symbol `", node->id.value, "`");
            }
            m_top->symbol_table[node->id.value] = SymbolType::Object;
            begin_scope();
            for (const auto &field : node->fields) {
                auto field_result = lookup_symbol(field.id);
                if (field_result.has_value()) {
                    return error(field.id.span, "redefinition of symbol `", field.id.value, "`");
                }
                m_top->symbol_table[field.id.value] = SymbolType::Variable;
            }
            end_scope();
        } break;
        case Stmt::Kind::Fun: {} break;
        case Stmt::Kind::Var: {} break;
        case Stmt::Kind::Return: {} break;
        case Stmt::Kind::Expr: {
            auto node = std::get<StmtDetails::Expr *>(stmt->var);
            auto result = expression(node->expr);
            if (not result.has_value()) return result.error();
            return Void{};
        }
    }

    return Void{};
}

ErrorOr<Type> Checker::expression(Expr *expr) {
    switch (static_cast<Expr::Kind>(expr->var.index())) {
        case Expr::Kind::Id: {
            auto node = std::get<ExprDetails::Id *>(expr->var);
            auto symbol_type = try$(lookup_symbol(node->id));
            if (symbol_type == SymbolType::Unknown) {
                return error(node->id.span, "use of undefined symbol `", node->id.value, "`");
            }
            return Type{.type = Type::Kind::Id, .id = node->id};
        }
        case Expr::Kind::Int: return Type{Type::Kind::Int};
        case Expr::Kind::String: return Type{Type::Kind::Str};
        case Expr::Kind::Call:
            break;
    }

    return Type{};
}

ErrorOr<Type> Checker::type(Type *) {
    return Type{};
}

void Checker::begin_scope() {
    auto *scope = m_scope_stack.back();
    m_scope_stack.push_back(scope);
    m_top = m_scope_stack.back();
}

void Checker::end_scope() {
    m_scope_stack.pop_back();
    m_top = m_scope_stack.back();
}

ErrorOr<SymbolType> Checker::lookup_symbol(const SpannedStr& symbol) {
    auto type = try$(lookup_symbol(m_top, symbol));
    if (type != SymbolType::Unknown) return type;
    type = try$(lookup_symbol(m_global_scope, symbol));
    if (type != SymbolType::Unknown) return type;
    return error(symbol.span, "use of undefined symbol `", symbol.value, "`");
}

ErrorOr<SymbolType> Checker::lookup_symbol(Scope *scope, const SpannedStr& symbol) {
    if (scope->symbol_table.contains(symbol.value)) return scope->symbol_table[symbol.value];
    return SymbolType::Unknown;
}

template <typename... Args> Error Checker::error(const Span &span, Args... args) {
    std::stringstream ss;
    (ss << ... << args);
    return {ss.str(), span};
}