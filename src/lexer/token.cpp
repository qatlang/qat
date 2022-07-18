#include "./token.hpp"

namespace qat::lexer {

Token Token::valued(TokenType _type, std::string _value,
                    utils::FileRange _fileRange //
) {
  return Token(_type, _value, _fileRange);
}

Token Token::normal(TokenType _type, utils::FileRange _fileRange) {
  return Token(_type, _fileRange);
}

} // namespace qat::lexer