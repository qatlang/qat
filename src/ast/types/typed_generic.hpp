#ifndef QAT_AST_NAMED_GENERIC_HPP
#define QAT_AST_NAMED_GENERIC_HPP

#include "../../utils/helpers.hpp"
#include "../expression.hpp"
#include "./qat_type.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

class TypedGeneric final : public GenericAbstractType {
  Maybe<ast::QatType*> defaultTypeAST;
  mutable IR::QatType* defaultType = nullptr;
  mutable IR::QatType* typeValue   = nullptr;

  TypedGeneric(usize _index, Identifier name, Maybe<ast::QatType*> _defaultTy, FileRange _fileRange);

public:
  static TypedGeneric* get(usize _index, Identifier _name, Maybe<ast::QatType*> _defaultTy, FileRange _fileRange);

  useit bool hasDefault() const final;
  useit IR::QatType* getDefault() const;

  void emit(IR::Context* ctx) const final;

  useit IR::QatType* getType() const;
  useit IR::TypedGeneric* toIR() const;

  useit bool isSet() const final;
  void       setType(IR::QatType* typ) const;
  void       unset() const final;

  ~TypedGeneric() final;
};

} // namespace qat::ast

#endif