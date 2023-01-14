#ifndef QAT_PARSER_TOKEN_FAMILY_HPP
#define QAT_PARSER_TOKEN_FAMILY_HPP

#include "../lexer/token_type.hpp"
#include "../utils/helpers.hpp"

namespace qat::parser {

class TokenFamily {
public:
  static Vec<lexer::TokenType> genericTypeSpecifiers;
  static Vec<lexer::TokenType> functionArgs;
  static Vec<lexer::TokenType> scopeLimiters;
};

} // namespace qat::parser

#endif