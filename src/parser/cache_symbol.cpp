#include "./cache_symbol.hpp"

namespace qat::parser {

CacheSymbol::CacheSymbol(String _name, usize _tokenIndex,
                         qat::utils::FileRange _fileRange)
    : relative(0), name(std::move(_name)), fileRange(std::move(_fileRange)),
      tokenIndex(_tokenIndex) {}

CacheSymbol::CacheSymbol(u32 _relative, String _name, usize _tokenIndex,
                         qat::utils::FileRange _fileRange)
    : relative(_relative), name(std::move(_name)),
      fileRange(std::move(_fileRange)), tokenIndex(_tokenIndex) {}

bool CacheSymbol::hasRelative() const { return relative != 0; }

utils::FileRange CacheSymbol::extend_fileRange(qat::utils::FileRange upto) {
  fileRange = qat::utils::FileRange(fileRange, std::move(upto));
  return fileRange;
}

} // namespace qat::parser
