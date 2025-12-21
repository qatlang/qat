#include "./token.hpp"

namespace qat::lexer {

Token Token::valued(TokenType _type, String _value,
                    FileRangePtr _fileRange //
) {
	return Token(_type, std::move(_value), std::move(_fileRange));
}

Token Token::normal(TokenType type, FileRangePtr fileRange) { return Token(type, fileRange); }

Token::operator Identifier() const { return Identifier(value, fileRange); }

} // namespace qat::lexer
