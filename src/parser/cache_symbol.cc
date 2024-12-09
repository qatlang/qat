#include "./cache_symbol.hpp"

namespace qat::parser {

CacheSymbol::CacheSymbol(Vec<Identifier> _name, usize _tokenIndex, qat::FileRange _fileRange)
    : relative(0), name(std::move(_name)), fileRange(std::move(_fileRange)), tokenIndex(_tokenIndex) {}

CacheSymbol::CacheSymbol(u32 _relative, Vec<Identifier> _name, usize _tokenIndex, qat::FileRange _fileRange)
    : relative(_relative), name(std::move(_name)), fileRange(std::move(_fileRange)), tokenIndex(_tokenIndex) {}

String CacheSymbol::to_string() const {
	String result;
	for (usize i = 0; i < relative; i++) {
		result += "up:";
	}
	for (usize i = 0; i < name.size(); i++) {
		result += name.at(i).value;
		if (i != (name.size() - 1)) {
			result += ":";
		}
	}
	return result;
}

bool CacheSymbol::hasRelative() const { return relative != 0; }

FileRange CacheSymbol::extend_fileRange(const FileRange& upto) {
	fileRange = qat::FileRange(fileRange, upto);
	return fileRange;
}

} // namespace qat::parser
