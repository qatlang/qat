#ifndef QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP
#define QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ConstructorCall : public Expression {
  friend class LocalDeclaration;

public:
  enum class OwnType {
    heap,
    type,
    region,
    parent,
  };

private:
  QatType*           type;
  Vec<Expression*>   args;
  Maybe<OwnType>     ownTy;
  Maybe<QatType*>    ownerType;
  Maybe<Expression*> ownCount;

  mutable IR::LocalValue*   local = nullptr;
  mutable Maybe<Identifier> irName;
  mutable bool              isVar = true;

  IR::PointerOwner getIRPtrOwnerTy(IR::Context* ctx) const;
  String           ownTyToString() const;

public:
  ConstructorCall(QatType* _type, Vec<Expression*> _args, Maybe<OwnType> _ownTy, Maybe<QatType*> _ownerType,
                  Maybe<Expression*> _ownCount, FileRange _fileRange);

  useit bool isOwning() const;

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::constructorCall; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif