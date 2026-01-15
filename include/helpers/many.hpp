#ifndef QAT_HELPERS_MANY_HPP
#define QAT_HELPERS_MANY_HPP

#include <tuple>

template <typename... SubTypes> using Many = std::tuple<SubTypes...>;

#endif
