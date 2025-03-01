#ifndef QAT_UTILS_HELPERS_HPP
#define QAT_UTILS_HELPERS_HPP

#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace qat {

using i64                                    = int64_t;
using i32                                    = int32_t;
using i16                                    = int16_t;
using i8                                     = int8_t;
using u64                                    = uint64_t;
using u32                                    = uint32_t;
using u16                                    = uint16_t;
using u8                                     = uint8_t;
using f32                                    = float;
using f64                                    = double;
using usize                                  = std::size_t;
using VoidPtr                                = void*;
using String                                 = std::string;
using StringView                             = std::string_view;
template <typename T> using Maybe            = std::optional<T>;
template <typename T> using Vec              = std::vector<T>;
template <typename T> using Deque            = std::deque<T>;
template <typename F, typename S> using Pair = std::pair<F, S>;
template <typename A, typename B> using Map  = std::map<A, B>;
namespace fs                                 = std::filesystem;
const std::nullopt_t None                    = std::nullopt;
template <typename T> using Shared           = std::shared_ptr<T>;
template <typename T> using Unique           = std::unique_ptr<T>;

} // namespace qat

#endif
