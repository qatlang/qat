#include "./cache_symbol.hpp"

namespace qat::parser {

CacheSymbol::CacheSymbol(std::string _name, std::size_t _tokenIndex,
                         qat::utils::FileRange _filePlacement)
    : name(_name), tokenIndex(_tokenIndex), filePlacement(_filePlacement) {}

utils::FileRange CacheSymbol::extend_placement(qat::utils::FileRange to) {
  filePlacement = qat::utils::FileRange(filePlacement, to);
  return filePlacement;
}

} // namespace qat::parser
