#ifndef QAT_UTILS_IDENTIFIER_HPP
#define QAT_UTILS_IDENTIFIER_HPP

#include "./helpers.hpp"
#include "file_range.hpp"

namespace qat {

class Identifier {
  public:
	Identifier(String value, FileRangePtr range);

	String       value;
	FileRangePtr range;

	operator JsonValue() const;

	useit static Identifier named(String value) { return Identifier{std::move(value), FileRange::null}; }

	static Identifier fullName(Vec<Identifier> ids);
};

} // namespace qat

#endif
