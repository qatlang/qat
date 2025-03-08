#ifndef QAT_IR_TYPES_REGION_HPP
#define QAT_IR_TYPES_REGION_HPP

#include "../context.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/GlobalVariable.h>

namespace qat::ir {

class Mod;

class Region : public Type, public EntityOverview {
  private:
	Identifier     name;
	usize          blockSize;
	Mod*           parent;
	VisibilityInfo visibInfo;
	FileRange      fileRange;

	llvm::GlobalVariable* blocks;
	llvm::GlobalVariable* blockCount;
	llvm::Function*       ownFn;
	llvm::Function*       destructor;

  public:
	Region(Identifier _name, usize _blockSize, Mod* _module, const VisibilityInfo& visibInfo, ir::Ctx* irCtx,
	       FileRange fileRange);

	static Region* get(Identifier name, usize blockSize, Mod* parent, const VisibilityInfo& visibInfo, ir::Ctx* irCtx,
	                   FileRange fileRange);

	useit Identifier get_name() const;

	useit String get_full_name() const;

	useit usize get_block_size() const { return blockSize; }

	useit ir::Mod* get_module() const;

	useit ir::Value* ownData(ir::Type* _type, Maybe<llvm::Value*> count, ir::Ctx* irCtx);

	void destroyObjects(ir::Ctx* irCtx);

	useit bool                  is_accessible(const AccessInfo& reqInfo) const;
	useit const VisibilityInfo& get_visibility() const;

	void update_overview() final;

	useit TypeKind type_kind() const final { return TypeKind::REGION; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
