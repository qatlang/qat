#ifndef QAT_HELPERS_POINTERS_HPP
#define QAT_HELPERS_POINTERS_HPP

#include <memory>

template <typename T> using Own = std::unique_ptr<T>;

#endif
