#ifndef QAT_AST_TYPES_QAT_TYPE_HPP
#define QAT_AST_TYPES_QAT_TYPE_HPP

#include "../../IR/context.hpp"
#include "../../backend/cpp.hpp"
#include "../../utils/file_range.hpp"
#include "./type_kind.hpp"

#include <string>

namespace qat::ast {

class TemplatedType;

class QatType {
protected:
  static Vec<TemplatedType *> templates;

private:
  bool variable;

public:
  QatType(bool _variable, utils::FileRange _fileRange)
      : variable(_variable), fileRange(std::move(_fileRange)) {}
  utils::FileRange fileRange;

  virtual ~QatType() = default;
  useit bool                 isConstant() const { return !variable; }
  useit bool                 isVariable() const { return variable; }
  useit virtual IR::QatType *emit(IR::Context *ctx) = 0;
  useit virtual TypeKind     typeKind() const       = 0;
  useit virtual nuo::Json    toJson() const         = 0;
  useit virtual String       toString() const       = 0;
  virtual void               destroy() {}
};

} // namespace qat::ast

#endif