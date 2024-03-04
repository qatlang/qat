#include "./token.hpp"

namespace qat::lexer {

Token Token::valued(TokenType _type, String _value,
                    FileRange _fileRange //
) {
  return Token(_type, std::move(_value), std::move(_fileRange));
}

Token Token::normal(TokenType _type, FileRange _fileRange) { return Token(_type, _fileRange); }

Token::operator Identifier() const { return Identifier(value, fileRange); }

} // namespace qat::lexer