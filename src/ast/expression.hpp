#ifndef QAT_AST_EXPRESSION_HPP
#define QAT_AST_EXPRESSION_HPP

#include "./node.hpp"

namespace qat::ast {

// Nature of the expression
//
// "assignable" & "temporary" can be grouped to the generalised nature glvalue
// "pure" and "temporary" can be grouped to the generalsied nature rvalue
//
// Assignable expressions can be assigned to
enum class ExpressionKind {
  assignable, // lvalue
  temporary,  // xvalue
  pure,       // prvalue
};

#define LOCAL_DECL_COMPATIBLE_FUNCTIONS                                                                                \
  useit bool                 isLocalDeclCompatible() const final { return true; }                                      \
  useit LocalDeclCompatible* asLocalDeclCompatible() final { return (LocalDeclCompatible*)this; }

#define IN_PLACE_CREATABLE_FUNCTIONS                                                                                   \
  useit bool              isInPlaceCreatable() const final { return true; }                                            \
  useit InPlaceCreatable* asInPlaceCreatable() final { return (InPlaceCreatable*)this; }

#define TYPE_INFERRABLE_FUNCTIONS                                                                                      \
  useit bool            hasTypeInferrance() const final { return true; }                                               \
  useit TypeInferrable* asTypeInferrable() final { return (TypeInferrable*)this; }

class LocalDeclCompatible {
public:
  IR::LocalValue*   localValue = nullptr;
  Maybe<Identifier> irName;
  bool              isVar = false;
  useit inline bool isLocalDecl() const { return localValue != nullptr; }
  void inline type_check_local(IR::QatType* type, IR::Context* ctx, FileRange fileRange) {
    if (!localValue->getType()->isSame(type)) {
      ctx->Error("The type of this expression is " + ctx->highlightError(type->toString()) +
                     " which does not match the type of the local declaration, which is " +
                     ctx->highlightError(localValue->getType()->toString()),
                 fileRange);
    }
  }
  inline void setLocalValue(IR::LocalValue* _localValue) { localValue = _localValue; }
};

class InPlaceCreatable {
public:
  IR::Value*        createIn = nullptr;
  useit inline bool canCreateIn() const { return createIn != nullptr; }
  inline void       setCreateIn(IR::Value* _createIn) { createIn = _createIn; }
  inline void       unsetCreateIn() { createIn = nullptr; }
};

struct FnAtEnd {
  std::function<void()> fn;

  ~FnAtEnd() { fn(); }
};

class TypeInferrable {
public:
  IR::QatType*      inferredType = nullptr;
  useit inline bool isTypeInferred() const { return inferredType != nullptr; }
  inline void       setInferenceType(IR::QatType* _type) {
    inferredType = _type->isReference() ? _type->asReference()->getSubType() : _type;
  }
};

class Expression : public Node {
public:
  Expression(FileRange _fileRange) : Node(std::move(_fileRange)) {}
  ~Expression() override = default;

  useit virtual bool                 isLocalDeclCompatible() const { return false; }
  useit virtual LocalDeclCompatible* asLocalDeclCompatible() { return nullptr; }

  useit virtual bool              isInPlaceCreatable() const { return false; }
  useit virtual InPlaceCreatable* asInPlaceCreatable() { return nullptr; }

  useit virtual bool            hasTypeInferrance() const { return false; }
  useit virtual TypeInferrable* asTypeInferrable() { return nullptr; }

  virtual void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                                   IR::Context* ctx) = 0;

  useit virtual IR::Value* emit(IR::Context* ctx) = 0;

  useit NodeType nodeType() const override = 0;
  useit Json     toJson() const override   = 0;
};

class PrerunExpression : public Expression {
public:
  PrerunExpression(FileRange _fileRange) : Expression(_fileRange) {}
  ~PrerunExpression() override = default;

  useit IR::PrerunValue* emit(IR::Context* ctx) override = 0;
  useit NodeType         nodeType() const override       = 0;
  useit Json             toJson() const override         = 0;
  useit virtual String   toString() const                = 0;
};

} // namespace qat::ast

#endif