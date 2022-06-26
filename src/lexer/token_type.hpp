#ifndef QAT_LEXER_TOKEN_TYPE_HPP
#define QAT_LEXER_TOKEN_TYPE_HPP

namespace qat {
namespace lexer {
/**
 *  Type of the token recognised by the Lexer.
 * This does not necessarily have to mean anything as
 * far as language semantics goes.
 */
enum class TokenType {
  null = -1, // null represents an empty value
  FALSE = 0, // false
  TRUE = 1,
  startOfFile,
  give,
  Public,
  stop,
  voidType,
  boolType,
  file,
  bring,
  from,
  to,
  child,
  external,
  function,
  Type,
  model,
  identifier,
  parenthesisOpen,
  parenthesisClose,
  curlybraceOpen,
  curlybraceClose,
  bracketOpen,
  bracketClose,
  colon,
  alias,
  For,
  constant,
  let,
  assignment,
  pointerType,
  separator,
  referenceType,
  integerType,
  IntegerLiteral,
  floatType,
  FloatLiteral,
  stringType,
  StringLiteral,
  say,
  as,
  self,
  variationMarker,
  lib,
  box,
  endOfFile,
  expose,
  packed,
  var,
  pointerAccess,
  obtainPointer,
  givenTypeSeparator,
  binaryOperator,
  assignedBinaryOperator,
  If,
  Else,
  lesserThan,
  greaterThan,
  heap,
  sizeOf,
  unaryOperatorLeft,
  unaryOperatorRight,
  lambda,
  comment,
  singleStatementMarker,
  bitwiseOr,
  bitwiseAnd,
  bitwiseNot,
  templateTypeStart,
  templateTypeEnd,
  Await,
  Async,
  semiColon,
  ternary,
  isNullPointer,
  isNotNullPointer,
  assignToNullPointer,
  assignToNonNullPointer,
  super
};
} // namespace lexer
} // namespace qat

#endif