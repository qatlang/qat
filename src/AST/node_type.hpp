#ifndef QAT_AST_NODE_TYPE_HPP
#define QAT_AST_NODE_TYPE_HPP

namespace qat {
namespace AST {
enum class NodeType {
  lib,
  block,
  bringEntities,
  bringFiles,
  functionPrototype,
  functionDefinition,
  localDeclaration,
  globalDeclaration,
  integerLiteral,
  unsignedLiteral,
  customIntegerLiteral,
  floatLiteral,
  stringLiteral,
  staticDeclaration,
  saySentence,
  ifElse,
  giveSentence,
  exposeBoxes,
  defineObjectType,
  closeBox,
  reassignment,
  entity,
  selfExpression,
  toConversion,
  unaryExpression,
  ternaryExpression,
  functionCall,
  binaryExpression,
  allocateOnHeap,
  memberFunctionCall,
  memberVariableExpression,
  sizeOf,
  sizeOfType,
  memberIndexAccess,
  symbol,
  expressionSentence,
  loopTimes,
  loopWhile,
  loopOver,
  multiThread,
  multiThreadGive,
  atomicAssignedExpression,
  nullPointer,
  radixLiteral
};
}
} // namespace qat

#endif