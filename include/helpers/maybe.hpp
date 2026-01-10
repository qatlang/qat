#ifndef QAT_HELPERS_MAYBE_HPP
#define QAT_HELPERS_MAYBE_HPP

#include <optional>

template <typename T> using Maybe = std::optional<T>;
constexpr std::nullopt_t None     = std::nullopt;

#endif
