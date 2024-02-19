#include "Project.hpp"
#include <format>

bool Scope::can_access(ScopeId own, ScopeId other, const Project &project) {
    if (own == other) return true;
    else {
        auto own_scope = project.scopes[own];
        while (own_scope->parent.has_value()) {
            auto parent = own_scope->parent.value();
            if (parent == other)
                return true;
            own_scope = project.scopes[parent];
        }
        return false;
    }
}

TypeId Project::find_or_add_type_id(const CheckedType& type) {
    for (usz i = 0; i < this->types.size(); i++) {
        auto ty = this->types[i];
        if (ty == type) { return i; }
    }

    this->types.push_back(type);
    return this->types.size() - 1;
}

ScopeId Project::create_scope(ScopeId scope) {
    this->scopes.push_back(new Scope(std::make_optional(scope)));
    return this->scopes.size() - 1;
}

ErrorOr<Void> Project::add_var_to_scope(ScopeId scope_id, const CheckedVariable& var, Span span) {
    Scope *scope = this->scopes[scope_id];
    for (const auto& existing_var : scope->types) {
        if (var.name == existing_var.id) {
            return Error(
                std::format("redefinition of variable {}", var.name),
                span
            );
        }
    }

    scope->variables.push_back(var);
    return Void{};
}

Opt<CheckedVariable> Project::find_var_in_scope(ScopeId id, const Str& var) {
    Opt<ScopeId> scope_id = std::make_optional(id);

    while (scope_id.has_value()) {
        ScopeId current_id = scope_id.value();
        Scope *scope = this->scopes[current_id];

        for (auto v : scope->variables) {
            if (v.name == var) return std::make_optional(v);
        }

        scope_id = scope->parent;
    }

    return std::nullopt;
}

ErrorOr<Void> Project::add_type_to_scope(ScopeId scope_id, Str type_name, TypeId type_id, Span span) {
    Scope *scope = this->scopes[scope_id];
    for (const auto& existing_type : scope->types) {
        if (type_name == existing_type.id) {
            return Error(
                    std::format("redefinition of variable {}", type_name),
                    span
            );
        }
    }

    scope->types.push_back({type_name, type_id});
    return Void{};
}

Opt<TypeId> Project::find_type_in_scope(ScopeId id, const Str &type) {
    Opt<ScopeId> scope_id = std::make_optional(id);

    while (scope_id.has_value()) {
        ScopeId current_id = scope_id.value();
        Scope *scope = this->scopes[current_id];

        for (auto v : scope->types) {
            if (v.id == type) return std::make_optional(v.value);
        }

        scope_id = scope->parent;
    }

    return std::nullopt;
}

ErrorOr<Void> Project::add_function_to_scope(ScopeId scope_id, Str name, FunctionId function_id, Span span) {
    Scope *scope = this->scopes[scope_id];
    for (const auto& fn : scope->functions) {
        if (name == fn.id) {
            return Error(
                    std::format("redefinition of function {}", name),
                    span
            );
        }
    }

    scope->functions.push_back({name, function_id});
    return Void{};
}

Opt<FunctionId> Project::find_function_in_scope(ScopeId id, const Str &name) {
    Opt<ScopeId> scope_id = std::make_optional(id);

    while (scope_id.has_value()) {
        ScopeId current_id = scope_id.value();
        Scope *scope = this->scopes[current_id];

        for (auto v : scope->functions) {
            if (v.id == name) return std::make_optional(v.value);
        }

        scope_id = scope->parent;
    }

    return std::nullopt;
}

ErrorOr<Void> Project::add_record_to_scope(ScopeId scope_id, Str name, RecordId record_id, Span span) {
    Scope *scope = this->scopes[scope_id];
    for (const auto& record : scope->records) {
        if (name == record.id) {
            return Error(
                    std::format("redefinition of record {}", name),
                    span
            );
        }
    }

    scope->records.push_back({name, record_id});
    return Void{};
}

Opt<RecordId> Project::find_record_in_scope(ScopeId id, const Str &record_name) {
    Opt<ScopeId> scope_id = std::make_optional(id);

    while (scope_id.has_value()) {
        ScopeId current_id = scope_id.value();
        Scope *scope = this->scopes[current_id];

        for (auto v : scope->records) {
            if (v.id == record_name) return std::make_optional(v.value);
        }

        scope_id = scope->parent;
    }

    return std::nullopt;
}
