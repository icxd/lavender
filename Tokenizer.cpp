#include "Tokenizer.hpp"
#include <sstream>

static Token::Type ident_type(Str s) {
    switch (s[0]) {
    case 'e':
        if (s == "else") return Token::Type::Else;
        if (s == "elif") return Token::Type::Elif;
        break;
    case 'f':
        if (s == "false") return Token::Type::False;
        if (s == "fun") return Token::Type::Fun;
        break;
    case 'g':
        if (s == "guard") return Token::Type::Guard;
        break;
    case 'i':
        if (s == "if") return Token::Type::If;
        if (s == "int") return Token::Type::IntType;
        break;
    case 'o':
        if (s == "object") return Token::Type::Object;
        break;
    case 'r':
        if (s == "return") return Token::Type::Return;
        break;
    case 's':
        if (s == "str") return Token::Type::StrType;
        break;
    case 't':
        if (s == "true") return Token::Type::True;
        break;
    }
    return Token::Type::Id;
}

TokenizeResult tokenize(const char *filename, Str source) {
    Vec<Token> tokens{};
    Vec<Error> errors{};

    usz pos = 0, line = 1, column = 1;
    bool continues = false, string_interp = false;
    Vec<usz> indent_stack{0};

    auto make_span = [&](usz len = 1) -> Span {
        return Span{filename, line, column - len, len};
    };

    auto advance = [&](usz offset = 1) {
        pos += offset;
        column += offset;
    };

    while (pos < source.length()) {
        switch (source[pos]) {
            case '\0':
                return {tokens, errors};

            case '\r':
            case ' ':
                advance();
                break;

            case '\n': {
                pos++;

                usz indent = 0;
                while (source[pos] == ' ') {
                    indent++;
                    advance();
                }
                if (continues) goto end;

                if (indent > indent_stack.back()) {
                    tokens.push_back(
                        Token{Token::Type::Indent, {}, make_span(indent)});
                    indent_stack.push_back(indent);
                } else if (indent < indent_stack.back()) {
                    tokens.push_back(
                        Token{Token::Type::Dedent, {}, make_span(indent)});
                    indent_stack.pop_back();
                } else {
                    tokens.push_back(
                        Token{Token::Type::Newline, {}, make_span(indent)});
                }

            end:
                column = indent + 1;
                line++;
            } break;

            case '/':
                if (pos + 1 < source.length() && source[pos + 1] == '/') {
                    while (pos < source.length() && source[pos] != '\n') {
                        advance();
                    }
                }
                break;

            case '=':
                advance();
                tokens.push_back(Token{Token::Type::Equals, {}, make_span()});
                break;

            case '>':
                advance();
                tokens.push_back(Token{Token::Type::GreaterThan, {}, make_span()});
                break;

            case '(':
                advance();
                tokens.push_back(Token{Token::Type::OpenParen, {}, make_span()});
                continues = true;
                break;

            case ')':
                advance();
                tokens.push_back(Token{Token::Type::CloseParen, {}, make_span()});
                continues = false;
                break;

            case '[':
                advance();
                tokens.push_back(Token{Token::Type::OpenBracket, {}, make_span()});
                continues = true;
                break;

            case ']':
                advance();
                tokens.push_back(Token{Token::Type::CloseBracket, {}, make_span()});
                continues = false;
                break;

            case ',':
                advance();
                tokens.push_back(Token{Token::Type::Comma, {}, make_span()});
                break;

            case ':':
                advance();
                tokens.push_back(Token{Token::Type::Colon, {}, make_span()});
                break;

            case '.': {
                if (pos + 1 < source.length() && std::isdigit(source[pos + 1])) {
                    Str value{source[pos]};
                    advance();
                    while (std::isdigit(source[pos])) {
                        value.push_back(source[pos]);
                        advance();
                    }
                    tokens.push_back(Token{Token::Type::Float, value,
                                           make_span(value.length())});
                } else if (pos + 1 < source.length() && source[pos + 1] == '.') {
                    advance(2);
                    tokens.push_back(Token{Token::Type::Range, {}, make_span(2)});
                } else {
                    advance();
                    tokens.push_back(Token{Token::Type::Dot, {}, make_span()});
                }
            } break;

            case '"': {
                advance();
                Str value{};
                while (pos < source.length() && source[pos] != '"') {
                    switch (source[pos]) {
                    case '\\': {
                        advance();
                        switch (source[pos]) {
                        case '\\':
                            value.append("\\");
                            break;

                        case 't':
                            value.append("\t");
                            break;

                        case 'n':
                            value.append("\n");
                            break;

                        case 'r':
                            value.append("\r");
                            break;

                        default: {
                            std::stringstream ss;
                            ss << "invalid escape sequence `" << source[pos] << "`";
                            errors.push_back(Error{ss.str(), make_span()});
                        } break;
                        }
                        advance();
                    } break;

                    case '{': {
                        if (pos + 1 < source.length() && source[pos + 1] == '{') {
                            value.push_back('{');
                            advance(2);
                        }
                        errors.push_back(
                            Error{"open braces (`{`) must be escaped (`{{`)",
                                  make_span()});
                        advance();
                    } break;

                    case '}': {
                        if (pos + 1 < source.length() && source[pos + 1] == '}') {
                            value.push_back('}');
                            advance(2);
                        }
                        errors.push_back(
                            Error{"closing braces (`}`) must be escaped (`}}`)",
                                  make_span()});
                        advance();
                    } break;

                    default:
                        value.push_back(source[pos]);
                        advance();
                        break;
                    }
                }
                advance();
                tokens.push_back(
                    Token{Token::Type::String, value, make_span(value.length())});
            } break;

            default: {
                if (std::isalpha(source[pos]) || source[pos] == '_') {
                    Str value{};
                    while (std::isalnum(source[pos]) || source[pos] == '_') {
                        value.push_back(source[pos]);
                        pos++;
                        column++;
                    }
                    tokens.push_back(Token{ident_type(value),
                                           std::make_optional(value),
                                           make_span(value.length())});
                    continue;
                }
                if (std::isdigit(source[pos])) {
                    Str value{""};
                    bool is_float = false;

                    if (source[pos] == '0' && pos + 1 < source.length()) {
                        if (source[pos + 1] == 'x') {
                            pos += 2;
                            column += 2;
                            value = "0x";
                            while (pos < source.length() &&
                                   ((source[pos] >= '0' && source[pos] <= '9') ||
                                    (source[pos] >= 'a' && source[pos] <= 'f') ||
                                    (source[pos] >= 'A' && source[pos] <= 'F'))) {
                                value.push_back(source[pos]);
                                pos++;
                                column++;
                            }
                            goto done;
                        } else if (source[pos + 1] == 'b') {
                            pos += 2;
                            column += 2;
                            value = "0b";
                            while (pos < source.length() &&
                                   (source[pos] == '0' || source[pos] == '1')) {
                                value.push_back(source[pos]);
                                pos++;
                                column++;
                            }
                            goto done;
                        }
                    }

                    while (std::isdigit(source[pos])) {
                        value.push_back(source[pos]);
                        pos++;
                        column++;
                    }

                    if (source[pos] == '.') {
                        if (source[pos + 1] == '.') goto done; // This is a range.

                        pos++;
                        column++;
                        value.push_back('.');

                        is_float = true;
                        while (std::isdigit(source[pos])) {
                            value.push_back(source[pos]);
                            pos++;
                            column++;
                        }
                    }

                done:
                    tokens.push_back(Token{
                        is_float ? Token::Type::Float : Token::Type::Int,
                        std::make_optional(value), make_span(value.length())});
                    continue;
                }
                std::stringstream ss;
                ss << "unexpected character `" << source[pos] << "`";
                errors.push_back(Error{ss.str(), make_span()});
                advance();
            } break;
        }
    }

    tokens.push_back(Token{Token::Type::Eof, {}, make_span()});

    return {tokens, errors};
}

Vec<Token> normalize(Vec<Token> tokens) {
    Vec<Token> toks{};
    for (usz i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Token::Type::Dedent) {
            if (i + 1 < tokens.size() &&
                tokens[i + 1].type == Token::Type::Indent) {
                toks.push_back(Token{Token::Type::Newline, {}, tokens[i].span});
                i++;
            } else {
                toks.push_back(tokens[i]);
            }
        } else {
            toks.push_back(tokens[i]);
        }
    }
    return toks;
}