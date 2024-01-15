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

  ConvertorPrototype(bool _isFrom, FileRange _nameRange, Maybe<Identifier> _argName, QatType* _candidateType,
                     bool _isMemberArg, Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange);

public:
  static ConvertorPrototype* From(FileRange nameRange, Maybe<Identifier> argName, QatType* _candidateType,
                                  bool _isMemberArg, Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange);

  static ConvertorPrototype* To(FileRange nameRange, QatType* _candidateType, Maybe<VisibilitySpec> visibSpec,
                                const FileRange& fileRange);

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::convertorPrototype; }
};

class ConvertorDefinition : public Node {
  friend class DefineCoreType;

private:
  Vec<Sentence*>      sentences;
  ConvertorPrototype* prototype;

  void setMemberParent(IR::MemberParent* memParent) const;

public:
  ConvertorDefinition(ConvertorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange);

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::functionDefinition; }
};

} // namespace qat::ast

#endif
