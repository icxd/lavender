#pragma once

#include "Ast.hpp"
#include "Common.hpp"
#include "Token.hpp"
#include <functional>
#include <utility>

class Parser {
  public:
    explicit Parser(Vec<Token> tokens) : m_tokens(std::move(tokens)), m_errors({}), m_pos(0) {}

    ErrorOr<Vec<Stmt *>> parse();

    ErrorOr<Stmt *> stmt();
    ErrorOr<Stmt *> object();
    ErrorOr<Stmt *> fun();
    ErrorOr<Stmt *> ret();
    ErrorOr<Stmt *> var();

    ErrorOr<Expr *> expr();
    ErrorOr<Expr *> primary();

    ErrorOr<Type *> type();

    ErrorOr<Field> field();

    template <typename T> ErrorOr<Block<T>> block(std::function<T()>);

    [[nodiscard]] Token current() const;
    [[nodiscard]] Token previous() const;
    [[nodiscard]] bool is(Token::Type) const;
    Token advance();
    ErrorOr<Token> expect(Token::Type);

    Error error(Token::Type, Token::Type);
    template <typename... Args> Error error(Args...);

    [[nodiscard]] Vec<Token> tokens() const { return m_tokens; }
    [[nodiscard]] Vec<Error> errors() const { return m_errors; }
    [[nodiscard]] usz pos() const { return m_pos; }

  private:
    Vec<Token> m_tokens;
    Vec<Error> m_errors;
    usz m_pos{0};
};