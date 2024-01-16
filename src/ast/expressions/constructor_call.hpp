#ifndef QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP
#define QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ConstructorCall : public Expression, public LocalDeclCompatible, public TypeInferrable {
  friend class LocalDeclaration;

public:
  enum class OwnType {
    heap,
    type,
    region,
    parent,
  };

private:
  Maybe<QatType*>    type;
  Vec<Expression*>   args;
  Maybe<OwnType>     ownTy;
  Maybe<QatType*>    ownerType;
  Maybe<Expression*> ownCount;

  IR::PointerOwner getIRPtrOwnerTy(IR::Context* ctx) const;
  String           ownTyToString() const;

public:
  ConstructorCall(Maybe<QatType*> _type, Vec<Expression*> _args, Maybe<OwnType> _ownTy, Maybe<QatType*> _ownerType,
                  Maybe<Expression*> _ownCount, FileRange _fileRange)
      : Expression(_fileRange), type(_type), args(_args), ownTy(_ownTy), ownerType(_ownerType), ownCount(_ownCount) {}

  useit static inline ConstructorCall* create(Maybe<QatType*> _type, Vec<Expression*> _args, Maybe<OwnType> _ownTy,
                                              Maybe<QatType*> _ownerType, Maybe<Expression*> _ownCount,
                                              FileRange _fileRange) {
    return std::construct_at(OwnNormal(ConstructorCall), _type, _args, _ownTy, _ownerType, _ownCount, _fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  useit bool isOwning() const;

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::CONSTRUCTOR_CALL; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif