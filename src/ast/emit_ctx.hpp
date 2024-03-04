#ifndef QAT_AST_EMIT_CTX_HPP
#define QAT_AST_EMIT_CTX_HPP

#include "../IR/prerun_function.hpp"
#include "../IR/qat_module.hpp"

#define Colored(val)        (cfg->noColorMode() ? "" : val)
#define ColoredOr(val, rep) (cfg->noColorMode() ? rep : val)

namespace qat::ast {

struct VisibilitySpec;

enum class LoopType {
  nTimes,
  While,
  doWhile,
  over,
  infinite,
};

struct LoopInfo {
  String          name;
  ir::Block*      mainBlock;
  ir::Block*      condBlock;
  ir::Block*      restBlock;
  ir::LocalValue* index;
  LoopType        type;

  LoopInfo(String _name, ir::Block* _mainB, ir::Block* _condB, ir::Block* _restB, ir::LocalValue* _index,
           LoopType _type)
      : name(_name), mainBlock(_mainB), condBlock(_condB), restBlock(_restB), index(_index), type(_type) {}

  useit inline bool isTimes() const { return type == LoopType::nTimes; }
};

enum class BreakableType {
  loop,
  match,
};

struct Breakable {
  Maybe<String> tag;
  ir::Block*    restBlock;
  ir::Block*    trueBlock;

  Breakable(Maybe<String> _tag, ir::Block* _restBlock, ir::Block* _trueBlock)
      : tag(std::move(_tag)), restBlock(_restBlock), trueBlock(_trueBlock) {}
};

struct EmitCtx {
  ir::Ctx* irCtx = nullptr;
  ir::Mod* mod   = nullptr;

  ir::Skill*          skill;
  ir::MemberParent*   memberParent;
  ir::OpaqueType*     parentOpaque;
  ir::Function*       fn;
  ir::PrerunFunction* preFn;

  Vec<LoopInfo>  loopsInfo;
  Vec<Breakable> breakables;

  EmitCtx(ir::Ctx* _irCtx, ir::Mod* _mod)
      : irCtx(_irCtx), mod(_mod), skill(nullptr), memberParent(nullptr), parentOpaque(nullptr), fn(nullptr),
        preFn(nullptr) {}

  useit static inline EmitCtx* get(ir::Ctx* _irCtx, ir::Mod* _mod) {
    return std::construct_at(OwnNormal(EmitCtx), _irCtx, _mod);
  }

  EmitCtx* with_member_parent(ir::MemberParent* _memberParent) {
    memberParent = _memberParent;
    return this;
  }

  EmitCtx* with_function(ir::Function* _fn) {
    fn = _fn;
    return this;
  }

  EmitCtx* with_prerun_function(ir::PrerunFunction* _preFn) {
    preFn = _preFn;
    return this;
  }

  EmitCtx* with_opaque_parent(ir::OpaqueType* _opq) {
    parentOpaque = _opq;
    return this;
  }

  useit AccessInfo get_access_info() const;

  useit inline bool              has_member_parent() const { return memberParent != nullptr; }
  useit inline ir::MemberParent* get_member_parent() const { return memberParent; }

  useit inline bool          has_fn() const { return fn != nullptr; }
  useit inline ir::Function* get_fn() const { return fn; }

  useit inline bool                has_pre_fn() const { return preFn != nullptr; }
  useit inline ir::PrerunFunction* get_pre_fn() const { return preFn; }

  useit inline bool            has_opaque_parent() const { return parentOpaque != nullptr; }
  useit inline ir::OpaqueType* get_opaque_parent() const { return parentOpaque; }

  useit inline bool       has_skill() const { return skill != nullptr; }
  useit inline ir::Skill* get_skill() const { return skill; }

  useit String color(String const& message, const char* color = colors::bold::yellow) const;

  void genericNameCheck(String const& name, FileRange const& range);

  void name_check_in_module(const Identifier& name, const String& entityType, Maybe<String> genericID = None,
                            Maybe<String> opaqueID = None);

  void Error(const String& message, Maybe<FileRange> fileRange, Maybe<Pair<String, FileRange>> pointTo = None);

  VisibilityInfo getVisibInfo(Maybe<VisibilitySpec> spec);
};

} // namespace qat::ast

#endif