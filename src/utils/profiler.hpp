#ifndef QAT_UTILS_PROFILER_HPP
#define QAT_UTILS_PROFILER_HPP

#include <chrono>
#include <unordered_map>

#include "./helpers.hpp"
#include "./macros.hpp"

#if ENABLE_PROFILER
#if RUNTIME_IS_MSVC
#define PROFILE_THIS     auto _profiler_scope = ProfileScope(__FUNCSIG__);
#define PROFILE_SCOPE(x) auto _profiler_local_scope = ProfileScope(std::string(__FUNCSIG__) + x);
#else
#define PROFILE_THIS     auto _profiler_scope = ProfileScope(__PRETTY_FUNCTION__);
#define PROFILE_SCOPE(x) auto _profiler_local_scope = ProfileScope(std::string(__PRETTY_FUNCTION__) + x);
#endif
#else
#define PROFILE_THIS     ;
#define PROFILE_SCOPE(x) ;
#endif

namespace qat {

struct Profiler;

struct ProfileScope {
	String                                         name;
	std::chrono::high_resolution_clock::time_point start;

	ProfileScope(String _name);

	~ProfileScope();
};

struct Profiler {
	static std::unordered_map<String, Pair<usize, usize>> timings;
	static std::array<String, 3>                          prefixExclusions;

	static void write_to_file(String filePath);
};

} // namespace qat

#endif
