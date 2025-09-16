#ifndef QAT_AST_TYPES_PTR_OWNER_HPP
#define QAT_AST_TYPES_PTR_OWNER_HPP

#include "../../IR/types/pointer.hpp"
#include "../../utils/file_range.hpp"
#include "../../utils/macros.hpp"

namespace qat::ir {
class Type;
}

namespace qat::ast {

struct EmitCtx;
class Type;

enum class OwnerKind : u8 {
	NONE,
	HEAP,
	SELF_INSTANCE,
	OWN,
	REGION_TYPE,
	ANY_REGION,
	STATIC,
};

struct PtrOwner {
	OwnerKind    kind;
	ast::Type*   candidate = nullptr;
	FileRangePtr range;

	useit static PtrOwner of_own(FileRangePtr range) {
		return PtrOwner{.kind = OwnerKind::OWN, .candidate = nullptr, .range = range};
	}

	useit static PtrOwner of_heap(FileRangePtr range) {
		return PtrOwner{.kind = OwnerKind::HEAP, .candidate = nullptr, .range = range};
	}

	useit static PtrOwner of_static(FileRangePtr range) {
		return PtrOwner{.kind = OwnerKind::STATIC, .candidate = nullptr, .range = range};
	}

	useit static PtrOwner of_any_region(FileRangePtr range) {
		return PtrOwner{.kind = OwnerKind::REGION_TYPE, .candidate = nullptr, .range = range};
	}

	useit static PtrOwner of_region_type(ast::Type* region, FileRangePtr range) {
		return PtrOwner{.kind = OwnerKind::REGION_TYPE, .candidate = region, .range = range};
	}

	useit static PtrOwner of_self_instance(FileRangePtr range) {
		return PtrOwner{.kind = OwnerKind::SELF_INSTANCE, .candidate = nullptr, .range = range};
	}

	useit static PtrOwner of_none(FileRangePtr range) {
		return PtrOwner{.kind = OwnerKind::NONE, .candidate = nullptr, .range = range};
	}

	useit Json to_json() const;

	useit String to_string() const;
};

useit ir::PtrOwner get_ptr_owner(EmitCtx* ctx, PtrOwner owner, FileRangePtr fileRange);

useit String ptr_owner_to_string(OwnerKind ownType);

} // namespace qat::ast

#endif
