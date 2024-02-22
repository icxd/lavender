#include "Common.hpp"
#include "Checker.hpp"
#include <sstream>
#include <format>
#include <iostream>

Opt<Error> typecheck_namespace(const ParsedNamespace& parsed_namespace, ScopeId scope_id, Project& project) {
    Opt<Error> error = std::nullopt;

    RecordId project_record_length = project.records.size();

    for (auto ns : parsed_namespace.namespaces) {
        ScopeId ns_scope_id = project.create_scope(scope_id);
        project.scopes[ns_scope_id]->namespace_name = ns->name;
        project.scopes[scope_id]->children.push_back(ns_scope_id);
        typecheck_namespace(*ns, ns_scope_id, project);
    }

    for (RecordId id = 0; id < parsed_namespace.objects.size(); id++) {
        auto object = parsed_namespace.objects[id];
        RecordId record_id = id + project_record_length;
        project.types.push_back(CheckedType::Record(record_id));

        auto object_type_id = project.types.size() - 1;
        ErrorOr<Void> type_error = project.add_type_to_scope(
                scope_id,
                object->id.value,
                object_type_id,
                object->id.span
        );
        if (not type_error.has_value())
            error = error.value_or(type_error.error());
    }

    for (RecordId id = 0; id < parsed_namespace.objects.size(); id++) {
        auto object = parsed_namespace.objects[id];
        RecordId record_id = id + project_record_length;

        Opt<Error> x = typecheck_record_predecl(*object, record_id, scope_id, project);
        if (x.has_value())
            error = error.value_or(x.value());
    }

    for (RecordId id = 0; id < parsed_namespace.objects.size(); id++) {
        auto object = parsed_namespace.objects[id];
        RecordId record_id = id + project_record_length;

        Opt<Error> x = typecheck_record(*object, record_id, scope_id, project);
        if (x.has_value())
            error = error.value_or(x.value());
    }

    return error;
}

Opt<Error> typecheck_record_predecl(const ParsedObject& record, RecordId record_id, ScopeId parent_scope_id, Project& project) {
    Opt<Error> error = std::nullopt;

    TypeId type_id = project.find_or_add_type_id(CheckedType::Record(record_id));
    ScopeId scope_id = project.create_scope(parent_scope_id);

    Vec<TypeId> generic_parameters = {};
    for (auto generic_parameter : record.generic_params) {
        project.types.push_back(CheckedType::TypeVariable(generic_parameter->id.value));
        TypeId parameter_type_id = project.types.size() - 1;

        generic_parameters.push_back(parameter_type_id);

        ErrorOr<Void> x = project.add_type_to_scope(scope_id, generic_parameter->id.value, parameter_type_id, generic_parameter->id.span);
        if (not x.has_value())
            error = error.value_or(x.error());
    }

    for (const auto& method : record.methods) {
        Vec<TypeId> method_generic_parameters = {};
        ScopeId method_scope_id = project.create_scope(scope_id);

        // TODO: generic parameters for functions/methods
//        for (Type *generic_parameter : method.generic_parameters) {
//            project.types.push_back(CheckedType::TypeVariable(generic_parameter->id.value));
//            TypeId type_var_type_id = project.types.size() - 1;
//        }

        auto checked_function = CheckedFunction{
            .name = method.id.value,
            .return_type_id = UNKNOWN_TYPE_ID,
            .parameters = {},
            .generic_parameters = generic_parameters,
            .scope_id = method_scope_id,
        };

        for (const auto& parameter : method.parameters) {
            auto [param_type, err] = typecheck_typename(parameter.type, method_scope_id, project);
            if (err.has_value())
                error = error.value_or(err.value());

            auto checked_variable = CheckedVariable{
                .name = parameter.id.value,
                .type_id = param_type,
            };

            checked_function.parameters.push_back(CheckedParameter{
                .requires_label = false,
                .variable = checked_variable,
            });
        }

        project.functions.push_back(checked_function);
        ErrorOr<Void> x = project.add_function_to_scope(scope_id, method.id.value, project.functions.size() - 1, record.id.span);
        if (not x.has_value())
            error = error.value_or(x.error());
    }

    project.records.push_back(CheckedRecord{
        .name = record.id.value,
        .generic_parameters = generic_parameters,
        .fields = {},
        .scope_id = scope_id,
    });

    ErrorOr<Void> x = project.add_record_to_scope(parent_scope_id, record.id.value, record_id, record.id.span);
    if (not x.has_value())
        error = error.value_or(x.error());

    return error;
}

