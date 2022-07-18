#ifndef QAT_PARSER_TOKEN_FAMILY_HPP
#define QAT_PARSER_TOKEN_FAMILY_HPP

#include "../lexer/token_type.hpp"
#include <vector>

namespace qat::parser {

class TokenFamily {
public:
  static std::vector<lexer::TokenType> templateTypeSpecifiers;
  static std::vector<lexer::TokenType> functionArgs;
  static std::vector<lexer::TokenType> scopeLimiters;
};

} // namespace qat::parser

#endif