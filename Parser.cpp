#include "Parser.hpp"
#include <sstream>
#include <iostream>

ErrorOr<Vec<Statement *>> Parser::parse() {
    Vec<Statement *> stmts{};
    while (not is(Token::Type::Eof) and not is(Token::Type::Eof)) {
        if (is(Token::Type::Newline)) { advance(); continue; }
        Statement *s = try$(stmt());
        if (s == nullptr)
            return error("statement is null, most likely a compiler bug.");
        stmts.push_back(s);
    }
    return stmts;
}

ErrorOr<Statement *> Parser::stmt() {
    switch (current().type) {
        case Token::Type::Object: return try$(object());
        case Token::Type::Fun: return try$(fun());
        case Token::Type::Return: return try$(ret());
        default: return try$(var());
    }
}

ErrorOr<Statement *> Parser::object() {
    try$(expect(Token::Type::Object));
    try$(expect(Token::Type::Id));
    SpannedStr id = SpannedStr{previous().value.value(), previous().span};
    Opt<SpannedStr> parent{};
    if (is(Token::Type::GreaterThan)) {
        expect(Token::Type::GreaterThan);
        expect(Token::Type::Id);
        parent = std::make_optional(Spanned{previous().value.value(), previous().span});
    }

    Vec<Field> fields{};
    try$(expect(Token::Type::Colon));
    try$(expect(Token::Type::Indent));
    while (not is(Token::Type::Eof) and !is(Token::Type::Dedent)) {
        fields.push_back(try$(field()));
        if (not is(Token::Type::Dedent))
            try$(expect(Token::Type::Newline));
        else break;
    }
    try$(expect(Token::Type::Dedent));

    return new Statement{
        .var = new StatementDetails::Object{id, parent, {}, fields}
    };
}

ErrorOr<Statement *> Parser::fun() {
    try$(expect(Token::Type::Fun));
    try$(expect(Token::Type::Id));
    SpannedStr id = SpannedStr{previous().value.value(), previous().span};

    Vec<Field> parameters{};
    try$(expect(Token::Type::OpenParen));
    while (not is(Token::Type::Eof) and not is(Token::Type::CloseParen)) {
        Type *ty = try$(type());

        try$(expect(Token::Type::Id));
        Str param = previous().value.value();

        parameters.push_back({ty, param, {}});
    }
    try$(expect(Token::Type::CloseParen));

    Opt<Type *> ret_type{};
    if (is(Token::Type::GreaterThan)) {
        try$(expect(Token::Type::GreaterThan));
        ret_type = std::make_optional(try$(type()));
    }

    Block<ErrorOr<Statement *>> raw_body = try$(block<ErrorOr<Statement *>>([&] { return stmt(); }));
    Vec<Statement *> stmts{};
    for (const auto& stmt : raw_body.elems) {
        stmts.push_back(try$(stmt));
    }

    return new Statement{
        .var = new StatementDetails::FunDecl{id, parameters, ret_type, Block<Statement *>{stmts} }
    };
}

ErrorOr<Statement *> Parser::ret() {
    try$(expect(Token::Type::Return));
    Opt<Expression *> value = std::make_optional(try$(expr()));
    return new Statement{ .var = new StatementDetails::Return{value} };
}

ErrorOr<Statement *> Parser::var() {
    Type *ty = try$(type());
    try$(expect(Token::Type::Id));
    SpannedStr id = Spanned{previous().value.value(), previous().span};
    try$(expect(Token::Type::Equals));
    Expression *ex = try$(expr());
    return new Statement{ .var = new StatementDetails::VarDecl{ty, id, ex} };
}

