#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <functional>
#include <map>
#include <memory>

using u8 = unsigned char;
using usz = unsigned long;

using Str = std::string;
template <typename T> using Opt = std::optional<T>;
template <typename T> using Vec = std::vector<T>;
template <typename T> using Unique = std::unique_ptr<T>;
template <typename... Args> using Var = std::variant<Args...>;
template <typename T, typename U> using Map = std::map<T, U>;
template <typename T> using Fn = std::function<T>;

struct Void {};

struct Span {
    const char *filename;
    usz line, column, length;

    Span extend(Span other) {
        if (other.line < line) {
            line = other.line;
            column = other.column;
        }
        if (other.line == line and other.column < column) {
            column = other.column;
        }
        if (other.line == line and other.column == column and other.length > length) {
            length = other.length;
        }

        return *this;
    }
};

template <typename T> struct Spanned {
    T value;
    Span span;
};

template <typename T> struct Id {
    Str id;
    T value;
};

using SpannedStr = Spanned<Str>;

struct Error {
    Str message;
    Span span;
};

template <typename T> requires (not std::is_same_v<T, Error>)
class ErrorOr {
public:
    ErrorOr(T value) : m_has_value(true), m_result_or_error(value) {}
    ErrorOr(Error error) : m_has_value(false), m_result_or_error(error) {}

    [[nodiscard]] bool has_value() const { return m_has_value; }
    T value() const { return std::get<T>(m_result_or_error); }
    [[nodiscard]] Error error() const { return std::get<Error>(m_result_or_error); }

    T value_or(T other) { return m_has_value ? value() : other; }

private:
    bool m_has_value;
    Var<T, Error> m_result_or_error;
};

#define try$(expr) \
    ({              \
        auto __temp_val = (expr); \
        if (not __temp_val.has_value()) return __temp_val.error(); \
        __temp_val.value();       \
    })

#define PANIC(msg, ...) panic(__FILE__, __LINE__, msg, ##__VA_ARGS__)
#define UNIMPLEMENTED(x) PANIC("unimplemented: %s in %s", x, __PRETTY_FUNCTION__)

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

[[noreturn]] void panic(const char *file, usz line, const char *fmt, ...);

static inline std::vector<std::string> split(std::string str, char delim = ' ') {
    std::vector<std::string> result;
    size_t start = 0;
    while (start < str.length()) {
        size_t pos = str.find(delim, start);
        if (pos == std::string::npos) pos = str.length();
        result.push_back(str.substr(start, pos - start));
        start = pos + 1;
    }
    return result;
}