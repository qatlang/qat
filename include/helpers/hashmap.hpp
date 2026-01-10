#ifndef QAT_HELPERS_HASHMAP_HPP
#define QAT_HELPERS_HASHMAP_HPP

#include <unordered_map>

template <typename A, typename B> using HashMap      = std::unordered_map<A, B>;
template <typename A, typename B> using HashMultiMap = std::unordered_multimap<A, B>;

#endif
