#include "./qat_type.hpp"
#include "./generic_abstract.hpp"

namespace qat::ast {

QatType::QatType(FileRange _fileRange) : fileRange(std::move(_fileRange)) { allTypes.push_back(this); }

Maybe<usize> QatType::getTypeSizeInBits(IR::Context* ctx) const { return None; }

Vec<GenericAbstractType*> QatType::generics{};

Vec<QatType*> QatType::allTypes{};

void QatType::clearAll() {
  for (auto* typ : allTypes) {
    std::destroy_at(typ);
  }
}

} // namespace qat::ast