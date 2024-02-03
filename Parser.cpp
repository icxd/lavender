#include "Parser.hpp"
#include <sstream>
#include <iostream>

ErrorOr<Vec<Statement *>> Parser::parse() {
    Vec<Statement *> stmts{};
    while (m_pos < m_tokens.size() and not is(Token::Type::Eof)) {
        if (is(Token::Type::Newline)) { advance(); continue; }
        Statement *s = try$(stmt());
        if (s == nullptr)
            return error("check_statement is null, most likely a compiler bug.");
        stmts.push_back(s);
    }
    return stmts;
}

ErrorOr<Statement *> Parser::stmt() {
    switch (try$(current()).type) {
        case Token::Type::Object: return try$(object());
        case Token::Type::Interface: return try$(interface());
        case Token::Type::Fun:
        case Token::Type::Unsafe: return try$(fun());
        case Token::Type::Return: return try$(ret());
        default: return try$(var());
    }
}

ErrorOr<Statement *> Parser::object() {
    try$(expect(Token::Type::Object));
    try$(expect(Token::Type::Id));
    SpannedStr id = SpannedStr{previous().value.value(), previous().span};

    Vec<SpannedStr> interfaces{};
    if (is(Token::Type::OpenParen)) {
        try$(expect(Token::Type::OpenParen));
        while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and not is(Token::Type::CloseParen)) {
            try$(expect(Token::Type::Id));
            SpannedStr interface = SpannedStr{previous().value.value(), previous().span};
            interfaces.push_back(interface);

            if (is(Token::Type::CloseParen)) break;
            try$(expect(Token::Type::Comma));
        }
        try$(expect(Token::Type::CloseParen));
    }

    Opt<SpannedStr> parent{};
    if (is(Token::Type::GreaterThan)) {
        try$(expect(Token::Type::GreaterThan));
        try$(expect(Token::Type::Id));
        parent = std::make_optional(SpannedStr{previous().value.value(), previous().span});
    }

    Vec<Field> fields{};
    Vec<Method> methods{};
    try$(expect(Token::Type::Colon));
    try$(expect(Token::Type::Indent));
    while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and not is(Token::Type::Dedent)) {
        if (is(Token::Type::Fun) or is(Token::Type::Unsafe))
            methods.push_back(try$(method()));
        else fields.push_back(try$(field()));

        while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and is(Token::Type::Newline))
            advance();
    }
    if (is(Token::Type::Eof)) try$(expect(Token::Type::Eof));
    else if (is(Token::Type::Dedent)) try$(expect(Token::Type::Dedent));

    return new Statement{
        .var = new StatementDetails::Object{id, parent, interfaces, fields, methods}
    };
}

ErrorOr<Statement *> Parser::interface() {
    try$(expect(Token::Type::Interface));
    try$(expect(Token::Type::Id));
    SpannedStr id = SpannedStr{previous().value.value(), previous().span};

    Vec<SpannedStr> interfaces{};
    if (is(Token::Type::OpenParen)) {
        try$(expect(Token::Type::OpenParen));
        while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and not is(Token::Type::CloseParen)) {
            try$(expect(Token::Type::Id));
            SpannedStr interface = SpannedStr{previous().value.value(), previous().span};
            interfaces.push_back(interface);

            if (is(Token::Type::CloseParen)) break;
            try$(expect(Token::Type::Comma));
        }
        try$(expect(Token::Type::CloseParen));
    }

    Block<ErrorOr<Method>> raw_methods = try$(block<ErrorOr<Method>>([&] { return method(); }));
    Vec<Method> methods{};
    for (const auto& method : raw_methods.elems) {
        methods.push_back(try$(method));
    }

    return new Statement{
        .var = new StatementDetails::Interface{
            id,
            interfaces,
            methods,
        }
    };
}

ErrorOr<Statement *> Parser::fun() {
    Method m = try$(method());

    return new Statement{
        .var = new StatementDetails::FunDecl{m.id, m.parameters, m.ret_type, m.body, m.unsafe}
    };
}

