#include "./token.hpp"

namespace qat::lexer {

Token Token::valued(TokenType _type, std::string _value,
                    utils::FileRange _filePlacement //
) {
  return Token(_type, _value, _filePlacement);
}

Token Token::normal(TokenType _type, utils::FileRange _filePlacement) {
  return Token(_type, _filePlacement);
}

} // namespace qat::lexer