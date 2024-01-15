#ifndef QAT_PRERUN_PLAIN_INITIALISER_HPP
#define QAT_PRERUN_PLAIN_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunPlainInit : public PrerunExpression, public TypeInferrable {
private:
  Maybe<PrerunExpression*> type;
  Maybe<Vec<Identifier>>   fields;
  Vec<PrerunExpression*>   fieldValues;

public:
  PrerunPlainInit(Maybe<PrerunExpression*> _type, Maybe<Vec<Identifier>> _fields, Vec<PrerunExpression*> _fieldValues,
                  FileRange _fileRange);

  useit static PrerunPlainInit* Create(Maybe<PrerunExpression*> _type, Maybe<Vec<Identifier>> _fields,
                                       Vec<PrerunExpression*> _fieldValues, FileRange _fileRange);

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit NodeType         nodeType() const final { return NodeType::prerunPlainInitialiser; }
  useit Json             toJson() const final;
};

} // namespace qat::ast

#endif