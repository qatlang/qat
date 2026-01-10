#ifndef QAT_LEXER_TOKEN_HPP
#define QAT_LEXER_TOKEN_HPP

#include "../utils/file_range.hpp"
#include "../utils/identifier.hpp"
#include "./token_type.hpp"

#include <helpers/string.hpp>

namespace qat::lexer {

class Token {
  private:
  public:
	Token(TokenType _type, String _value, FileRangePtr _fileRange)
	    : type(_type), value(std::move(_value)), fileRange(_fileRange) {}

	Token(TokenType _type, FileRangePtr _fileRange) : type(_type), value(), fileRange(_fileRange) {}

	TokenType    type;
	String       value;
	FileRangePtr fileRange;

	operator Identifier() const { return Identifier(value, fileRange); }
};

} // namespace qat::lexer

#endif
