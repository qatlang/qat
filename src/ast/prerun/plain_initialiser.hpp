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
                  FileRange _fileRange)
      : PrerunExpression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

  useit static inline PrerunPlainInit* create(Maybe<PrerunExpression*> _type, Maybe<Vec<Identifier>> _fields,
                                              Vec<PrerunExpression*> _fieldValues, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunPlainInit), _type, _fields, _fieldValues, _fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_PLAIN_INITIALISER; }
  useit Json             toJson() const final;
};

} // namespace qat::ast

#endif