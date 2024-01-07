#ifndef QAT_PRERUN_PLAIN_INITIALISER_HPP
#define QAT_PRERUN_PLAIN_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunPlainInit : public PrerunExpression, public TypeInferrable {
private:
  Maybe<QatType*>              type;
  Vec<Pair<String, FileRange>> fields;
  Vec<PrerunExpression*>       fieldValues;

public:
  PrerunPlainInit(Maybe<QatType*> _type, Vec<Pair<String, FileRange>> _fields, Vec<PrerunExpression*> _fieldValues,
                  FileRange _fileRange);

  useit static PrerunPlainInit* Create(Maybe<QatType*> _type, Vec<Pair<String, FileRange>> _fields,
                                       Vec<PrerunExpression*> _fieldValues, FileRange _fileRange);

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit NodeType         nodeType() const final { return NodeType::prerunPlainInitialiser; }
  useit Json             toJson() const final;
};

} // namespace qat::ast

#endif