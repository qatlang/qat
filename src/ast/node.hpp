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

  virtual ~Node() { destroy(); };

  // This is the code emitter function that handles Qat IR
  virtual IR::Value *emit(IR::Context *ctx){};

  // This is the emitter function that handles the generation of JSON
  virtual nuo::Json toJson() const {};

  // This is the emitter function for C++
  virtual void emitCPP(backend::cpp::File &file, bool isHeader) const {}

  // Type of the node represented by this AST member
  virtual NodeType nodeType() const {};

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