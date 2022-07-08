#ifndef QAT_AST_BRING_PATHS_HPP
#define QAT_AST_BRING_PATHS_HPP

#include "../utils/visibility.hpp"
#include "./expressions/string_literal.hpp"
#include "./sentence.hpp"
#include <filesystem>

namespace qat {
namespace AST {

/**
 *  BringPaths represents importing of files or folders into the current
 * compilable scope
 *
 */
class BringPaths : public Sentence {
private:
  /**
   *  All paths specified to be brought into the scope
   *
   */
  std::vector<StringLiteral> paths;

  utils::VisibilityInfo visibility;

public:
  /**
   *  BringPaths represents importing of files or folders into the
   * current compilable scope
   *
   * @param _paths
   * @param _placement
   */
  BringPaths(std::vector<StringLiteral> _paths,
             utils::VisibilityInfo _visibility,
             utils::FilePlacement _placement);

  llvm::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::bringPaths; }
};

} // namespace AST
} // namespace qat

#endif