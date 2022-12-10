#include "./qat_type.hpp"

namespace qat::ast {

QatType::QatType(bool _variable, FileRange _fileRange) : variable(_variable), fileRange(std::move(_fileRange)) {
  allTypes.push_back(this);
}

Vec<TemplatedType*> QatType::templates = {};

Vec<QatType*> QatType::allTypes = {};

void QatType::clearAll() {
  for (auto* typ : allTypes) {
    delete typ;
  }
}

} // namespace qat::ast