#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include <utility>

#include "../IR/context.hpp"
#include "../backend/cpp.hpp"
#include "../utils/file_range.hpp"
#include "./errors.hpp"
#include "./node_type.hpp"
#include "nuo/json.hpp"

namespace qat::ast {

//  Node is the base class for all AST members of the language, and it
// requires a FileRange instance that indicates its position in the
// corresponding file
class Node {
public:
  // Node is the base class for all AST members of the language, and it
  // requires a FileRange instance that indicates its position in the
  // corresponding file
  //
  // `_fileRange` FileRange instance that represents the range
  // spanned by the tokens that made up this AST member
  explicit Node(utils::FileRange _fileRange)
      : fileRange(std::move(_fileRange)) {}

  virtual ~Node() = default;

  // This is the code emitter function that handles Qat IR
  useit virtual IR::Value *emit(IR::Context *ctx) = 0;

  // This is the emitter function that handles the generation of JSON
  useit virtual nuo::Json toJson() const = 0;

  // Type of the node represented by this AST member
  useit virtual NodeType nodeType() const = 0;

  /**
   *  A range present in a file that represents the fileRange spanned by
   * the tokens that made up this AST member
   *
   */
  utils::FileRange fileRange;

  // Destroy all owned objects in the heap
  virtual void destroy(){};
};

} // namespace qat::ast

#endif