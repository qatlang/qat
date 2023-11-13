#ifndef QAT_AST_CONSTRUCTOR_PROTOTYPE_HPP
#define QAT_AST_CONSTRUCTOR_PROTOTYPE_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "sentence.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

enum class ConstructorType {
  normal,
  copy,
  Default,
  move,
};

class ConstructorPrototype : public Node {
  friend class ConstructorDefinition;
  friend class DefineCoreType;

private:
  IR::CoreType*         coreType;
  Vec<Argument*>        arguments;
  Maybe<VisibilitySpec> visibSpec;
  ConstructorType       type;
  Maybe<Identifier>     argName;
  FileRange             nameRange;

  ConstructorPrototype(ConstructorType constrType, FileRange nameRange, Vec<Argument*> _arguments,
                       Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange, Maybe<Identifier> argName = None);

public:
  static ConstructorPrototype* Normal(FileRange nameRange, Vec<Argument*> args, Maybe<VisibilitySpec> visibSpec,
                                      FileRange fileRange);
  static ConstructorPrototype* Default(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange);
  static ConstructorPrototype* Copy(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange,
                                    Identifier argName);
  static ConstructorPrototype* Move(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange,
                                    Identifier argName);

  void setCoreType(IR::CoreType* _coreType);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::convertorPrototype; }

  ~ConstructorPrototype();
};

class ConstructorDefinition : public Node {
private:
  Vec<Sentence*>        sentences;
  ConstructorPrototype* prototype;

public:
  ConstructorDefinition(ConstructorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange);

  void setCoreType(IR::CoreType* coreType) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::memberDefinition; }
};

} // namespace qat::ast

#endif
