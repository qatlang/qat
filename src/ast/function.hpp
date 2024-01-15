#ifndef QAT_AST_FUNCTION_HPP
#define QAT_AST_FUNCTION_HPP

#include "../IR/context.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./sentence.hpp"
#include "meta_info.hpp"
#include "types/generic_abstract.hpp"
#include <iostream>
#include <string>

namespace qat::ast {

class FunctionPrototype final : public Node {
private:
  friend class FunctionDefinition;
  Identifier               name;
  Vec<Argument*>           arguments;
  bool                     isVariadic;
  Maybe<QatType*>          returnType;
  Maybe<MetaInfo>          metaInfo;
  Maybe<VisibilitySpec>    visibSpec;
  Maybe<PrerunExpression*> checker;
  Maybe<PrerunExpression*> genericConstraint;

  Vec<GenericAbstractType*> generics;
  IR::GenericFunction*      genericFn = nullptr;

  mutable Maybe<llvm::GlobalValue::LinkageTypes> linkageType;

  mutable Maybe<String> variantName;
  mutable IR::Function* function = nullptr;
  mutable bool          isMainFn = false;
  mutable Maybe<bool>   checkResult;

  bool hasDefinition = false;

public:
  FunctionPrototype(Identifier _name, Vec<Argument*> _arguments, bool _isVariadic, Maybe<QatType*> _returnType,
                    Maybe<PrerunExpression*> checker, Maybe<PrerunExpression*> genericConstraint,
                    Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibility, const FileRange& _fileRange,
                    Vec<GenericAbstractType*> _generics = {});

  useit bool isGeneric() const;
  useit Vec<GenericAbstractType*> getGenerics() const;
  void                            setVariantName(const String& value) const;
  void                            unsetVariantName() const;
  IR::Function*                   createFunction(IR::Context* ctx) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::functionPrototype; }
  ~FunctionPrototype() final;
};

class FunctionDefinition : public Node {
private:
  Vec<Sentence*> sentences;

public:
  FunctionDefinition(FunctionPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange);

  FunctionPrototype* prototype;

  useit bool isGeneric() const;
  void       define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::functionDefinition; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif
