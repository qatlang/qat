#include "./cache_symbol.hpp"

namespace qat::parser {

CacheSymbol::CacheSymbol(std::string _name, std::size_t _tokenIndex,
                         qat::utils::FileRange _fileRange)
    : name(_name), tokenIndex(_tokenIndex), fileRange(_fileRange) {}

utils::FileRange CacheSymbol::extend_fileRange(qat::utils::FileRange to) {
  fileRange = qat::utils::FileRange(fileRange, to);
  return fileRange;
}

} // namespace qat::parser
