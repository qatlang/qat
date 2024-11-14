#ifndef QAT_AST_CONVERTOR_HPP
#define QAT_AST_CONVERTOR_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "member_parent_like.hpp"
#include "meta_info.hpp"
#include "sentence.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class ConvertorPrototype {
  friend class ConvertorDefinition;

private:
  Maybe<Identifier>     argName;
  Type*                 candidateType;
  bool                  is_member_argumentument;
  Maybe<VisibilitySpec> visibSpec;
  bool                  isFrom;
  FileRange             nameRange;
  Maybe<FileRange>      definitionRange;
  FileRange             fileRange;

  PrerunExpression* defineChecker;
  Maybe<MetaInfo>   metaInfo;

public:
  ConvertorPrototype(bool _isFrom, FileRange _nameRange, Maybe<Identifier> _argName, Type* _candidateType,
                     bool _is_member_argument, Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange,
                     PrerunExpression* _defineCondition, Maybe<MetaInfo> _metaInfo)
      : argName(std::move(_argName)), candidateType(_candidateType), is_member_argumentument(_is_member_argument),
        visibSpec(_visibSpec), isFrom(_isFrom), nameRange(std::move(_nameRange)), fileRange(_fileRange),
        defineChecker(_defineCondition), metaInfo(std::move(_metaInfo)) {}

  static ConvertorPrototype* create_from(FileRange _nameRange, Maybe<Identifier> _argName, Type* _candidateType,
                                         bool _is_member_argument, Maybe<VisibilitySpec> _visibSpec,
                                         const FileRange& _fileRange, PrerunExpression* _defineCondition,
                                         Maybe<MetaInfo> _metaInfo) {
    return std::construct_at(OwnNormal(ConvertorPrototype), true, _nameRange, _argName, _candidateType,
                             _is_member_argument, _visibSpec, _fileRange, _defineCondition, std::move(_metaInfo));
  }

  static ConvertorPrototype* create_to(FileRange _nameRange, Type* _candidateType, Maybe<VisibilitySpec> _visibSpec,
                                       const FileRange& _fileRange, PrerunExpression* _defineCondition,
                                       Maybe<MetaInfo> _metaInfo) {
    return std::construct_at(OwnNormal(ConvertorPrototype), false, _nameRange, None, _candidateType, false, _visibSpec,
                             _fileRange, _defineCondition, std::move(_metaInfo));
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
    if (candidateType) {
      UPDATE_DEPS(candidateType);
    }
    if (defineChecker) {
      UPDATE_DEPS(defineChecker);
    }
  }

  void           define(MethodState& state, ir::Ctx* irCtx);
  useit Json     to_json() const;
  useit NodeType nodeType() const { return NodeType::CONVERTOR_PROTOTYPE; }
};

class ConvertorDefinition {
  friend class DefineCoreType;
  friend class DoSkill;

private:
  Vec<Sentence*>      sentences;
  ConvertorPrototype* prototype;
  FileRange           fileRange;

public:
  ConvertorDefinition(ConvertorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {
    prototype->definitionRange = fileRange;
  }

  useit static inline ConvertorDefinition* create(ConvertorPrototype* _prototype, Vec<Sentence*> _sentences,
                                                  FileRange _fileRange) {
    return std::construct_at(OwnNormal(ConvertorDefinition), _prototype, _sentences, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  void  define(MethodState& state, ir::Ctx* irCtx);
  useit ir::Value* emit(MethodState& state, ir::Ctx* irCtx);
  useit Json       to_json() const;
  useit NodeType   nodeType() const { return NodeType::FUNCTION_DEFINITION; }
};

} // namespace qat::ast

#endif
