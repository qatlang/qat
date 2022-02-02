#ifndef QAT_AST_NODETYPE_HPP
#define QAT_AST_NODETYPE_HPP

namespace qat {
namespace AST {
enum class NodeType {
  functionPrototype,
  functionDefinition,
  localDeclaration,
  integerLiteral,
  floatLiteral,
  stringLiteral,
  staticDeclaration,
  saySentence,
  ifElseSentence,
  giveSentence,
  exposeSpace,
  defineObjectType,
  closeSpace,
  reassignment,
  variableExpression,
  thisExpression,
  toConversion,
  unaryExpression,
  ternaryExpression,
  functionCall,
  binaryExpression,
  allocateOnHeap,
  memberFunctionCall,
  memberVariableExpression,
  sizeOf,
};
}
} // namespace qat

#endif