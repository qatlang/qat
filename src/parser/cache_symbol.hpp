#ifndef QAT_PARSER_CACHE_SYMBOL_HPP
#define QAT_PARSER_CACHE_SYMBOL_HPP

#include "../utils/file_range.hpp"
#include <string>

namespace qat::parser {

/**
 *  CacheSymbol is used to represent a symbol/group of identifiers that
 * represents an entity, type name or a value in the language
 *
 */
class CacheSymbol {
public:
  CacheSymbol(std::string _name, std::size_t _tokenIndex,
              utils::FileRange _fileRange);

  std::string name;
  utils::FileRange fileRange;
  std::size_t tokenIndex;

  utils::FileRange extend_fileRange(utils::FileRange to);
};

} // namespace qat::parser

#endif