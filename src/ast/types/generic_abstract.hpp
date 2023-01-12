#ifndef QAT_AST_TYPES_GENERIC_ABSTRACT_HPP
#define QAT_AST_TYPES_GENERIC_ABSTRACT_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class GenericAbstractType : public QatType {
  String               id;
  String               name;
  Maybe<ast::QatType*> defaultTy;

  mutable IR::QatType* typeValue = nullptr;

public:
  // NOLINTNEXTLINE(readability-identifier-length)
  GenericAbstractType(String id, String name, bool _variable, Maybe<ast::QatType*> _defaultTy, FileRange _fileRange);

  useit String getID() const;
  useit String getName() const;

  useit bool hasDefault() const;
  useit Maybe<ast::QatType*> getDefault() const;

  void setType(IR::QatType* typ) const;
  void unsetType() const;

  useit bool isSet() const;

  useit String getTemplateID() const;
  useit String getTemplateName() const;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif