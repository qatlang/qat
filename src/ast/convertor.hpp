#ifndef QAT_AST_CONVERTOR_HPP
#define QAT_AST_CONVERTOR_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "member_parent_like.hpp"
#include "sentence.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class ConvertorPrototype {
  friend class ConvertorDefinition;

private:
  Maybe<Identifier>     argName;
  QatType*              candidateType;
  bool                  isMemberArgument;
  Maybe<VisibilitySpec> visibSpec;
  bool                  isFrom;
  FileRange             nameRange;
  Maybe<FileRange>      definitionRange;
  FileRange             fileRange;

public:
  ConvertorPrototype(bool _isFrom, FileRange _nameRange, Maybe<Identifier> _argName, QatType* _candidateType,
                     bool _isMemberArg, Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange)
      : argName(std::move(_argName)), candidateType(_candidateType), isMemberArgument(_isMemberArg),
        visibSpec(_visibSpec), isFrom(_isFrom), nameRange(std::move(_nameRange)), fileRange(_fileRange) {}

  static ConvertorPrototype* create_from(FileRange _nameRange, Maybe<Identifier> _argName, QatType* _candidateType,
                                         bool _isMemberArg, Maybe<VisibilitySpec> _visibSpec,
                                         const FileRange& _fileRange) {
    return std::construct_at(OwnNormal(ConvertorPrototype), true, _nameRange, _argName, _candidateType, _isMemberArg,
                             _visibSpec, _fileRange);
  }

  static ConvertorPrototype* create_to(FileRange _nameRange, QatType* _candidateType, Maybe<VisibilitySpec> _visibSpec,
                                       const FileRange& _fileRange) {
    return std::construct_at(OwnNormal(ConvertorPrototype), false, _nameRange, None, _candidateType, false, _visibSpec,
                             _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent, IR::Context* ctx) {
    if (candidateType) {
      UPDATE_DEPS(candidateType);
    }
  }

  void           define(MethodState& state, IR::Context* ctx);
  useit Json     toJson() const;
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

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent, IR::Context* ctx) {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  void  define(MethodState& state, IR::Context* ctx);
  useit IR::Value* emit(MethodState& state, IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::FUNCTION_DEFINITION; }
};

} // namespace qat::ast

#endif
