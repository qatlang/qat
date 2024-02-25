#ifndef QAT_AST_PRERUN_TYPE_WRAPPING_HPP
#define QAT_AST_PRERUN_TYPE_WRAPPING_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class TypeWrap final : public PrerunExpression {
  ast::QatType* theType;
  bool          isExplicit;

public:
  TypeWrap(ast::QatType* _theType, bool _isExplicit, FileRange _fileRange)
      : PrerunExpression(_fileRange), theType(_theType), isExplicit(_isExplicit) {}

  useit static inline TypeWrap* create(ast::QatType* _theType, bool _isExplicit, FileRange _fileRange) {
    return std::construct_at(OwnNormal(TypeWrap), _theType, _isExplicit, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final;

  IR::PrerunValue* emit(IR::Context* ctx);
  Json             toJson() const;
  String           toString() const;
  NodeType         nodeType() const { return NodeType::TYPE_WRAP; }
};

} // namespace qat::ast

#endif