#ifndef QAT_LEXER_TOKEN_HPP
#define QAT_LEXER_TOKEN_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "./token_type.hpp"
#include <string>

namespace qat::lexer {

// Token consists of a symbol encountered by the Lexer
// during file analysis. All tokens are later parsed through, by
// the QAT Parser to obtain an abstract syntax tree.
class Token {
private:
  Token(TokenType _type, utils::FileRange _fileRange)
      : type(_type), value(), hasValue(false),
        fileRange(std::move(_fileRange)) {}

  Token(TokenType _type, String _value, utils::FileRange _fileRange)
      : type(_type), value(std::move(_value)), hasValue(true),
        fileRange(std::move(_fileRange)) {}

public:
  /**
   *  Tokens that represents a value. Integer literal, Double literal,
   * Identifiers are some examples of this nature of token
   *
   * @param _type Type of the token - In the case of an Identifier token,
   * this is TokenType::identifier
   * @param _value Value of the token - In the case of an identifier token,
   * this is the name of the identifier
   * @param lexer Pointer to the Lexer instance to get the line and character
   * numbers
   * @return Token
   */
  static Token valued(TokenType _type, String _value,
                      utils::FileRange fileRange);

  /**
   *  Tokens that are by default, recognised by the language. These tokens
   * represents elements that are part of the language.
   *
   * @param _type Type of the token - In the case of an Identifier token,
   * this is TokenType::identifier
   * @param lexer Pointer to the Lexer instance to get the line and character
   * numbers
   * @return Token
   */
  static Token normal(TokenType _type, utils::FileRange fileRange);

  /**
   *  Type of the token. Can mostly refer to symbols that
   * are part of the language.
   */
  TokenType type;

  /**
   *  Value of the token. This will be empty for all tokens
   * that are part of the language
   */
  String value;

  /**
   *  Whether this token represents a value. This is necessary as the
   * value can just be an empty string and cannot be considered as no value in
   * that case.
   */
  bool hasValue = false;

  /**
   *  FileRange indicates a starting and ending position that contains
   * appropriate value
   */
  utils::FileRange fileRange;
};

} // namespace qat::lexer

#endif