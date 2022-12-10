#ifndef QAT_IR_TYPES_DEFINITION_HPP
#define QAT_IR_TYPES_DEFINITION_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/helpers.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class QatModule;

class DefinitionType : public QatType {
private:
  Identifier            name;
  QatType*              subType;
  QatModule*            parent;
  utils::VisibilityInfo visibInfo;

public:
  DefinitionType(Identifier _name, QatType* _actualType, QatModule* mod, const utils::VisibilityInfo& _visibInfo);

  useit Identifier getName() const;
  useit String     getFullName() const;
  useit QatModule* getParent();
  useit QatType*   getSubType();
  useit TypeKind   typeKind() const override;
  useit String     toString() const override;
  useit Json       toJson() const override;
  useit utils::VisibilityInfo getVisibility() const;
};

} // namespace qat::IR

#endif