#pragma once

#include "Ast.hpp"
#include "Common.hpp"
#include "Token.hpp"
#include <functional>
#include <utility>

class Parser {
  public:
    explicit Parser(Vec<Token> tokens) : m_tokens(std::move(tokens)), m_errors({}), m_pos(0) {}

    ErrorOr<Vec<ParsedStatement *>> parse();

    ErrorOr<ParsedStatement *> stmt();
    ErrorOr<ParsedStatement *> object();
    ErrorOr<ParsedStatement *> interface();
    ErrorOr<ParsedStatement *> fun();
    ErrorOr<ParsedStatement *> ret();
    ErrorOr<ParsedStatement *> var();

    ErrorOr<Expression *> expr();
    ErrorOr<Expression *> binary();
    ErrorOr<Expression *> unary();
    ErrorOr<Expression *> primary();
    ErrorOr<Expression *> postfix(Expression *);

    ErrorOr<Type *> type();

    ErrorOr<ParsedField> field();
    ErrorOr<ParsedMethod> method();
    ErrorOr<Vec<Type *>> generics();

    ErrorOr<Pattern *> pattern();
    ErrorOr<PatternCondition> pattern_condition();

    template <typename T> ErrorOr<Block<T>> block(Fn<T()>);

    [[nodiscard]] ErrorOr<Token> current();
    [[nodiscard]] Token previous() const;
    [[nodiscard]] bool is(Token::Type);
    Token advance();
    ErrorOr<Token> expect(Token::Type);

    Error error(Token::Type, Token::Type);
    template <typename... Args> Error error(Args...);

    [[nodiscard]] ParsedNamespace parsed_namespace() const { return m_parsed_namespace; }
    [[nodiscard]] Vec<Token> tokens() const { return m_tokens; }
    [[nodiscard]] Vec<Error> errors() const { return m_errors; }
    [[nodiscard]] usz pos() const { return m_pos; }

  private:
    ParsedNamespace m_parsed_namespace{};

    Vec<Token> m_tokens;
    Vec<Error> m_errors;
    usz m_pos{0};
};