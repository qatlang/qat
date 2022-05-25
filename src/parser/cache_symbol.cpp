#include "./cache_symbol.hpp"

qat::parser::CacheSymbol::CacheSymbol(std::string _name,
                                      std::size_t _tokenIndex,
                                      qat::utils::FilePlacement _filePlacement)
    : name(_name), tokenIndex(_tokenIndex), filePlacement(_filePlacement) {}

qat::utils::FilePlacement
qat::parser::CacheSymbol::extend_placement(qat::utils::FilePlacement to) {
  filePlacement = qat::utils::FilePlacement(filePlacement, to);
  return filePlacement;
}