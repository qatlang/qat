#ifndef QAT_UTILS_PROFILER_HPP
#define QAT_UTILS_PROFILER_HPP

#include "./macros.hpp"
#include <helpers/array.hpp>
#include <helpers/hashmap.hpp>
#include <helpers/integers.hpp>
#include <helpers/pair.hpp>
#include <helpers/string.hpp>
#include <helpers/time.hpp>

#if ENABLE_PROFILER
#if RUNTIME_IS_MSVC
#define PROFILE_THIS     auto _profiler_scope = ProfileScope(__FUNCSIG__);
#define PROFILE_SCOPE(x) auto _profiler_local_scope = ProfileScope(String(__FUNCSIG__) + x);
#else
#define PROFILE_THIS     auto _profiler_scope = ProfileScope(__PRETTY_FUNCTION__);
#define PROFILE_SCOPE(x) auto _profiler_local_scope = ProfileScope(String(__PRETTY_FUNCTION__) + x);
#endif
#else
#define PROFILE_THIS     ;
#define PROFILE_SCOPE(x) ;
#endif

namespace qat {

struct Profiler;

struct ProfileScope {
	String    name;
	TimePoint start;

	ProfileScope(String _name);

	~ProfileScope();
};

struct Profiler {
	static HashMap<String, Pair<usize, usize>> timings;
	static Array<String, 3>                    prefixExclusions;

	static void write_to_file(String filePath);
};

} // namespace qat

#endif
