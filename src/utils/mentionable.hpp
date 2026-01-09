#ifndef QAT_UTILS_MENTIONABLE_HPP
#define QAT_UTILS_MENTIONABLE_HPP

#include "./file_range.hpp"
#include "./helpers.hpp"

namespace qat {

namespace ir {
class Mod;
}

struct Mentionable {
	Vec<FileRangePtr>                 mentions;
	Vec<Pair<ir::Mod*, FileRangePtr>> importedMentions;

	void add_mention(FileRangePtr range) { mentions.push_back(range); }

	void add_import_mention(ir::Mod* mod, FileRangePtr range) {
		importedMentions.push_back(std::make_pair(mod, range));
	}
};
} // namespace qat

#endif
