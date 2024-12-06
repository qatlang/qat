#ifndef QAT_AST_DESTRUCTOR_HPP
#define QAT_AST_DESTRUCTOR_HPP

#include "../IR/context.hpp"
#include "../IR/types/struct_type.hpp"
#include "./argument.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "member_parent_like.hpp"
#include "meta_info.hpp"
#include <string>

namespace qat::ast {

class DestructorDefinition {
  friend DefineCoreType;
  friend DoSkill;

  FileRange      nameRange;
  Vec<Sentence*> sentences;
  FileRange      fileRange;

  PrerunExpression* defineChecker;
  Maybe<MetaInfo>   metaInfo;

public:
  DestructorDefinition(FileRange _nameRange, PrerunExpression* _defineChecker, Maybe<MetaInfo> _metaInfo,
                       Vec<Sentence*> _sentences, FileRange _fileRange)
      : nameRange(_nameRange), sentences(_sentences), fileRange(_fileRange), defineChecker(_defineChecker),
        metaInfo(std::move(_metaInfo)) {}

  useit static DestructorDefinition* create(FileRange nameRange, PrerunExpression* _defineChecker,
                                            Maybe<MetaInfo> metaInfo, Vec<Sentence*> _sentences, FileRange fileRange) {
    return std::construct_at(OwnNormal(DestructorDefinition), nameRange, _defineChecker, std::move(metaInfo),
                             std::move(_sentences), fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
    if (defineChecker) {
      UPDATE_DEPS(defineChecker);
    }
  }

  void  define(MethodState& state, ir::Ctx* irCtx);
  useit ir::Value* emit(MethodState& state, ir::Ctx* irCtx);
  useit Json       to_json() const;
  useit NodeType   nodeType() const { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
