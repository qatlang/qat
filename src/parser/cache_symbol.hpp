#ifndef QAT_PARSER_CACHE_SYMBOL_HPP
#define QAT_PARSER_CACHE_SYMBOL_HPP

#include "../utils/file_range.hpp"
#include "../utils/identifier.hpp"

#include <helpers/integers.hpp>
#include <helpers/string.hpp>
#include <helpers/vec.hpp>

namespace qat::parser {

/**
 * CacheSymbol is used to represent a symbol/group of identifiers that
 * represents an entity, type name or a value in the language
 *
 */
class CacheSymbol {
  public:
	CacheSymbol(Vec<Identifier> _name, usize _tokenIndex, FileRangePtr _fileRange);
	CacheSymbol(u32 _relative, Vec<Identifier> _name, usize _tokenIndex, FileRangePtr _fileRange);

	u32             relative;
	Vec<Identifier> name;
	FileRangePtr    fileRange;
	usize           tokenIndex;

	String to_string() const;

	bool         hasRelative() const;
	FileRangePtr extend_fileRange(FileRangePtr upto);
};

} // namespace qat::parser

#endif
