#ifndef QAT_IR_TYPES_SUM_TYPE_HPP
#define QAT_IR_TYPES_SUM_TYPE_HPP

#include "./qat_type.hpp"
#include <string>
#include <vector>

namespace qat::IR {

class QatModule;

class SumType : public QatType {
private:
  std::string name;

  std::vector<QatType *> subTypes;

  QatModule *parent;

public:
  SumType(std::string _name, std::vector<QatType *> _subTypes);

  std::string getName() const;

  std::string getFullName() const;

  unsigned getSubtypeCount() const;

  QatType *getSubtypeAt(unsigned index);

  bool isCompatible(QatType *other) const;

  TypeKind typeKind() const { return TypeKind::sumType; }

  std::string toString() const;
};

} // namespace qat::IR

#endif