#ifndef QAT_IR_TYPES_DEFINITION_HPP
#define QAT_IR_TYPES_DEFINITION_HPP

#include "../../utils/helpers.hpp"
#include "../../utils/visibility.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class QatModule;

class DefinitionType : public QatType {
private:
  String                name;
  QatType              *subType;
  QatModule            *parent;
  utils::VisibilityInfo visibInfo;

public:
  DefinitionType(String _name, QatType *_actualType, QatModule *mod,
                 const utils::VisibilityInfo &_visibInfo);

  useit String     getName() const;
  useit String     getFullName() const;
  useit QatModule *getParent();
  useit QatType   *getSubType();
  useit utils::VisibilityInfo getVisibility() const;
  useit TypeKind              typeKind() const override;
  useit String                toString() const override;
  useit Json                  toJson() const override;
};

} // namespace qat::IR

#endif