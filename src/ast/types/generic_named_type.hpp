#ifndef QAT_AST_EXPRESSIONS_GENERIC_NAMED_TYPE_HPP
#define QAT_AST_EXPRESSIONS_GENERIC_NAMED_TYPE_HPP

#include "../expression.hpp"
#include "../generics.hpp"
#include "../types/qat_type.hpp"
#include "type_kind.hpp"

namespace qat::ast {

class GenericNamedType : public QatType {
private:
  u32               relative;
  Vec<Identifier>   names;
  Vec<FillGeneric*> genericTypes;

public:
  GenericNamedType(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes, FileRange _fileRange)
      : QatType(_fileRange), relative(_relative), names(_names), genericTypes(_genericTypes) {}

  useit static inline GenericNamedType* create(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes,
                                               FileRange _fileRange) {
    return std::construct_at(OwnNormal(GenericNamedType), _relative, _names, _genericTypes, _fileRange);
  }

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit Json         toJson() const final;
  useit String       toString() const final;
  useit AstTypeKind  typeKind() const final { return AstTypeKind::GENERIC_NAMED; }
};

Maybe<IR::QatType*> handle_generic_named_type(IR::QatModule* mod, IR::Function* fun, IR::Block* curr,
                                              Identifier entityName, Vec<Identifier> names,
                                              Vec<FillGeneric*> genericTypes, AccessInfo reqInfo, FileRange fileRange,
                                              IR::Context* ctx);

} // namespace qat::ast

#endif