Opt<Error> typecheck_record(const ParsedObject& object, RecordId record_id, ScopeId parent_scope_id, Project& project) {
    Opt<Error> error = std::nullopt;

    Vec<CheckedVarDecl> fields = {};

    CheckedRecord checked_record = project.records[record_id];
    ScopeId checked_record_scope_id = checked_record.scope_id;
    TypeId record_type_id = project.find_or_add_type_id(CheckedType::Record(record_id));

    for (const auto& unchecked_member : object.fields) {
        auto [checked_member_type, err] = typecheck_typename(unchecked_member.type, checked_record_scope_id, project);
        if (err.has_value()) error = error.value_or(err.value());

        fields.push_back(CheckedVarDecl{
            .name = unchecked_member.id.value,
            .type_id = checked_member_type,
            .span = unchecked_member.id.span,
        });
    }

    // No constructor was found so we need to make one
    if (not project.find_function_in_scope(checked_record_scope_id, object.id.value).has_value()) {
        Vec<CheckedParameter> params = {};
        for (const auto& field : fields) {
            params.push_back(CheckedParameter{
                    .requires_label = true,
                    .variable = CheckedVariable{
                            .name = field.name,
                            .type_id = field.type_id,
                    }
            });

            ScopeId constructor_scope_id = project.create_scope(parent_scope_id);
            auto checked_constructor = CheckedFunction{
                    .name = object.id.value,
                    .return_type_id = record_type_id,
                    .parameters = params,
                    .generic_parameters = {},
                    .scope_id = constructor_scope_id,
            };

            project.functions.push_back(checked_constructor);

            auto x = project.add_function_to_scope(checked_record_scope_id, object.id.value,
                                                   project.functions.size() - 1, object.id.span);
            if (not x.has_value())
                error = error.value_or(x.error());
        }
    }

    CheckedRecord record = project.records[record_id];
    record.fields = fields;

    for (const auto& fn : object.methods) {
        Opt<Error> x = typecheck_method(fn, record_id, project);
        if (x.has_value()) error = error.value_or(x.value());
    }

    return error;
}


Opt<Error> typecheck_method(const ParsedMethod& method, RecordId record_id, Project& project) {
    Opt<Error> error = std::nullopt;

    CheckedRecord record = project.records[record_id];
    ScopeId record_scope_id = record.scope_id;

    Opt<FunctionId> opt_method_id = project.find_function_in_scope(record_scope_id, method.id.value);
    if (not opt_method_id.has_value()) PANIC("Internal error: pushed a checked function but it's not defined.");
    FunctionId method_id = opt_method_id.value();
    project.current_function_index = std::make_optional(method_id);

    CheckedFunction checked_function = project.functions[method_id];
    ScopeId function_scope_id = checked_function.scope_id;

    Vec<CheckedVariable> parameters = {};
    for (const auto &parameter : checked_function.parameters) {
        parameters.push_back(parameter.variable);
    }

    for (const auto &variable : parameters) {
        ErrorOr<Void> x = project.add_var_to_scope(function_scope_id, variable, method.id.span);
        if (not x.has_value())
            error = error.value_or(x.error());
    }

    auto [block, err2] = typecheck_block(method.body, function_scope_id, project, SafetyContext::Safe);
    if (err2.has_value()) error = error.value_or(err2.value());

    TypeId return_type_id = UNKNOWN_TYPE_ID;
    if (method.ret_type.has_value()) {
        auto [function_return_type_id, err] = typecheck_typename(method.ret_type.value(), function_scope_id, project);
        if (err.has_value()) error = error.value_or(err.value());
        return_type_id = function_return_type_id;
    }

    if (return_type_id == UNKNOWN_TYPE_ID) return_type_id = UNIT_TYPE_ID;

    CheckedFunction checked_fn = project.functions[method_id];
    checked_fn.block = block;
    checked_fn.return_type_id = return_type_id;

    project.current_function_index = std::nullopt;

    return error;
}

std::tuple<CheckedStatement, Opt<Error>> typecheck_statement(ParsedStatement *statement, ScopeId scope_id, Project& project, SafetyContext context) {
    Opt<Error> error = std::nullopt;

    switch (static_cast<ParsedStatement::Kind>(statement->var.index())) {
        case ParsedStatement::Kind::Object:
            UNIMPLEMENTED("Object");
        case ParsedStatement::Kind::Interface:
            UNIMPLEMENTED("Interface");
        case ParsedStatement::Kind::Fun:
            UNIMPLEMENTED("Fun");
        case ParsedStatement::Kind::Var:
            UNIMPLEMENTED("Var");
        case ParsedStatement::Kind::Return: {
            auto *stmt = std::get<ParsedReturn *>(statement->var);
            auto [output, err] = typecheck_expression(stmt->value.value(), scope_id, project, context, project.functions[project.current_function_index.value()].return_type_id);
            return std::make_tuple(CheckedStatement::Return(&output), err);
        }
        case ParsedStatement::Kind::Expr: {
            auto *stmt = std::get<ParsedExpression *>(statement->var);
            auto [output, err] = typecheck_expression(stmt->expr, scope_id, project, context, project.functions[project.current_function_index.value()].return_type_id);
            return std::make_tuple(CheckedStatement::Expression(&output), err);
        }
    }
}

std::tuple<CheckedExpression, Opt<Error>> typecheck_expression(Expression *expression, ScopeId scope_id, Project& project, SafetyContext context, Opt<TypeId> type_hint) {
    Opt<Error> error = std::nullopt;

    auto unify_with_type_hint = [&](Project &project, TypeId type_id) -> std::tuple<TypeId, Opt<Error>> {
        if (type_hint.has_value()) {
            TypeId hint = type_hint.value();
            if (hint == UNKNOWN_TYPE_ID)
                return std::make_tuple(type_id, std::nullopt);

            Map<TypeId, TypeId> generic_interface = {};
            Opt<Error> err = check_types_for_compat(
                    hint,
                    type_id,
                    &generic_interface,
                    expression->span(),
                    project
            );
            if (err.has_value())
                return std::make_tuple(type_id, err);

            return std::make_tuple(
                    substitute_typevars_in_type(type_id, &generic_interface, project),
                    std::nullopt
            );
        }

        return std::make_tuple(type_id, std::nullopt);
    };

    switch (static_cast<Expression::Kind>(expression->var.index())) {
        case Expression::Kind::Null: return std::make_tuple(CheckedExpression::Null(), std::nullopt);
        case Expression::Kind::Id: {
            auto *expr = std::get<ExpressionDetails::Id *>(expression->var);

            Opt<CheckedVariable> opt_var = project.find_var_in_scope(scope_id, expr->id.value);
            if (not opt_var.has_value()) {
                return std::make_tuple(
                        CheckedExpression::Var({
                                CheckedVariable{expr->id.value, type_hint.value_or(UNKNOWN_TYPE_ID)},
                                expr->id.span
                        }),
                        Error{"variable not found", expr->id.span}
                );
            }
            CheckedVariable var = opt_var.value();

            auto [_, err] = unify_with_type_hint(project, var.type_id);
            return std::make_tuple(CheckedExpression::Var({var, expr->id.span}), err);
        }
        case Expression::Kind::Int: {
            auto *expr = std::get<ExpressionDetails::Int *>(expression->var);

            // TODO: make sure integer constants can have user-specified type ids such as uint or int64
            auto [type_id, err] = unify_with_type_hint(project, INT_TYPE_ID);
            if (err.has_value()) error = error.value_or(err.value());

            return std::make_tuple(CheckedExpression::Int(expr->value), error);
        }
        case Expression::Kind::String: {
            auto *expr = std::get<ExpressionDetails::String *>(expression->var);

            auto [type_id, err] = unify_with_type_hint(project, STRING_TYPE_ID);

            return std::make_tuple(CheckedExpression::String(expr->value), err);
        }
        case Expression::Kind::Call: {
            auto *expr = std::get<ExpressionDetails::Call *>(expression->var);

            auto [id, id_err] = typecheck_expression(expr->callee, scope_id, project, context, type_hint);
            if (id_err.has_value()) error = error.value_or(id_err.value());

            UNIMPLEMENTED("Call");
        }
        case Expression::Kind::Index:
            UNIMPLEMENTED("Index");
        case Expression::Kind::GenericInstance:
            UNIMPLEMENTED("GenericInstance");
        case Expression::Kind::Unary:
            UNIMPLEMENTED("Unary");
        case Expression::Kind::Binary: {
            auto *expr = std::get<ExpressionDetails::Binary *>(expression->var);

            auto [left, left_err] = typecheck_expression(expr->left, scope_id, project, context, type_hint);
            if (left_err.has_value()) error = error.value_or(left_err.value());

            auto [right, right_err] = typecheck_expression(expr->right, scope_id, project, context, type_hint);
            if (right_err.has_value()) error = error.value_or(right_err.value());
        }
        case Expression::Kind::If: {
            auto *expr = std::get<ExpressionDetails::If *>(expression->var);

            auto [cond, cond_err] = typecheck_expression(expr->condition, scope_id, project, context, type_hint);
            if (cond_err.has_value()) error = error.value_or(cond_err.value());

            auto [then, then_err] = typecheck_expression(expr->then, scope_id, project, context, type_hint);
            if (then_err.has_value()) error = error.value_or(then_err.value());

            auto [else_, else_err] = typecheck_expression(expr->else_, scope_id, project, context, type_hint);
            if (else_err.has_value()) error = error.value_or(else_err.value());

            return std::make_tuple(CheckedExpression::If(&cond, &then, &else_), error);
        }
        case Expression::Kind::Access:
            UNIMPLEMENTED("Access");
        case Expression::Kind::Switch:
            UNIMPLEMENTED("Switch");
        case Expression::Kind::UnsafeBlock:
            UNIMPLEMENTED("UnsafeBlock");
    }
}

std::tuple<CheckedBlock, Opt<Error>> typecheck_block(const Block<ParsedStatement *>& block, ScopeId parent_scope_id, Project& project, SafetyContext context) {
    Opt<Error> error = std::nullopt;
    CheckedBlock checked_block = {{}};

    ScopeId block_scope_id = project.create_scope(parent_scope_id);

    for (const auto &stmt : block.elems) {
        auto [checked_stmt, err] = typecheck_statement(stmt, block_scope_id, project, context);
        if (err.has_value()) error = error.value_or(err.value());
        checked_block.statements.push_back(&checked_stmt);
    }

    return std::make_tuple(checked_block, error);
}

