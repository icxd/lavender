#pragma once

#include <tuple>
#include <utility>
#include "Common.hpp"
#include "Ast.hpp"
#include "Project.hpp"

Opt<Error> typecheck_namespace(const ParsedNamespace&, ScopeId, Project&);
Opt<Error> typecheck_record_predecl(const ParsedObject&, RecordId, ScopeId, Project&);
Opt<Error> typecheck_record(const ParsedObject&, RecordId, ScopeId, Project&);
Opt<Error> typecheck_method(const ParsedMethod&, RecordId, Project&);

std::tuple<CheckedStatement, Opt<Error>> typecheck_statement(ParsedStatement *, ScopeId, Project&, SafetyContext);
std::tuple<CheckedExpression, Opt<Error>> typecheck_expression(Expression *, ScopeId, Project&, SafetyContext, Opt<TypeId>);
std::tuple<CheckedBlock, Opt<Error>> typecheck_block(const Block<ParsedStatement *>&, ScopeId, Project&, SafetyContext);
std::tuple<TypeId, Opt<Error>> typecheck_typename(Type *, ScopeId, Project&);
std::tuple<TypeId, Opt<Error>> typecheck_binary_operation(CheckedExpression *, ExpressionDetails::Binary::Operation, CheckedExpression *, Span, Project&);

TypeId substitute_typevars_in_type(TypeId, Map<TypeId, TypeId> *, Project &);
TypeId substitute_typevars_in_type_helper(TypeId, Map<TypeId, TypeId> *, Project &);
Opt<Error> check_types_for_compat(TypeId, TypeId, Map<TypeId, TypeId> *, Span, Project &);