#include "./qat_type.hpp"
#include "./generic_abstract.hpp"

namespace qat::ast {

Type::Type(FileRange _fileRange) : fileRange(std::move(_fileRange)) { allTypes.push_back(this); }

Maybe<usize> Type::getTypeSizeInBits(EmitCtx* ctx) const { return None; }

Vec<GenericAbstractType*> Type::generics{};

Vec<Type*> Type::allTypes{};

void Type::clear_all() {
	for (auto* typ : allTypes) {
		std::destroy_at(typ);
	}
}

} // namespace qat::ast