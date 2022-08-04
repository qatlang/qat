#ifndef QAT_IR_TYPES_SUM_TYPE_HPP
#define QAT_IR_TYPES_SUM_TYPE_HPP

#include "./qat_type.hpp"
#include <string>
#include <vector>

namespace qat::IR {

class QatModule;

class SumType : public QatType {
private:
  String name;

  Vec<QatType *> subTypes;

  QatModule *parent;

public:
  SumType(String _name, Vec<QatType *> _subTypes);

  useit String getName() const;

  useit String getFullName() const;

  useit u32 getSubtypeCount() const;

  QatType *getSubtypeAt(usize index);

  bool isCompatible(QatType *other) const;

  useit TypeKind typeKind() const override { return TypeKind::sumType; }

  useit String toString() const override;
};

} // namespace qat::IR

#endif