#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include <utility>

#include "../IR/context.hpp"
#include "../IR/types/array.hpp"
#include "../IR/types/core_type.hpp"
#include "../IR/types/float.hpp"
#include "../IR/types/function.hpp"
#include "../IR/types/integer.hpp"
#include "../IR/types/pointer.hpp"
#include "../IR/types/reference.hpp"
#include "../IR/types/sum.hpp"
#include "../IR/types/tuple.hpp"
#include "../IR/types/unsigned.hpp"
#include "../backend/cpp.hpp"
#include "../show.hpp"
#include "../utils/file_range.hpp"
#include "./errors.hpp"
#include "./node_type.hpp"
#include "nuo/json.hpp"

namespace qat::ast {

//  Node is the base class for all AST members of the language, and it
// requires a FileRange instance that indicates its position in the
// corresponding file
class Node {
private:
  static Vec<Node *> allNodes;

public:
  utils::FileRange fileRange;

  // Node is the base class for all AST members of the language, and it
  // requires a FileRange instance that indicates its position in the
  // corresponding file
  //
  // `_fileRange` FileRange instance that represents the range
  // spanned by the tokens that made up this AST member
  explicit Node(utils::FileRange _fileRange);
  virtual ~Node()                                 = default;
  useit virtual IR::Value *emit(IR::Context *ctx) = 0;
  useit virtual nuo::Json  toJson() const         = 0;
  useit virtual NodeType   nodeType() const       = 0;
  static void              clearAll();
};

} // namespace qat::ast

#endif