#pragma once

#include "Common.hpp"

#define TOKENS                                                                 \
    X(Id, "identifier")                                                        \
    X(Int, "integer")                                                          \
    X(Float, "float")                                                          \
    X(String, "string")                                                        \
    X(Null, "null")                                                            \
                                                                               \
    X(If, "if")                                                                \
    X(Then, "then")                                                            \
    X(Elif, "elif")                                                            \
    X(Else, "else")                                                            \
    X(Guard, "guard")                                                          \
    X(True, "true")                                                            \
    X(False, "false")                                                          \
    X(Object, "object")                                                        \
    X(Interface, "interface")                                                  \
    X(Static, "static")                                                        \
    X(Fun, "fun")                                                              \
    X(Return, "return")                                                        \
    X(Switch, "switch")                                                        \
    X(Case, "case")                                                            \
    X(Default, "default")                                                      \
    X(Unsafe, "unsafe")                                                        \
                                                                               \
    X(StrType, "str")                                                          \
    X(IntType, "int")                                                          \
                                                                               \
    X(Weak, "weak")                                                            \
    X(Raw, "raw")                                                              \
                                                                               \
    X(Minus, "-")                                                              \
    X(Asterisk, "*")                                                           \
    X(Equals, "=")                                                             \
    X(EqualsEquals, "==")                                                      \
    X(GreaterThan, ">")                                                        \
    X(LessThan, "<")                                                           \
    X(BitwiseAnd, "&")                                                         \
                                                                               \
    X(OpenParen, "(")                                                          \
    X(CloseParen, ")")                                                         \
    X(OpenBracket, "[")                                                        \
    X(CloseBracket, "]")                                                       \
    X(Colon, ":")                                                              \
    X(Comma, ",")                                                              \
    X(Dot, ".")                                                                \
    X(Question, "?")                                                           \
    X(Range, "..")                                                             \
    X(Arrow, "->")                                                             \
                                                                               \
    X(Indent, "indent")                                                        \
    X(Dedent, "dedent")                                                        \
    X(Newline, "newline")                                                      \
    X(Eof, "end of file")

struct Token {
    enum class Type {
#define X(id, repr) id,
        TOKENS
#undef X
    };

    static const char *type_to_string(Type type) {
        const char *strings[] = {
#define X(id, repr) #id,
            TOKENS
#undef X
        };
        return strings[static_cast<usz>(type)];
    }

    static const char *repr(Type type) {
        const char *strings[] = {
#define X(id, repr) repr,
            TOKENS
#undef X
        };
        return strings[static_cast<usz>(type)];
    }

    Type type;
    Opt<Str> value;
    Span span{};

    [[nodiscard]] inline u8 precedence() const {
        switch (type) {
            case Type::EqualsEquals: return 1;
            default: return 0;
        }
    }

    [[nodiscard]] inline bool is_binary() const {
        switch (type) {
            case Type::EqualsEquals:
                return true;
            default:
                return false;
        }
    }
};