std::tuple<TypeId, Opt<Error>> typecheck_typename(Type *unchecked_type, ScopeId scope_id, Project& project) {
    Opt<Error> error = std::nullopt;

    switch (unchecked_type->type) {
        case Type::Kind::Undetermined: return std::make_tuple(UNKNOWN_TYPE_ID, std::nullopt);
        case Type::Kind::Id: {
            Opt<TypeId> type_id = project.find_type_in_scope(scope_id, unchecked_type->id.value);
            if (type_id.has_value())
                return std::make_tuple(type_id.value(), std::nullopt);
            else
                return std::make_tuple(UNKNOWN_TYPE_ID, std::make_optional(Error{"unknown type", unchecked_type->id.span}));
        }
        case Type::Kind::Str: return std::make_tuple(STRING_TYPE_ID, std::nullopt);
        case Type::Kind::Int: return std::make_tuple(INT_TYPE_ID, std::nullopt);
        case Type::Kind::Array: {
            auto [inner_type_id, err] = typecheck_typename(unchecked_type->subtype, scope_id, project);
            if (err.has_value()) error = error.value_or(err.value());

            Opt<RecordId> opt_array_record_id = project
                    .find_record_in_scope(0, "Array");
            if (not opt_array_record_id.has_value())
                PANIC("internal error: `Array` builtin was not found");
            RecordId array_record_id = opt_array_record_id.value();

            TypeId type_id = project.find_or_add_type_id(CheckedType::GenericInstance(array_record_id, {inner_type_id}));

            return std::make_tuple(type_id, error);
        }
        case Type::Kind::Weak: break;
        case Type::Kind::Raw: {
            auto [inner_type_id, err] = typecheck_typename(unchecked_type->subtype, scope_id, project);
            if (err.has_value()) error = error.value_or(err.value());

            TypeId type_id = project.find_or_add_type_id(CheckedType::RawPtr(inner_type_id));

            return std::make_tuple(type_id, error);
        }
        case Type::Kind::Optional: break;
        case Type::Kind::Generic: {
            Vec<TypeId> checked_inner_types = {};

            for (const auto& inner_type : unchecked_type->generic_args) {
                auto [inner_type_id, err] = typecheck_typename(inner_type, scope_id, project);
                if (err.has_value()) error = error.value_or(err.value());

                checked_inner_types.push_back(inner_type_id);
            }

            Opt<RecordId> record_id = project.find_record_in_scope(scope_id, unchecked_type->id.value);
            if (record_id.has_value())
                return std::make_tuple(project.find_or_add_type_id(CheckedType::GenericInstance(record_id.value(), checked_inner_types)), error);
            else return std::make_tuple(UNKNOWN_TYPE_ID, std::make_optional(Error{std::format("undefined type `{}`", unchecked_type->id.value), unchecked_type->id.span}));
        }
    }
}

TypeId substitute_typevars_in_type(TypeId type_id, Map<TypeId, TypeId> *generic_inferences, Project &project) {
    TypeId result = substitute_typevars_in_type_helper(type_id, generic_inferences, project);

    for (;;) {
        TypeId fixed_point = substitute_typevars_in_type_helper(type_id, generic_inferences, project);
        if (fixed_point == result) break;
        else result = fixed_point;
    }

    return result;
}

TypeId substitute_typevars_in_type_helper(TypeId type_id, Map<TypeId, TypeId> *generic_inferences, Project &project) {
    CheckedType type = project.types[type_id];
    switch (type.tag) {
        case CheckedType::Tag::TypeVariable:
            if (generic_inferences->contains(type_id))
                return generic_inferences->at(type_id);
            break;
        case CheckedType::Tag::GenericInstance: {
            RecordId record_id = type.generic_instance.record_id;
            Vec<TypeId> args = type.generic_instance.generic_arguments;

            for (TypeId& idx : args) {
                TypeId *arg = &idx;
                *arg = substitute_typevars_in_type(*arg, generic_inferences, project);
            }

            return project.find_or_add_type_id(CheckedType::GenericInstance(record_id, args));
        } break;
        case CheckedType::Tag::Record: {
            RecordId record_id = type.record.record_id;
            CheckedRecord record = project.records[record_id];

            if (not record.generic_parameters.empty()) {
                Vec<TypeId> args = record.generic_parameters;

                for (TypeId& idx : args) {
                    TypeId *arg = &idx;
                    *arg = substitute_typevars_in_type(*arg, generic_inferences, project);
                }

                return project.find_or_add_type_id(CheckedType::GenericInstance(record_id, args));
            }
        } break;
        default: break;
    }
    return type_id;
}

