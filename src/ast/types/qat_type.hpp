#ifndef QAT_AST_TYPES_QAT_TYPE_HPP
#define QAT_AST_TYPES_QAT_TYPE_HPP

#include "../../IR/context.hpp"
#include "../../utils/file_range.hpp"
#include "./type_kind.hpp"

#include <string>

namespace qat::ast {

class GenericAbstractType;

class QatType {
protected:
  static Vec<GenericAbstractType*> templates;

private:
  bool                 variable;
  static Vec<QatType*> allTypes;

public:
  QatType(bool _variable, FileRange _fileRange);
  FileRange fileRange;

  virtual ~QatType() = default;
  useit bool                 isConstant() const { return !variable; }
  useit bool                 isVariable() const { return variable; }
  useit virtual Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const;
  useit virtual IR::QatType* emit(IR::Context* ctx) = 0;
  useit virtual TypeKind     typeKind() const       = 0;
  useit virtual Json         toJson() const         = 0;
  useit virtual String       toString() const       = 0;
  virtual void               destroy() {}
  static void                clearAll();
};

} // namespace qat::ast

#endif