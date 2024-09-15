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

  FileRange         nameRange;
  PrerunExpression* defineChecker;
  Vec<Sentence*>    sentences;
  FileRange         fileRange;

  mutable Maybe<bool> checkResult;

public:
  DestructorDefinition(FileRange _nameRange, PrerunExpression* _defineChecker, Vec<Sentence*> _sentences,
                       FileRange _fileRange)
      : nameRange(_nameRange), defineChecker(_defineChecker), sentences(_sentences), fileRange(_fileRange) {}

  useit static inline DestructorDefinition* create(FileRange _nameRange, PrerunExpression* _defineChecker,
                                                   Vec<Sentence*> _sentences, FileRange _fileRange) {
    return std::construct_at(OwnNormal(DestructorDefinition), _nameRange, _defineChecker, _sentences, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  void  define(MethodState& state, ir::Ctx* irCtx);
  useit ir::Value* emit(MethodState& state, ir::Ctx* irCtx);
  useit Json       to_json() const;
  useit NodeType   nodeType() const { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
