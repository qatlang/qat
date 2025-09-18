#ifndef QAT_LEXER_TOKEN_HPP
#define QAT_LEXER_TOKEN_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "./token_type.hpp"

namespace qat::lexer {

class Token {
  private:
	Token(TokenType _type, FileRangePtr _fileRange) : type(_type), value(), hasValue(false), fileRange(_fileRange) {}

	Token(TokenType _type, String _value, FileRangePtr _fileRange)
	    : type(_type), value(std::move(_value)), hasValue(true), fileRange(_fileRange) {}

  public:
	static Token valued(TokenType _type, String _value, FileRangePtr fileRange);
	static Token normal(TokenType _type, FileRangePtr fileRange);

	TokenType    type;
	String       value;
	bool         hasValue = false;
	FileRangePtr fileRange;

	operator Identifier() const;
};

} // namespace qat::lexer

#endif
