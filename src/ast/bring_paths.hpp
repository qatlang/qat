#ifndef QAT_AST_BRING_PATHS_HPP
#define QAT_AST_BRING_PATHS_HPP

#include "../utils/visibility.hpp"
#include "./constants/string_literal.hpp"
#include "./sentence.hpp"
#include <filesystem>

namespace qat::ast {

class BringPaths : public Sentence {
private:
  bool                         isMember;
  Vec<StringLiteral*>          paths;
  Maybe<utils::VisibilityKind> visibility;
  Vec<Maybe<StringLiteral*>>   names;

public:
  BringPaths(bool _isMember, Vec<StringLiteral*> _paths, Vec<Maybe<StringLiteral*>> _names,
             Maybe<utils::VisibilityKind> _visibility, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  void             handleBrings(IR::Context* ctx) const override;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::bringPaths; }
};

} // namespace qat::ast

#endif