#ifndef QAT_IR_TYPE_ID_HPP
#define QAT_IR_TYPE_ID_HPP

#include "../utils/macros.hpp"
#include "../utils/qat_region.hpp"

#include <unordered_map>

namespace llvm {
class Constant;
class ConstantStruct;
class Type;
class StructType;
class GlobalVariable;
}; // namespace llvm

namespace qat::ir {

class Type;
class Mod;
class Ctx;

struct ModTypeInfo {
	Mod*                  mod;
	Mod*                  infoMod;
	llvm::GlobalVariable* infoList;
	Vec<llvm::Constant*>  typeInfos;

	ModTypeInfo(Mod* _mod, Mod* _infoMod, llvm::GlobalVariable* _infoList)
	    : mod(_mod), infoMod(_infoMod), infoList(_infoList), typeInfos() {}

	useit static ModTypeInfo* create(Mod* mod, Mod* infoMod, llvm::GlobalVariable* infoList) {
		return std::construct_at(OwnNormal(ModTypeInfo), mod, infoMod, infoList);
	}
};

struct TypeInfo {
	static std::unordered_map<llvm::Constant*, TypeInfo*> idMapping;

	static Mod* builtinModule;

	static llvm::StructType* typeInfoType;

	static Vec<llvm::Constant*> builtinTypeInfos;

	static llvm::GlobalVariable* builtinGlobal;

	static Vec<ModTypeInfo*> modules;

	TypeInfo(llvm::Constant* _id, llvm::ConstantStruct* _typeInfo, Type* _type, Mod* _mod)
	    : id(_id), typeInfo(_typeInfo), type(_type), mod(_mod) {
		idMapping[id] = this;
	}

	useit static TypeInfo* get_for(llvm::Constant* id);

	useit static TypeInfo* create(ir::Ctx* ctx, Type* type, Mod* mod);

	static void finalise_type_infos(ir::Ctx* ctx);

	llvm::Constant*       id;
	llvm::ConstantStruct* typeInfo;
	Type*                 type;
	Mod*                  mod;
};

} // namespace qat::ir

#endif