ErrorOr<Statement *> Parser::ret() {
    Span span = try$(current()).span;
    try$(expect(Token::Type::Return));
    Opt<Expression *> value = std::make_optional(try$(expr()));
    return new Statement{ .var = new StatementDetails::Return{span, value} };
}

ErrorOr<Statement *> Parser::var() {
    Type *ty = try$(type());
    try$(expect(Token::Type::Id));
    SpannedStr id = SpannedStr{previous().value.value(), previous().span};
    try$(expect(Token::Type::Equals));
    Expression *ex = try$(expr());
    return new Statement{ .var = new StatementDetails::VarDecl{ty, id, ex} };
}

ErrorOr<Expression *> Parser::expr() {return primary();}
ErrorOr<Expression *> Parser::primary() {
    Expression *expression = nullptr;
    switch (try$(current()).type) {
        case Token::Type::Null: {
            Span span = try$(current()).span;
            try$(expect(Token::Type::Null));
            expression = new Expression{.var = new ExpressionDetails::Null{span}};
        } break;
        case Token::Type::Id: {
            try$(expect(Token::Type::Id));
            expression = new Expression{.var = new ExpressionDetails::Id{previous().value.value()}};
        } break;
        case Token::Type::Int: {
            int value = std::stoi(try$(current()).value.value());
            try$(expect(Token::Type::Int));
            expression = new Expression{.var = new ExpressionDetails::Int{value}};
        } break;
        case Token::Type::String: {
            Str value = try$(current()).value.value();
            try$(expect(Token::Type::String));
            expression = new Expression{.var = new ExpressionDetails::String{value}};
        } break;
        case Token::Type::Switch: {
            try$(expect(Token::Type::Switch));
            Expression *condition = try$(expr());

            Block<ErrorOr<Pattern *>> raw_patterns = try$(block<ErrorOr<Pattern *>>([&] { return pattern(); }));
            Vec<Pattern *> patterns{};
            for (const auto& pattern : raw_patterns.elems) {
                patterns.push_back(try$(pattern));
            }

        } break;
        case Token::Type::Unsafe: {
            try$(expect(Token::Type::Unsafe));
            Block<ErrorOr<Expression *>> raw_body = try$(block<ErrorOr<Expression *>>([&] { return expr(); }));
            Vec<Expression *> exprs{};
            for (const auto& expr : raw_body.elems) {
                exprs.push_back(try$(expr));
            }

            expression = new Expression{
                .var = new ExpressionDetails::UnsafeBlock{exprs},
            };
        } break;
        default:
            return error("expected an expression (such as an integer or a string) but got ", Token::repr(try$(current()).type), " instead");
    }
    if (expression == nullptr)
        return error("expected an expression (such as an integer or a string) but got ", Token::repr(try$(current()).type), " instead");
    return postfix(expression);
}

ErrorOr<Expression *> Parser::postfix(Expression *expression) {
    switch (try$(current()).type) {
        case Token::Type::OpenParen: {
            Span span = previous().span;
            advance();
            Vec<Argument> args{};
            while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and not is(Token::Type::CloseParen)) {
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
            return postfix(new Expression{.var = new ExpressionDetails::Call{span, expression, args}});
        }
        default:
            return expression;
    }
}

