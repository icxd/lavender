#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <functional>

using usz = unsigned long;

using Str = std::string;
template <typename T> using Opt = std::optional<T>;
template <typename T> using Vec = std::vector<T>;
template <typename... Args> using Var = std::variant<Args...>;

struct Span {
    const char *filename;
    usz line, column, length;
};

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

    T unwrap_or(const std::function<void(Error error)>& fn) const {
        if (not has_value()) fn(error());
        return value();
    }

private:
    bool m_has_value;
    Var<T, Error> m_result_or_error;
};

#define try$(expr) \
    ({              \
        auto __temp_val = (expr); \
        if (not __temp_val.has_value()) return __temp_val.error(); \
        __temp_val.value();               \
    })