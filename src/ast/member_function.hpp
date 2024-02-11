#ifndef QAT_AST_MEMBER_FUNCTION_HPP
#define QAT_AST_MEMBER_FUNCTION_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "expression.hpp"
#include "member_parent_like.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

enum class AstMemberFnType {
  Static,
  normal,
  variation,
  valued,
};

inline String member_fn_type_to_string(AstMemberFnType ty) {
  switch (ty) {
    case AstMemberFnType::Static:
      return "static";
    case AstMemberFnType::normal:
      return "normal";
    case AstMemberFnType::variation:
      return "variation";
    case AstMemberFnType::valued:
      return "valued";
  }
}

class MemberPrototype {
  friend class MemberDefinition;

  AstMemberFnType          fnTy;
  Identifier               name;
  Maybe<PrerunExpression*> condition;
  Vec<Argument*>           arguments;
  bool                     isVariadic;
  Maybe<QatType*>          returnType;
  Maybe<VisibilitySpec>    visibSpec;
  FileRange                fileRange;

public:
  MemberPrototype(AstMemberFnType _fnTy, Identifier _name, Maybe<PrerunExpression*> _condition,
                  Vec<Argument*> _arguments, bool _isVariadic, Maybe<QatType*> _returnType,
                  Maybe<VisibilitySpec> visibSpec, FileRange _fileRange);

  static MemberPrototype* Normal(bool _isVariationFn, const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                 const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<QatType*> _returnType,
                                 Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange);

  static MemberPrototype* Static(const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                 const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<QatType*> _returnType,
                                 Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange);

  static MemberPrototype* Value(const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<QatType*> _returnType,
                                Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange);

  void           define(MethodState& state, IR::Context* ctx);
  useit Json     toJson() const;
  useit NodeType nodeType() const { return NodeType::MEMBER_PROTOTYPE; }
  ~MemberPrototype();
};

class MemberDefinition {
private:
  Vec<Sentence*>   sentences;
  MemberPrototype* prototype;
  FileRange        fileRange;

public:
  MemberDefinition(MemberPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {}

  useit static inline MemberDefinition* create(MemberPrototype* _prototype, Vec<Sentence*> _sentences,
                                               FileRange _fileRange) {
    return std::construct_at(OwnNormal(MemberDefinition), _prototype, _sentences, _fileRange);
  }

  void  define(MethodState& state, IR::Context* ctx);
  useit IR::Value* emit(MethodState& state, IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
