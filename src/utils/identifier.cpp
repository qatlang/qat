#include "./identifier.hpp"

namespace qat {

Identifier::Identifier(String _value, FileRangePtr _fileRange)
    : value(std::move(_value)), range(std::move(_fileRange)) {}

Identifier Identifier::fullName(Vec<Identifier> ids) {
	auto         name  = ids.front().value;
	FileRangePtr range = ids.front().range;
	for (usize i = 1; i < ids.size(); i++) {
		name.append(":").append(ids.at(i).value);
		range = FileRange::merge(range, ids.at(i).range);
	}
	return {name, range};
}

} // namespace qat
