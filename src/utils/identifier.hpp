#ifndef QAT_UTILS_IDENTIFIER_HPP
#define QAT_UTILS_IDENTIFIER_HPP

#include "./helpers.hpp"
#include "file_range.hpp"

namespace qat {

class Identifier {
  public:
	Identifier(String value, FileRange range);

	String	  value;
	FileRange range;

	operator JsonValue() const;

	static Identifier fullName(Vec<Identifier> ids);
};

} // namespace qat

#endif