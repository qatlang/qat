#ifndef QAT_AST_CONVERTOR_HPP
#define QAT_AST_CONVERTOR_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "sentence.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class ConvertorPrototype : public Node {
  friend class ConvertorDefinition;
  friend class DefineCoreType;

private:
  Maybe<Identifier>     argName;
  QatType*              candidateType;
  bool                  isMemberArgument;
  Maybe<VisibilitySpec> visibSpec;
  bool                  isFrom;
  FileRange             nameRange;
  Maybe<FileRange>      definitionRange;

  mutable IR::MemberParent*   memberParent = nullptr;
  mutable IR::MemberFunction* memberFn     = nullptr;

  void setMemberParent(IR::MemberParent* _memberParent) const;

public:
  ConvertorPrototype(bool _isFrom, FileRange _nameRange, Maybe<Identifier> _argName, QatType* _candidateType,
                     bool _isMemberArg, Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange)
      : Node(_fileRange), argName(std::move(_argName)), candidateType(_candidateType), isMemberArgument(_isMemberArg),
        visibSpec(_visibSpec), isFrom(_isFrom), nameRange(std::move(_nameRange)) {}

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

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::CONVERTOR_PROTOTYPE; }
};

class ConvertorDefinition : public Node {
  friend class DefineCoreType;

private:
  Vec<Sentence*>      sentences;
  ConvertorPrototype* prototype;

public:
  ConvertorDefinition(ConvertorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : Node(_fileRange), sentences(_sentences), prototype(_prototype) {
    prototype->definitionRange = fileRange;
  }

  useit static inline ConvertorDefinition* create(ConvertorPrototype* _prototype, Vec<Sentence*> _sentences,
                                                  FileRange _fileRange) {
    return std::construct_at(OwnNormal(ConvertorDefinition), _prototype, _sentences, _fileRange);
  }

  void  setMemberParent(IR::MemberParent* memParent) const;
  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::FUNCTION_DEFINITION; }
};

} // namespace qat::ast

#endif
