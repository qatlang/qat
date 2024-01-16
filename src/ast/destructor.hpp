#ifndef QAT_AST_DESTRUCTOR_HPP
#define QAT_AST_DESTRUCTOR_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class DestructorDefinition : public Node {
private:
  FileRange      nameRange;
  Vec<Sentence*> sentences;

  mutable IR::MemberFunction* memberFn     = nullptr;
  mutable IR::MemberParent*   memberParent = nullptr;

public:
  DestructorDefinition(FileRange _nameRange, Vec<Sentence*> _sentences, FileRange _fileRange)
      : Node(_fileRange), nameRange(_nameRange), sentences(_sentences) {}

  useit static inline DestructorDefinition* create(FileRange _nameRange, Vec<Sentence*> _sentences,
                                                   FileRange _fileRange) {
    return std::construct_at(OwnNormal(DestructorDefinition), _nameRange, _sentences, _fileRange);
  }

  void setMemberParent(IR::MemberParent* memberParent) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
