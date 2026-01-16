#ifndef QAT_AST_TYPES_LOCALITY_HPP
#define QAT_AST_TYPES_LOCALITY_HPP

#include "../../IR/types/pointer.hpp"
#include "../../utils/file_range.hpp"

namespace qat::ir {
class Type;
}

namespace qat::ast {

struct EmitCtx;
class Type;

enum class LocalityKind : u8 {
	NONE,
	HEAP,
	SELF_INSTANCE,
	OWN,
	REGION_TYPE,
	ANY_REGION,
	STATIC,
	PRERUN,
};

struct Locality {
	LocalityKind kind;
	ast::Type*   candidate = nullptr;
	FileRangePtr range;

	static Locality in_own(FileRangePtr range) {
		return Locality{.kind = LocalityKind::OWN, .candidate = nullptr, .range = range};
	}

	static Locality in_heap(FileRangePtr range) {
		return Locality{.kind = LocalityKind::HEAP, .candidate = nullptr, .range = range};
	}

	static Locality in_static(FileRangePtr range) {
		return Locality{.kind = LocalityKind::STATIC, .candidate = nullptr, .range = range};
	}

	static Locality in_any_region(FileRangePtr range) {
		return Locality{.kind = LocalityKind::REGION_TYPE, .candidate = nullptr, .range = range};
	}

	static Locality in_region_type(ast::Type* region, FileRangePtr range) {
		return Locality{.kind = LocalityKind::REGION_TYPE, .candidate = region, .range = range};
	}

	static Locality in_self_instance(FileRangePtr range) {
		return Locality{.kind = LocalityKind::SELF_INSTANCE, .candidate = nullptr, .range = range};
	}

	static Locality none(FileRangePtr range) {
		return Locality{.kind = LocalityKind::NONE, .candidate = nullptr, .range = range};
	}

	static Locality in_prerun(FileRangePtr range) {
		return Locality{.kind = LocalityKind::PRERUN, .candidate = nullptr, .range = range};
	}

	String to_string() const;
};

ir::Locality get_locality(EmitCtx* ctx, Locality owner, FileRangePtr fileRange);

String locality_to_string(LocalityKind ownType);

} // namespace qat::ast

#endif