ErrorOr<Type *> Parser::type() {
    Type *ty = nullptr;
    switch (try$(current()).type) {
        case Token::Type::Id:
            advance();
            ty = new Type{Type::Kind::Id, SpannedStr{previous().value.value(), previous().span}};
            break;
        case Token::Type::StrType:
            advance();
            ty = new Type{Type::Kind::Str};
            break;
        case Token::Type::IntType:
            advance();
            ty = new Type{Type::Kind::Int};
            break;
        case Token::Type::OpenBracket: {
            try$(expect(Token::Type::OpenBracket));
            Type *subtype = try$(type());
            try$(expect(Token::Type::CloseBracket));
            ty = new Type{.type = Type::Kind::Array, .subtype = subtype};
        }
            break;
        case Token::Type::Weak: {
            try$(expect(Token::Type::Weak));
            Type *subtype = try$(type());
            ty = new Type{.type = Type::Kind::Weak, .subtype = subtype};
        }
            break;
        case Token::Type::Raw: {
            try$(expect(Token::Type::Raw));
            Type *subtype = try$(type());
            ty = new Type{.type = Type::Kind::Raw, .subtype = subtype};
        }
            break;
        default:
            return error("expected `str`, `int`, or `[` but got `",
                         Token::repr(try$(current()).type), "` instead");
    }

    if (is(Token::Type::Question)) {
        try$(expect(Token::Type::Question));
        ty = new Type{.type = Type::Kind::Optional, .subtype = ty};
    }

    return ty;
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

ErrorOr<Method> Parser::method() {
    bool unsafe = false;
    if (is(Token::Type::Unsafe)) {
        try$(expect(Token::Type::Unsafe));
        unsafe = true;
    }

    try$(expect(Token::Type::Fun));
    try$(expect(Token::Type::Id));
    SpannedStr id = SpannedStr{previous().value.value(), previous().span};

    Vec<Field> parameters{};
    try$(expect(Token::Type::OpenParen));
    while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and not is(Token::Type::CloseParen)) {
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

    if (not is(Token::Type::Colon) and not is(Token::Type::Arrow))
        return Method{id, parameters, ret_type, Block{Vec<Statement *>()}, unsafe};

    Block<ErrorOr<Statement *>> raw_body = try$(block<ErrorOr<Statement *>>([&] { return stmt(); }));
    Vec<Statement *> stmts{};
    for (const auto& stmt : raw_body.elems) {
        stmts.push_back(try$(stmt));
    }

    return Method{id, parameters, ret_type, Block{stmts}, unsafe};
}

ErrorOr<Pattern *> Parser::pattern() {
    if (is(Token::Type::Case)) {
        try$(expect(Token::Type::Case));
        PatternCondition condition = try$(pattern_condition());
        if (is(Token::Type::Arrow)) {
            try$(expect(Token::Type::Arrow));
            Expression *value = try$(expr());
            if (is(Token::Type::Newline))
                try$(expect(Token::Type::Newline));
        } else {
            // Block - not supported yet.
        }
    }

    if (is(Token::Type::Default)) {
    }
}

ErrorOr<PatternCondition> Parser::pattern_condition() { }

template <typename T> ErrorOr<Block<T>> Parser::block(std::function<T()> fn) {
    Vec<T> elems{};
    if (is(Token::Type::Arrow)) {
        try$(expect(Token::Type::Arrow));
        elems.push_back(try$(fn()));
        return Block<T>{elems};
    }
    try$(expect(Token::Type::Colon));
    while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and is(Token::Type::Newline)) advance();
    try$(expect(Token::Type::Indent));
    while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and not is(Token::Type::Dedent)) {
        elems.push_back(try$(fn()));
        while (m_pos < m_tokens.size() and not is(Token::Type::Eof) and is(Token::Type::Newline))
            advance();
    }
    if (is(Token::Type::Eof)) try$(expect(Token::Type::Eof));
    else if (is(Token::Type::Dedent)) try$(expect(Token::Type::Dedent));
    return Block<T>{elems};
}

ErrorOr<Token> Parser::current() {
    if (m_pos >= m_tokens.size())
        return error("unexpected end of file");
    return m_tokens[m_pos];
}
Token Parser::previous() const { return m_tokens[m_pos - 1]; }
bool Parser::is(Token::Type type) { return m_pos < m_tokens.size() and current().value().type == type; }
Token Parser::advance() { return m_tokens[m_pos++]; }
ErrorOr<Token> Parser::expect(Token::Type type) {
    if (!is(type)) return error(type, try$(current()).type);
    return advance();
}

Error Parser::error(Token::Type expects, Token::Type got) {
    return error("expected `", Token::repr(expects), "`, but got `", Token::repr(got),
          "` instead");
}
template <typename... Args> Error Parser::error(Args... args) {
    std::stringstream ss;
    (ss << ... << args);
    return {ss.str(), (m_pos < m_tokens.size() ? m_tokens[m_pos] : m_tokens[m_pos - 1]).span };
}