Opt<Error> check_types_for_compat(
        TypeId lhs_type_id,
        TypeId rhs_type_id,
        Map<TypeId, TypeId> *generic_inferences,
        Span span,
        Project &project
) {
    Opt<Error> error = std::nullopt;
    CheckedType lhs_type = project.types[lhs_type_id];

    Opt<RecordId> opt_optional_record_id = project.find_record_in_scope(0, "Optional");
    if (not opt_optional_record_id.has_value())
        PANIC("internal error: `Optional` builtin was not found");
    RecordId optional_record_id = opt_optional_record_id.value();

    Opt<RecordId> opt_weak_ptr_record_id = project.find_record_in_scope(0, "WeakPtr");
    if (not opt_weak_ptr_record_id.has_value())
        PANIC("internal error: `WeakPtr` builtin was not found");
    RecordId weak_ptr_record_id = opt_weak_ptr_record_id.value();

    if (lhs_type.tag == CheckedType::Tag::GenericInstance) {
        RecordId lhs_struct_id = lhs_type.generic_instance.record_id;
        Vec<TypeId> args = lhs_type.generic_instance.generic_arguments;

        if ((lhs_struct_id == optional_record_id or lhs_struct_id == weak_ptr_record_id) and args.front() == rhs_type_id) {
            return std::nullopt;
        }
    }

    switch (lhs_type.tag) {
        case CheckedType::Tag::TypeVariable: {
            if (generic_inferences->contains(lhs_type_id)) {
                TypeId seen_type_id = generic_inferences->at(lhs_type_id);

                if (rhs_type_id != seen_type_id) {
                    error = error.value_or(Error{
                            std::format("type mismatch; expected {}, but got {} instead",
                                        project.typename_for_type_id(seen_type_id),
                                        project.typename_for_type_id(rhs_type_id)),
                            span
                    });
                }
            } else {
                generic_inferences->insert({lhs_type_id, rhs_type_id});
            }
        } break;
        case CheckedType::Tag::GenericInstance: {
            RecordId lhs_record_id = lhs_type.generic_instance.record_id;
            Vec<TypeId> lhs_args = lhs_type.generic_instance.generic_arguments;
            CheckedType rhs_type = project.types[rhs_type_id];
            if (rhs_type.tag == CheckedType::Tag::GenericInstance) {
                RecordId rhs_record_id = rhs_type.generic_instance.record_id;
                if (lhs_record_id == rhs_record_id) {
                    Vec<TypeId> rhs_args = rhs_type.generic_instance.generic_arguments;

                    CheckedRecord lhs_record = project.records[lhs_record_id];
                    if (rhs_args.size() != lhs_args.size())
                        return Error{
                                std::format("mismatched number of generic parameters for {}", lhs_record.name),
                                span
                        };

                    for (usz idx = 0; idx < lhs_args.size(); idx++) {
                        TypeId lhs_arg_type_id = lhs_args[idx];
                        TypeId rhs_arg_type_id = rhs_args[idx];

                        Opt<Error> err = check_types_for_compat(lhs_arg_type_id, rhs_arg_type_id, generic_inferences, span, project);
                        if (err.has_value()) return err.value();
                    }
                }
            } else {
                if (rhs_type_id != lhs_type_id) {
                    error = error.value_or(Error{
                            std::format("type mismatch; expected {}, but got {} instead",
                                        project.typename_for_type_id(lhs_type_id),
                                        project.typename_for_type_id(rhs_type_id)),
                            span
                    });
                }
            }
        } break;
        case CheckedType::Tag::Record: {
            if (rhs_type_id == lhs_type_id) return std::nullopt;
            else error = error.value_or(Error{
                        std::format("type mismatch; expected {}, but got {} instead",
                                    project.typename_for_type_id(lhs_type_id),
                                    project.typename_for_type_id(rhs_type_id)),
                        span
                });
        } break;
        default:
            if (rhs_type_id != lhs_type_id)
                error = error.value_or(Error{
                        std::format("type mismatch; expected {}, but got {} instead",
                                    project.typename_for_type_id(lhs_type_id),
                                    project.typename_for_type_id(rhs_type_id)),
                        span
                });
            break;
    }

    return error;
}
