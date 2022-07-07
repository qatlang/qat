#ifndef QAT_AST_NODE_TYPE_HPP
#define QAT_AST_NODE_TYPE_HPP

namespace qat {
namespace AST {
enum class NodeType {
  lib,
  block,
  bringEntities,
  bringPaths,
  functionPrototype,
  functionDefinition,
  localDeclaration,
  globalDeclaration,
  integerLiteral,
  unsignedLiteral,
  customIntegerLiteral,
  floatLiteral,
  stringLiteral,
  saySentence,
  ifElse,
  giveSentence,
  exposeBoxes,
  defineCoreType,
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
  loopIndex,
  multithread,
  multithreadGive,
  atomicAssignedExpression,
  nullPointer,
  radixLiteral,
  customFloatLiteral,
  box,
  tupleValue
};
}
} // namespace qat

#endif