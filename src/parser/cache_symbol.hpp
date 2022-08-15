#ifndef QAT_PARSER_CACHE_SYMBOL_HPP
#define QAT_PARSER_CACHE_SYMBOL_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"

namespace qat::parser {

/**
 * CacheSymbol is used to represent a symbol/group of identifiers that
 * represents an entity, type name or a value in the language
 *
 */
class CacheSymbol {
public:
  CacheSymbol(String _name, usize _tokenIndex, utils::FileRange _fileRange);
  CacheSymbol(u32 _relative, String _name, usize _tokenIndex,
              utils::FileRange _fileRange);

  u32              relative;
  String           name;
  utils::FileRange fileRange;
  usize            tokenIndex;

  useit bool hasRelative() const;
  useit utils::FileRange extend_fileRange(utils::FileRange upto);
};

} // namespace qat::parser

#endif