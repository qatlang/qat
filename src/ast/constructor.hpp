#ifndef QAT_AST_CONSTRUCTOR_PROTOTYPE_HPP
#define QAT_AST_CONSTRUCTOR_PROTOTYPE_HPP

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

enum class ConstructorType {
  normal,
  copy,
  Default,
  move,
};

class ConstructorPrototype {
  friend class ConstructorDefinition;
  friend class DefineCoreType;

private:
  Vec<Argument*>        arguments;
  Maybe<VisibilitySpec> visibSpec;
  ConstructorType       type;
  Maybe<Identifier>     argName;
  FileRange             nameRange;
  FileRange             fileRange;

  mutable Vec<IR::CoreType::Member*> presentMembers;
  mutable Vec<IR::CoreType::Member*> absentMembersWithDefault;
  mutable Vec<IR::CoreType::Member*> absentMembersWithoutDefault;

public:
  ConstructorPrototype(ConstructorType _constrType, FileRange _nameRange, Vec<Argument*> _arguments,
                       Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange, Maybe<Identifier> _argName = None)
      : arguments(_arguments), visibSpec(_visibSpec), type(_constrType), argName(_argName), nameRange(_nameRange),
        fileRange(_fileRange) {}

  static ConstructorPrototype* Normal(FileRange nameRange, Vec<Argument*> args, Maybe<VisibilitySpec> visibSpec,
                                      FileRange fileRange) {
    return std::construct_at(OwnNormal(ConstructorPrototype), ConstructorType::normal, nameRange, std::move(args),
                             visibSpec, std::move(fileRange));
  }

  static ConstructorPrototype* Default(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange) {
    return std::construct_at(OwnNormal(ConstructorPrototype), ConstructorType::Default, nameRange, Vec<Argument*>{},
                             visibSpec, std::move(fileRange));
  }

  static ConstructorPrototype* Copy(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange,
                                    Identifier _argName) {
    return std::construct_at(OwnNormal(ConstructorPrototype), ConstructorType::copy, nameRange, Vec<Argument*>{},
                             visibSpec, std::move(fileRange), _argName);
  }

  static ConstructorPrototype* Move(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange,
                                    Identifier _argName) {
    return std::construct_at(OwnNormal(ConstructorPrototype), ConstructorType::move, nameRange, Vec<Argument*>{},
                             visibSpec, std::move(fileRange), _argName);
  }

  void           define(MethodState& state, IR::Context* ctx);
  useit Json     toJson() const;
  useit NodeType nodeType() const { return NodeType::CONVERTOR_PROTOTYPE; }

  ~ConstructorPrototype();
};

class ConstructorDefinition {
  friend DefineCoreType;
  friend DoSkill;

  Vec<Sentence*>        sentences;
  ConstructorPrototype* prototype;
  FileRange             fileRange;

public:
  ConstructorDefinition(ConstructorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {}

  useit static inline ConstructorDefinition* create(ConstructorPrototype* _prototype, Vec<Sentence*> _sentences,
                                                    FileRange _fileRange) {
    return std::construct_at(OwnNormal(ConstructorDefinition), _prototype, _sentences, _fileRange);
  }

  void  define(MethodState& state, IR::Context* ctx);
  useit IR::Value* emit(MethodState& state, IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
