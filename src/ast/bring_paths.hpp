#ifndef QAT_AST_BRING_PATHS_HPP
#define QAT_AST_BRING_PATHS_HPP

#include "../utils/visibility.hpp"
#include "./expressions/string_literal.hpp"
#include "./sentence.hpp"
#include <filesystem>

namespace qat::ast {

// BringPaths represents importing of files or folders into the current
// compilable scope
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
   * @param _fileRange
   */
  BringPaths(std::vector<StringLiteral> _paths,
             utils::VisibilityInfo _visibility, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::bringPaths; }
};

} // namespace qat::ast

#endif