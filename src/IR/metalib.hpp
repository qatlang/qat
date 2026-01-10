#ifndef QAT_IR_METALIB_HPP
#define QAT_IR_METALIB_HPP

namespace qat::ir {

class Mod;
class Ctx;
class ChoiceType;

class MetaLib {
  public:
	static Mod* lib;

	static void create(Ctx* irCtx);

	static ChoiceType* get_intrinsic_id(Ctx* irCtx);
};

} // namespace qat::ir

#endif
