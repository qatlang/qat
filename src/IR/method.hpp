#ifndef QAT_IR_MEMBER_FUNCTION_HPP
#define QAT_IR_MEMBER_FUNCTION_HPP

#include "../utils/visibility.hpp"
#include "./argument.hpp"
#include "./function.hpp"
#include "link_names.hpp"
#include "skill.hpp"
#include "types/function.hpp"
#include "types/qat_type.hpp"

#include <set>
#include <string>

namespace qat::ast {
class MethodPrototype;
}

namespace qat::ir {

enum class MethodType {
	normal,
	valueMethod,
	staticFn,
	fromConvertor,
	toConvertor,
	constructor,
	copyConstructor,
	moveConstructor,
	copyAssignment,
	moveAssignment,
	destructor,
	binaryOperator,
	unaryOperator,
	defaultConstructor,
};

String method_type_to_string(MethodType type);

class ExpandedType;

enum class MethodParentType { expandedType, doSkill };
class MethodParent {
	static Vec<MethodParent*> allMemberParents;

	void*            data;
	MethodParentType parentType;

  public:
	MethodParent(MethodParentType _parentType, void* data);
	useit static MethodParent* create_expanded_type(ir::ExpandedType* expTy);
	useit static MethodParent* create_do_skill(ir::DoneSkill* doneSkill);

	useit bool is_expanded() const;
	useit bool is_done_skill() const;
	useit ir::Type* get_parent_type() const;
	useit ir::ExpandedType* as_expanded() const;
	useit ir::DoneSkill* as_done_skill() const;
	useit FileRange      get_type_range() const;
	useit ir::Mod* get_module() const;

	useit bool is_same(ir::MethodParent* other);
};

class Method : public Function {
	friend class ast::MethodPrototype;

	MethodParent* parent;
	bool          isStatic;
	bool          isVariation;
	MethodType    fnType;
	Identifier    selfName;

	SkillMethod* skillMethod = nullptr;

	std::set<String>            usedMembers;
	std::set<ir::Method*>       memberFunctionCalls;
	Vec<Pair<usize, FileRange>> initTypeMembers;

	static LinkNames get_link_names_from(MethodParent* parent, bool isStatic, Identifier name, bool isVar,
	                                     MethodType fnType, Vec<Argument> args, Type* retTy);

  public:
	Method(MethodType fnType, bool _isVariation, MethodParent* _parent, const Identifier& _name, bool isInline,
	       ReturnType* returnType, Vec<Argument> _args, bool _is_static, Maybe<FileRange> _fileRange,
	       const VisibilityInfo& _visibility_info, ir::Ctx* irCtx);

	static std::map<MethodType, String> methodTypes;

	useit static Method* Create(MethodParent* parent, bool is_variation, const Identifier& name, bool isInline,
	                            ReturnType* returnType, const Vec<Argument>& args, Maybe<FileRange> fileRange,
	                            const VisibilityInfo& visib_info, ir::Ctx* irCtx);

	useit static Method* CreateValued(MethodParent* parent, const Identifier& name, bool isInline, Type* return_type,
	                                  const Vec<Argument>& args, Maybe<FileRange> fileRange,
	                                  const VisibilityInfo& visib_info, ir::Ctx* irCtx);

	useit static Method* DefaultConstructor(MethodParent* parent, FileRange nameRange, bool isInline,
	                                        const VisibilityInfo& visibInfo, Maybe<FileRange> fileRange,
	                                        ir::Ctx* irCtx);

	useit static Method* CopyConstructor(MethodParent* parent, FileRange nameRange, bool isInline,
	                                     const Identifier& otherName, Maybe<FileRange> fileRange, ir::Ctx* irCtx);

	useit static Method* MoveConstructor(MethodParent* parent, FileRange nameRange, bool isInline,
	                                     const Identifier& otherName, Maybe<FileRange> fileRange, ir::Ctx* irCtx);

	useit static Method* CopyAssignment(MethodParent* parent, FileRange nameRange, bool isInline,
	                                    const Identifier& otherName, Maybe<FileRange> fileRange, ir::Ctx* irCtx);

	useit static Method* MoveAssignment(MethodParent* parent, FileRange nameRange, bool isInline,
	                                    const Identifier& otherName, const FileRange& fileRange, ir::Ctx* irCtx);

	useit static Method* CreateConstructor(MethodParent* parent, FileRange nameRange, bool isInline,
	                                       const Vec<Argument>& args, Maybe<FileRange> fileRange,
	                                       const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

	useit static Method* CreateFromConvertor(MethodParent* parent, FileRange nameRange, bool isInline, Type* sourceType,
	                                         const Identifier& name, Maybe<FileRange> fileRange,
	                                         const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

	useit static Method* CreateToConvertor(MethodParent* parent, FileRange nameRange, bool isInline, Type* destType,
	                                       Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

	useit static Method* CreateDestructor(MethodParent* parent, FileRange nameRange, bool isInline,
	                                      Maybe<FileRange> fileRange, ir::Ctx* irCtx);

	useit static Method* CreateOperator(MethodParent* parent, FileRange nameRange, bool isBinary, bool isVariationFn,
	                                    const String& opr, bool isInline, ReturnType* returnType,
	                                    const Vec<Argument>& args, Maybe<FileRange> fileRange,
	                                    const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

	useit static Method* CreateStatic(MethodParent* parent, const Identifier& name, bool isInline, Type* return_type,
	                                  const Vec<Argument>& args, Maybe<FileRange> fileRange,
	                                  const VisibilityInfo& visib_info, ir::Ctx* irCtx);

	~Method() override;

	useit Identifier get_name() const final {
		switch (fnType) {
			case MethodType::normal:
			case MethodType::binaryOperator:
			case MethodType::unaryOperator:
			case MethodType::staticFn: {
				return selfName;
			}
			default:
				return name;
		}
	}

	useit MethodType get_method_type() const { return fnType; }

	useit bool isConstructor() const {
		switch (get_method_type()) {
			case MethodType::fromConvertor:
			case MethodType::constructor:
			case MethodType::copyConstructor:
			case MethodType::moveConstructor:
			case MethodType::defaultConstructor:
				return true;
			default:
				return false;
		}
	}

	useit bool is_variation_method() const { return isStatic ? false : isVariation; }

	useit bool is_static_method() const { return isStatic; }

	useit String get_full_name() const final;

	useit bool is_method() const final { return true; }

	useit bool is_in_skill() const { return parent->is_done_skill(); }

	useit DoneSkill* get_parent_skill() const { return parent->as_done_skill(); }

	useit Type* get_parent_type() { return parent->get_parent_type(); }

	useit SkillMethod* get_skill_method() const { return skillMethod; }

	useit Json to_json() const { return Json()._("parentType", parent->get_parent_type()->get_id()); }

	useit Maybe<FileRange> is_member_initted(usize memInd) const {
		for (auto mem : initTypeMembers) {
			if (mem.first == memInd) {
				return mem.second;
			}
		}
		return None;
	}
	useit std::set<String>& get_used_members() { return usedMembers; }

	void add_used_members(String memberName) { usedMembers.insert(memberName); }

	void add_method_call(ir::Method* other) { memberFunctionCalls.insert(other); }

	void add_init_member(Pair<usize, FileRange> memInfo) { initTypeMembers.push_back(memInfo); }

	void update_overview() final;
};

} // namespace qat::ir

#endif
