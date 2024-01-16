#ifndef QAT_AST_BRING_PATHS_HPP
#define QAT_AST_BRING_PATHS_HPP

#include "./prerun/string_literal.hpp"
#include "./sentence.hpp"

namespace qat::ast {

class BringPaths : public Sentence {
  bool                       isMember;
  Vec<StringLiteral*>        paths;
  Maybe<VisibilitySpec>      visibSpec;
  Vec<Maybe<StringLiteral*>> names;

public:
  BringPaths(bool _isMember, Vec<StringLiteral*> _paths, Vec<Maybe<StringLiteral*>> _names,
             Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
      : Sentence(_fileRange), isMember(_isMember), paths(_paths), visibSpec(_visibSpec), names(_names) {}

  useit static inline BringPaths* create(bool _isMember, Vec<StringLiteral*> _paths, Vec<Maybe<StringLiteral*>> _names,
                                         Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(BringPaths), _isMember, _paths, _names, _visibSpec, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  void             handleFilesystemBrings(IR::Context* ctx) const override;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::BRING_PATHS; }
};

} // namespace qat::ast

#endif