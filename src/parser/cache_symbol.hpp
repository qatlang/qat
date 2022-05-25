#ifndef QAT_PARSER_CACHE_SYMBOL_HPP
#define QAT_PARSER_CACHE_SYMBOL_HPP

#include "../utils/file_placement.hpp"
#include <string>

namespace qat {
namespace parser {
/**
 * @brief CacheSymbol is used to represent a symbol/group of identifiers that
 * represents an entity, type name or a value in the language
 *
 */
class CacheSymbol {
public:
  CacheSymbol(std::string _name, std::size_t _tokenIndex,
              utils::FilePlacement _filePlacement);

  std::string name;
  utils::FilePlacement filePlacement;
  std::size_t tokenIndex;

  utils::FilePlacement extend_placement(utils::FilePlacement to);
};
} // namespace parser
} // namespace qat

#endif