#ifndef QAT_HELPERS_ARRAY_HPP
#define QAT_HELPERS_ARRAY_HPP

#include <array>

template <typename T, const std::size_t S> using Array = std::array<T, S>;

#endif
