#ifndef QAT_IR_TYPES_RESULT_HPP
#define QAT_IR_TYPES_RESULT_HPP

#include "qat_type.hpp"

namespace qat::IR {

class ResultType : public QatType {
  friend class QatType;

  IR::QatType* validType;
  IR::QatType* errorType;
  bool         isPacked = false;

  ResultType(IR::QatType* validType, IR::QatType* errorType, bool isPacked, IR::Context* ctx);

public:
  static ResultType* get(IR::QatType* validType, IR::QatType* errorType, bool isPacked, IR::Context* ctx);

  useit IR::QatType* getValidType() const;
  useit IR::QatType* getErrorType() const;
  useit bool         isTypePacked() const;
  useit bool         isTypeSized() const final;
  useit bool         isTriviallyCopyable() const final {
    return validType->isTriviallyCopyable() && errorType->isTriviallyCopyable();
  }
  useit bool isTriviallyMovable() const final {
    return validType->isTriviallyMovable() && errorType->isTriviallyMovable();
  }
  useit TypeKind typeKind() const final;
  useit String   toString() const final;
};

} // namespace qat::IR

#endif
