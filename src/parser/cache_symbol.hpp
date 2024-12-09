#ifndef QAT_PARSER_CACHE_SYMBOL_HPP
#define QAT_PARSER_CACHE_SYMBOL_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "../utils/macros.hpp"

namespace qat::parser {

/**
 * CacheSymbol is used to represent a symbol/group of identifiers that
 * represents an entity, type name or a value in the language
 *
 */
class CacheSymbol {
  public:
	CacheSymbol(Vec<Identifier> _name, usize _tokenIndex, FileRange _fileRange);
	CacheSymbol(u32 _relative, Vec<Identifier> _name, usize _tokenIndex, FileRange _fileRange);

	u32             relative;
	Vec<Identifier> name;
	FileRange       fileRange;
	usize           tokenIndex;

	useit String to_string() const;

	useit bool      hasRelative() const;
	useit FileRange extend_fileRange(const FileRange& upto);
};

} // namespace qat::parser

#endif