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
  Vec<StringLiteral *>
      paths; // All paths specified to be brought into the scope
  utils::VisibilityInfo visibility; // Visibility of the brought paths

public:
  // BringPaths represents importing of files or folders into the
  // current compilable scope
  BringPaths(Vec<StringLiteral *>         _paths,
             const utils::VisibilityInfo &_visibility,
             utils::FileRange             _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::bringPaths; }
};

} // namespace qat::ast

#endif