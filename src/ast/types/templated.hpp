#ifndef QAT_AST_TYPES_TEMPLATED_HPP
#define QAT_AST_TYPES_TEMPLATED_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class TemplatedType : public QatType {
  String id;
  String name;

  mutable IR::QatType *typeValue = nullptr;

public:
  // NOLINTNEXTLINE(readability-identifier-length)
  TemplatedType(String id, String name, bool _variable,
                utils::FileRange _fileRange);

  useit String getID() const;
  useit String getName() const;

  void setType(IR::QatType *typ) const;
  void unsetType() const;

  useit bool isSet() const;

  useit String getTemplateID() const;
  useit String getTemplateName() const;

  useit IR::QatType *emit(IR::Context *ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif