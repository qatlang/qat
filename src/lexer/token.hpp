/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#ifndef QAT_LEXER_TOKEN_HPP
#define QAT_LEXER_TOKEN_HPP

#include "../utilities/file_placement.hpp"
#include "./token_type.hpp"
#include <string>

namespace qat {
namespace lexer {

/**
 * @brief Token constitutes of a symbol encountered by the Lexer
 * during file analysis. All tokens are later parsed through, by
 * the QAT Parser to obtain an abstract syntax tree.
 */
class Token {
private:
  explicit Token(TokenType _type, utilities::FilePlacement _filePlacement)
      : type(_type), value(""), hasValue(false), filePlacement(_filePlacement) {
  }

  explicit Token(TokenType _type, std::string _value,
                 utilities::FilePlacement _filePlacement)
      : type(_type), value(_value), hasValue(true),
        filePlacement(_filePlacement) {}

public:
  /**
   * @brief Tokens that represents a value. Integer literal, Double literal,
   * Identifiers are some examples of this kind of token
   *
   * @param _type Type of the token - In the case of an Identifier token,
   * this is TokenType::identifier
   * @param _value Value of the token - In the case of an identifier token,
   * this is the name of the identifier
   * @param lexer Pointer to the Lexer instance to get the line and character
   * numbers
   * @return Token
   */
  static Token valued(TokenType _type, std::string _value,
                      utilities::FilePlacement filePlacement);

  /**
   * @brief Tokens that are by default, recognised by the language. These tokens
   * represents elements that are part of the language.
   *
   * @param _type Type of the token - In the case of an Identifier token,
   * this is TokenType::identifier
   * @param lexer Pointer to the Lexer instance to get the line and character
   * numbers
   * @return Token
   */
  static Token normal(TokenType _type, utilities::FilePlacement filePlacement);

  /**
   * @brief Type of the token. Can mostly refer to symbols that
   * are part of the language.
   */
  TokenType type;

  /**
   * @brief Value of the token. This will be empty for all tokens
   * that are part of the language
   */
  std::string value;

  /**
   * @brief Whether this token represents a value. This is necessary as the
   * value can just be an empty string and cannot be considerd as no value in
   * that case.
   */
  bool hasValue = false;

  /**
   * @brief FilePlacement indicates a starting and ending position that contains
   * appropriate value
   */
  utilities::FilePlacement filePlacement;
};
} // namespace lexer
} // namespace qat

#endif