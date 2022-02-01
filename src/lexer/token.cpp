/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENSE - 1.0
 *
 * This project is licensed under the AAF Inspectable License 1.0.
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
 * Inspectable License 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicense/
 *
 * This project may contain parts that are not licensed under the
 * same license. If so, the licenses of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same license in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, it's
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

#include "./token.hpp"

qat::lexer::Token
qat::lexer::Token::valued(qat::lexer::TokenType _type, std::string _value,
                          qat::utilities::FilePosition _filePosition //
) {
  return Token(_type, _value, _filePosition);
}

qat::lexer::Token
qat::lexer::Token::normal(qat::lexer::TokenType _type,
                          qat::utilities::FilePosition _filePosition) {
  return Token(_type, _filePosition);
}