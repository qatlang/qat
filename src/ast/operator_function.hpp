#ifndef QAT_AST_OPERATOR_FUNCTION_HPP
#define QAT_AST_OPERATOR_FUNCTION_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./expressions/operator.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "member_parent_like.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class OperatorPrototype {
  friend class OperatorDefinition;

private:
  bool                  isVariationFn;
  Op                    opr;
  Vec<Argument*>        arguments;
  QatType*              returnType;
  Maybe<VisibilitySpec> visibSpec;
  Maybe<Identifier>     argName;
  FileRange             nameRange;
  FileRange             fileRange;

public:
  OperatorPrototype(bool _isVariationFn, Op _op, FileRange _nameRange, Vec<Argument*> _arguments, QatType* _returnType,
                    Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange, Maybe<Identifier> _argName)
      : isVariationFn(_isVariationFn), opr(_op), arguments(_arguments), returnType(_returnType), visibSpec(_visibSpec),
        argName(_argName), nameRange(_nameRange), fileRange(_fileRange) {}

  useit static inline OperatorPrototype* create(bool _isVariationFn, Op _op, FileRange _nameRange,
                                                Vec<Argument*> _arguments, QatType* _returnType,
                                                Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange,
                                                Maybe<Identifier> _argName) {
    return std::construct_at(OwnNormal(OperatorPrototype), _isVariationFn, _op, _nameRange, _arguments, _returnType,
                             _visibSpec, _fileRange, _argName);
  }

  void           define(MethodState& state, IR::Context* ctx);
  useit Json     toJson() const;
  useit NodeType nodeType() const { return NodeType::OPERATOR_PROTOTYPE; }
  ~OperatorPrototype();
};

class OperatorDefinition {
  friend DefineCoreType;
  friend DoSkill;

  Vec<Sentence*>     sentences;
  OperatorPrototype* prototype;
  FileRange          fileRange;

public:
  OperatorDefinition(OperatorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {}

  useit static inline OperatorDefinition* create(OperatorPrototype* _prototype, Vec<Sentence*> _sentences,
                                                 FileRange _fileRange) {
    return std::construct_at(OwnNormal(OperatorDefinition), _prototype, _sentences, _fileRange);
  }

  void  define(MethodState& state, IR::Context* ctx);
  useit IR::Value* emit(MethodState& state, IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::OPERATOR_DEFINITION; }
};

} // namespace qat::ast

#endif
