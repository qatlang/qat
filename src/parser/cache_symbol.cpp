#include "./cache_symbol.hpp"

namespace qat::parser {

CacheSymbol::CacheSymbol(Vec<Identifier> _name, usize _tokenIndex, qat::FileRange _fileRange)
    : relative(0), name(std::move(_name)), fileRange(std::move(_fileRange)), tokenIndex(_tokenIndex) {}

CacheSymbol::CacheSymbol(u32 _relative, Vec<Identifier> _name, usize _tokenIndex, qat::FileRange _fileRange)
    : relative(_relative), name(std::move(_name)), fileRange(std::move(_fileRange)), tokenIndex(_tokenIndex) {}

bool CacheSymbol::hasRelative() const { return relative != 0; }

FileRange CacheSymbol::extend_fileRange(const FileRange& upto) {
  fileRange = qat::FileRange(fileRange, upto);
  return fileRange;
}

} // namespace qat::parser
