#ifndef QAT_AST_NAMED_GENERIC_HPP
#define QAT_AST_NAMED_GENERIC_HPP

#include "../../utils/helpers.hpp"
#include "../expression.hpp"
#include "./qat_type.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

class TypedGenericAbstract final : public GenericAbstractType {
  Maybe<ast::Type*>      defaultTypeAST;
  mutable ir::Type*      defaultType = nullptr;
  mutable Vec<ir::Type*> typeValue;

public:
  TypedGenericAbstract(usize _index, Identifier _name, Maybe<ast::Type*> _defaultTy, FileRange _fileRange)
      : GenericAbstractType(_index, _name, GenericKind::typedGeneric, _fileRange), defaultTypeAST(_defaultTy) {}

  useit static inline TypedGenericAbstract* create(usize _index, Identifier _name, Maybe<ast::Type*> _defaultTy,
                                                   FileRange _fileRange) {
    return std::construct_at(OwnNormal(TypedGenericAbstract), _index, std::move(_name), _defaultTy,
                             std::move(_fileRange));
  }

  useit bool hasDefault() const final;
  useit ast::Type* getDefaultAST() const { return defaultTypeAST.value(); }
  useit ir::Type* getDefault() const;

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                           EmitCtx* ctx) final {
    if (defaultTypeAST.has_value()) {
      UPDATE_DEPS(defaultTypeAST.value());
    }
  }

  void emit(EmitCtx* ctx) const final;

  useit ir::Type* get_type() const;
  useit ir::TypedGeneric* toIR() const;

  useit bool isSet() const final;
  void       setType(ir::Type* typ) const;
  void       unset() const final;

  useit Json to_json() const final;

  ~TypedGenericAbstract() final;
};

} // namespace qat::ast

#endif