ErrorOr<Expression *> Parser::expr() {return primary();}
ErrorOr<Expression *> Parser::primary() {
    Expression *expression;
    switch (current().type) {
        case Token::Type::Id: {
            try$(expect(Token::Type::Id));
            expression = new Expression{.var = new ExpressionDetails::Id{previous().value.value()}};
        } break;
        case Token::Type::Int: {
            int value = std::stoi(current().value.value());
            try$(expect(Token::Type::Int));
            expression = new Expression{.var = new ExpressionDetails::Int{value}};
        } break;
        case Token::Type::String: {
            Str value = current().value.value();
            try$(expect(Token::Type::String));
            expression = new Expression{.var = new ExpressionDetails::String{value}};
        } break;
        default:
            return error("expected an expression (such as an integer or a string) but got ", Token::repr(current().type), " instead");
    }
    return postfix(expression);
}

ErrorOr<Expression *> Parser::postfix(Expression *expression) {
    switch (current().type) {
        case Token::Type::OpenParen: {
            advance();
            Vec<Argument> args{};
            while (not is(Token::Type::Eof) and not is(Token::Type::CloseParen)) {
                Opt<SpannedStr> id{};
                if (is(Token::Type::Id)) {
                    try$(expect(Token::Type::Id));
                    id = std::make_optional(SpannedStr{previous().value.value(), previous().span});
                    try$(expect(Token::Type::Colon));
                }
                Expression *ex = try$(expr());
                args.push_back({id, ex});
                if (not is(Token::Type::CloseParen))
                    try$(expect(Token::Type::Comma));
            }
            try$(expect(Token::Type::CloseParen));
            return postfix(new Expression{.var = new ExpressionDetails::Call{expression, args}});
        }
        default:
            return expression;
    }
}

ErrorOr<Type *> Parser::type() {
    switch (current().type) {
        case Token::Type::Id: advance(); return new Type{Type::Kind::Id, SpannedStr{previous().value.value(), previous().span}};
        case Token::Type::StrType: advance(); return new Type{Type::Kind::Str};
        case Token::Type::IntType: advance(); return new Type{Type::Kind::Int};
        case Token::Type::OpenBracket: {
            try$(expect(Token::Type::OpenBracket));
            Type *subtype = try$(type());
            try$(expect(Token::Type::CloseBracket));
            return new Type{.type = Type::Kind::Array, .subtype = subtype};
        }
        default:
            return error("expected `str`, `int`, or `[` but got `",
                  Token::repr(current().type), "` instead");
    }
}

ErrorOr<Field> Parser::field() {
    Type *ty = try$(type());
    try$(expect(Token::Type::Id));
    Span span = previous().span;
    Str id = previous().value.value();
    Opt<Expression *> value = {};
    if (is(Token::Type::Equals)) {
        try$(expect(Token::Type::Equals));
        Expression *e = try$(expr());
        if (e == nullptr) return Field{ty, id, {}};
        value = std::make_optional(e);
    }
    return Field{ty, SpannedStr{id, span}, value};
}

template <typename T> ErrorOr<Block<T>> Parser::block(std::function<T()> fn) {
    Vec<T> elems{};
    try$(expect(Token::Type::Colon));
    while (not is(Token::Type::Eof) and is(Token::Type::Newline)) advance();
    try$(expect(Token::Type::Indent));
    while (not is(Token::Type::Eof) and not is(Token::Type::Dedent)) {
        elems.push_back(fn());
        while (is(Token::Type::Newline))
            advance();
    }
    if (is(Token::Type::Eof)) try$(expect(Token::Type::Eof));
    else try$(expect(Token::Type::Dedent));
    return Block<T>{elems};
}

Token Parser::current() const {
    if (m_pos >= m_tokens.size()) return previous();
    return m_tokens[m_pos];
}
Token Parser::previous() const { return m_tokens[m_pos - 1]; }
bool Parser::is(Token::Type type) const { return current().type == type; }
Token Parser::advance() { return m_tokens[m_pos++]; }
ErrorOr<Token> Parser::expect(Token::Type type) {
    if (!is(type)) return error(type, current().type);
    return advance();
}

Error Parser::error(Token::Type expects, Token::Type got) {
    return error("expected `", Token::repr(expects), "`, but got `", Token::repr(got),
          "` instead");
}
template <typename... Args> Error Parser::error(Args... args) {
    std::stringstream ss;
    (ss << ... << args);
    return {ss.str(), current().span};
}