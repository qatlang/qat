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
  Vec<Argument*>        arguments;
  Maybe<VisibilitySpec> visibSpec;
  ConstructorType       type;
  Maybe<Identifier>     argName;
  FileRange             nameRange;

  mutable IR::MemberParent*          memberParent   = nullptr;
  mutable IR::MemberFunction*        memberFunction = nullptr;
  mutable Vec<IR::CoreType::Member*> presentMembers;
  mutable Vec<IR::CoreType::Member*> absentMembersWithDefault;
  mutable Vec<IR::CoreType::Member*> absentMembersWithoutDefault;

public:
  ConstructorPrototype(ConstructorType _constrType, FileRange _nameRange, Vec<Argument*> _arguments,
                       Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange, Maybe<Identifier> _argName = None)
      : Node(_fileRange), arguments(_arguments), visibSpec(_visibSpec), type(_constrType), argName(_argName),
        nameRange(_nameRange) {}

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

  void setMemberParent(IR::MemberParent* _memberParent) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::CONVERTOR_PROTOTYPE; }

  ~ConstructorPrototype();
};

class ConstructorDefinition : public Node {
private:
  Vec<Sentence*>        sentences;
  ConstructorPrototype* prototype;

  mutable Vec<IR::MemberFunction*> functions;

public:
  ConstructorDefinition(ConstructorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : Node(_fileRange), sentences(_sentences), prototype(_prototype) {}

  useit static inline ConstructorDefinition* create(ConstructorPrototype* _prototype, Vec<Sentence*> _sentences,
                                                    FileRange _fileRange) {
    return std::construct_at(OwnNormal(ConstructorDefinition), _prototype, _sentences, _fileRange);
  }

  void setMemberParent(IR::MemberParent* memberParent) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
