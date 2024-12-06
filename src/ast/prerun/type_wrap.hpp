#ifndef QAT_AST_PRERUN_TYPE_WRAPPING_HPP
#define QAT_AST_PRERUN_TYPE_WRAPPING_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class TypeWrap final : public PrerunExpression {
  ast::Type* theType;
  bool       isExplicit;

public:
  TypeWrap(ast::Type* _theType, bool _isExplicit, FileRange _fileRange)
      : PrerunExpression(_fileRange), theType(_theType), isExplicit(_isExplicit) {}

  useit static TypeWrap* create(ast::Type* _theType, bool _isExplicit, FileRange _fileRange) {
    return std::construct_at(OwnNormal(TypeWrap), _theType, _isExplicit, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

  ir::PrerunValue* emit(EmitCtx* ctx);
  Json             to_json() const;
  String           to_string() const;
  NodeType         nodeType() const { return NodeType::TYPE_WRAP; }
};

} // namespace qat::ast

#endif
