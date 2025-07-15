#include "./qat_type.hpp"
#include "./generic_abstract.hpp"

namespace qat::ast {

Type::Type(FileRangePtr _fileRange) : fileRange(_fileRange) { allTypes.push_back(this); }

Vec<GenericAbstractType*> Type::generics{};

Vec<Type*> Type::allTypes{};

void Type::clear_all() {
	for (auto* typ : allTypes) {
		std::destroy_at(typ);
	}
}

} // namespace qat::ast
