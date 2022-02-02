#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include "../IR/generator.hpp"
#include "../utilities/file_position.hpp"
#include "node_type.hpp"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {
class Node {
public:
  Node(utilities::FilePosition _filePosition) : filePosition(_filePosition) {}

  virtual ~Node(){};

  virtual llvm::Value *generate(IR::Generator *generator){};

  virtual NodeType nodeType(){};

  utilities::FilePosition filePosition;
};
} // namespace AST
} // namespace qat

#endif