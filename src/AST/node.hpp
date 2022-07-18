#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include <utility>

#include "../IR/context.hpp"
#include "../backend/cpp.hpp"
#include "../utils/file_placement.hpp"
#include "./errors.hpp"
#include "./node_type.hpp"
#include "nuo/json.hpp"

namespace qat::AST {

//  Node is the base class for all AST members of the language, and it
// requires a FilePlacement instance that indicates its position in the
// corresponding file
class Node {
public:
  // Node is the base class for all AST members of the language, and it
  // requires a FilePlacement instance that indicates its position in the
  // corresponding file
  //
  // `_filePlacement` FilePlacement instance that represents the range
  // spanned by the tokens that made up this AST member
  explicit Node(utils::FilePlacement _filePlacement)
      : file_placement(std::move(_filePlacement)) {}

  virtual ~Node() = default;

  // This is the code emitter function that handles Qat IR
  virtual IR::Value *emit(IR::Context *ctx){};

  // This is the emitter function that handles the generation of JSON
  virtual nuo::Json toJson() const {};

  // This is the emitter function for C++
  virtual void emitCPP(backend::cpp::File &file, bool isHeader) const {}

  // Type of the node represented by this AST member
  virtual NodeType nodeType() const {};

  /**
   *  A range present in a file that represents the placement spanned by
   * the tokens that made up this AST member
   *
   */
  utils::FilePlacement file_placement;

  // Destroy all owned objects in the heap
  virtual void destroy(){};
};

} // namespace qat::AST

#endif