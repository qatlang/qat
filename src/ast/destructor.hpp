#ifndef QAT_AST_DESTRUCTOR_HPP
#define QAT_AST_DESTRUCTOR_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "member_parent_like.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class DestructorDefinition {
  friend DefineCoreType;
  friend DoSkill;

  FileRange      nameRange;
  Vec<Sentence*> sentences;
  FileRange      fileRange;

public:
  DestructorDefinition(FileRange _nameRange, Vec<Sentence*> _sentences, FileRange _fileRange)
      : nameRange(_nameRange), sentences(_sentences), fileRange(_fileRange) {}

  useit static inline DestructorDefinition* create(FileRange _nameRange, Vec<Sentence*> _sentences,
                                                   FileRange _fileRange) {
    return std::construct_at(OwnNormal(DestructorDefinition), _nameRange, _sentences, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent, IR::Context* ctx) {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  void  define(MethodState& state, IR::Context* ctx);
  useit IR::Value* emit(MethodState& state, IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
