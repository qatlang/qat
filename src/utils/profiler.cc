#include "./profiler.hpp"

#include <chrono>
#include <experimental/simd>
#include <fstream>

namespace qat {

ProfileScope::ProfileScope(String _name) : name(_name), start(std::chrono::high_resolution_clock::now()) {}

ProfileScope::~ProfileScope() {
	auto diff =
	    (usize)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start)
	        .count();
	if (Profiler::timings.contains(name)) {
		Profiler::timings[name].first += diff;
		Profiler::timings[name].second += 1u;
	} else {
		Profiler::timings[name] = Pair<usize, usize>(diff, 1u);
	}
}

std::unordered_map<String, Pair<usize, usize>> Profiler::timings          = {};
std::array<String, 3>                          Profiler::prefixExclusions = {"qat::lexer::", "qat::parser::", "qat::"};

void Profiler::write_to_file(String filePath) {
#if ENABLE_PROFILER
	std::ofstream out;
	out.open(filePath.c_str(), std::ofstream::ios_base::out | std::ofstream::ios_base::trunc);
	for (auto& it : timings) {
		auto name   = it.first;
		auto parPos = name.find_first_of('(');
		if (parPos != String::npos) {
			auto beginInd = parPos;
			while (true) {
				if (beginInd == 0) {
					break;
				} else if (name[beginInd] == ' ' || name[beginInd] == '*') {
					beginInd++;
					break;
				} else {
					beginInd--;
				}
			}
			name = name.substr(beginInd, parPos - beginInd);
		}
		for (auto& exc : prefixExclusions) {
			if (name.starts_with(exc)) {
				name = name.substr(exc.size());
				break;
			}
		}
		out << name << " = " << (((double)it.second.first) / ((double)it.second.second)) << " | " << it.second.second
		    << '\n';
	}
	out.close();
#endif
}

} // namespace qat
