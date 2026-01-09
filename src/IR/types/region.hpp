#ifndef QAT_IR_TYPES_REGION_HPP
#define QAT_IR_TYPES_REGION_HPP

#include "../context.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/GlobalVariable.h>

namespace qat::ir {

class Mod;

class Region : public Type, public Mentionable {
  private:
	Identifier     name;
	usize          blockSize;
	Mod*           parent;
	VisibilityInfo visibInfo;
	FileRangePtr   fileRange;

	llvm::GlobalVariable* blocks;
	llvm::GlobalVariable* blockCount;
	llvm::Function*       ownFn;
	llvm::Function*       destructor;

  public:
	Region(Identifier _name, usize _blockSize, Mod* _module, const VisibilityInfo& visibInfo, ir::Ctx* irCtx,
	       FileRangePtr fileRange);

	static Region* get(Identifier name, usize blockSize, Mod* parent, const VisibilityInfo& visibInfo, ir::Ctx* irCtx,
	                   FileRangePtr fileRange);

	Identifier get_name() const;

	String get_full_name() const;

	usize get_block_size() const { return blockSize; }

	ir::Mod* get_module() const;

	ir::Value* ownData(ir::Type* _type, Maybe<llvm::Value*> count, ir::Ctx* irCtx);

	void destroyObjects(ir::Ctx* irCtx);

	bool is_accessible(const AccessInfo& reqInfo) const;

	const VisibilityInfo& get_visibility() const;

	TypeKind type_kind() const final { return TypeKind::REGION; }

	String to_string() const final;
};

} // namespace qat::ir

#endif
