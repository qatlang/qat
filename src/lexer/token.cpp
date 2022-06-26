#include "./token.hpp"

namespace qat {
namespace lexer {

Token Token::valued(TokenType _type, std::string _value,
                    utils::FilePlacement _filePlacement //
) {
  return Token(_type, _value, _filePlacement);
}

Token Token::normal(TokenType _type, utils::FilePlacement _filePlacement) {
  return Token(_type, _filePlacement);
}

} // namespace lexer
} // namespace qat