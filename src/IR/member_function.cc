#include "./member_function.hpp"
#include "../show.hpp"
#include "./context.hpp"
#include "./qat_module.hpp"
#include "argument.hpp"
#include "function.hpp"
#include "link_names.hpp"
#include "skill.hpp"
#include "types/expanded_type.hpp"
#include "types/function.hpp"
#include "types/pointer.hpp"
#include "types/qat_type.hpp"
#include "types/reference.hpp"
#include "types/void.hpp"

namespace qat::ir {

Vec<MemberParent*> MemberParent::allMemberParents = {};

MemberParent::MemberParent(MemberParentType _parentTy, void* _data) : data(_data), parentType(_parentTy) {}

MemberParent* MemberParent::create_expanded_type(ir::ExpandedType* expTy) {
  for (auto mem : allMemberParents) {
    if (mem->is_expanded()) {
      if (mem->as_expanded()->is_same(expTy)) {
        return mem;
      }
    }
  }
  return std::construct_at(OwnNormal(MemberParent), MemberParentType::expandedType, (void*)expTy);
}

MemberParent* MemberParent::create_do_skill(ir::DoneSkill* doneSkill) {
  for (auto mem : allMemberParents) {
    if (mem->is_done_skill()) {
      if (mem->as_done_skill()->get_id() == doneSkill->get_id()) {
        return mem;
      }
    }
  }
  return std::construct_at(OwnNormal(MemberParent), MemberParentType::doSkill, (void*)doneSkill);
}

bool MemberParent::is_same(ir::MemberParent* other) {
  if (is_done_skill() && other->is_done_skill()) {
    return as_done_skill()->get_id() == other->as_done_skill()->get_id();
  } else if (is_expanded() && other->is_expanded()) {
    return as_expanded()->get_id() == other->as_expanded()->get_id();
  } else {
    return false;
  }
}

bool MemberParent::is_expanded() const { return parentType == MemberParentType::expandedType; }

bool MemberParent::is_done_skill() const { return parentType == MemberParentType::doSkill; }

ir::ExpandedType* MemberParent::as_expanded() const { return (ir::ExpandedType*)data; }

ir::DoneSkill* MemberParent::as_done_skill() const { return (ir::DoneSkill*)data; }

ir::Mod* MemberParent::get_module() const {
  if (is_done_skill()) {
    return as_done_skill()->get_module();
  } else {
    return as_expanded()->get_module();
  }
}

ir::Type* MemberParent::get_parent_type() const {
  return is_done_skill() ? as_done_skill()->get_ir_type() : as_expanded();
}

FileRange MemberParent::get_type_range() const {
  return is_done_skill() ? as_done_skill()->get_type_range() : as_expanded()->get_name().range;
}

LinkNames Method::get_link_names_from(MemberParent* parent, bool isStatic, Identifier name, bool isVar,
                                      MethodType fnType, Vec<Argument> args, Type* retTy) {
  // FIXME - Update foreignID using meta info
  auto linkNames =
      parent->is_done_skill() ? parent->as_done_skill()->get_link_names() : parent->as_expanded()->get_link_names();
  switch (fnType) {
    case MethodType::staticFn: {
      linkNames.addUnit(LinkNameUnit(name.value, LinkUnitType::staticFunction), None);
      break;
    }
    case MethodType::normal: {
      linkNames.addUnit(LinkNameUnit(name.value, isVar ? LinkUnitType::variationMethod : LinkUnitType::method), None);
      break;
    }
    case MethodType::value_method: {
      linkNames.addUnit(LinkNameUnit(name.value, LinkUnitType::value_method), None);
      break;
    }
    case MethodType::fromConvertor: {
      linkNames.addUnit(LinkNameUnit(args.at(1).get_type()->get_name_for_linking(), LinkUnitType::fromConvertor), None);
      break;
    }
    case MethodType::toConvertor: {
      linkNames.addUnit(LinkNameUnit(retTy->get_name_for_linking(), LinkUnitType::toConvertor), None);
      break;
    }
    case MethodType::constructor: {
      Vec<LinkNames> argLinkNames;
      for (usize i = 1; i < args.size(); i++) {
        argLinkNames.push_back(LinkNames(
            {LinkNameUnit(args.at(i).get_type()->get_name_for_linking(), LinkUnitType::typeName)}, None, nullptr));
      }
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::constructor, argLinkNames), None);
      break;
    }
    case MethodType::copyConstructor: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::copyConstructor), None);
      break;
    }
    case MethodType::moveConstructor: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::moveConstructor), None);
      break;
    }
    case MethodType::copyAssignment: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::copyAssignment), None);
      break;
    }
    case MethodType::moveAssignment: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::moveAssignment), None);
      break;
    }
    case MethodType::destructor: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::destructor), None);
      break;
    }
    case MethodType::binaryOperator: {
      linkNames.addUnit(
          LinkNameUnit(name.value, isVar ? LinkUnitType::variationBinaryOperator : LinkUnitType::normalBinaryOperator,
                       {LinkNames({LinkNameUnit(args.at(1).get_type()->get_name_for_linking(), LinkUnitType::typeName)},
                                  None, nullptr)}),
          None);
      break;
    }
    case MethodType::unaryOperator: {
      linkNames.addUnit(LinkNameUnit(name.value, LinkUnitType::unaryOperator), None);
      break;
    }
    case MethodType::defaultConstructor: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::defaultConstructor), None);
      break;
    }
  }
  return linkNames;
}

Method::Method(MethodType _fnType, bool _isVariation, MemberParent* _parent, const Identifier& _name,
               ReturnType* returnType, Vec<Argument> _args, bool is_variable_arguments, bool _is_static,
               Maybe<FileRange> _fileRange, const VisibilityInfo& _visibility_info, ir::Ctx* irCtx)
    : Function(_parent->is_done_skill() ? _parent->as_done_skill()->get_module() : _parent->as_expanded()->get_module(),
               Identifier(_name.value, _name.range),
               get_link_names_from(_parent, _is_static, _name, _isVariation, _fnType, _args, returnType->get_type()),
               {/* Generics */}, returnType, std::move(_args), is_variable_arguments, _fileRange, _visibility_info,
               irCtx, true),
      parent(_parent), isStatic(_is_static), isVariation(_isVariation), fnType(_fnType), selfName(_name) {
  switch (fnType) {
    case MethodType::defaultConstructor: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->defaultConstructor = this;
      } else {
        parent->as_expanded()->defaultConstructor = this;
      }
      break;
    }
    case MethodType::constructor: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->constructors.push_back(this);
      } else {
        parent->as_expanded()->constructors.push_back(this);
      }
      break;
    }
    case MethodType::normal: {
      selfName = _name;
      if (parent->is_done_skill()) {
        parent->as_done_skill()->memberFunctions.push_back(this);
      } else {
        parent->as_expanded()->memberFunctions.push_back(this);
      }
      break;
    }
    case MethodType::value_method: {
      selfName = _name;
      if (parent->is_done_skill()) {
        parent->as_done_skill()->valuedMemberFunctions.push_back(this);
      } else {
        parent->as_expanded()->valuedMemberFunctions.push_back(this);
      }
      break;
    }
    case MethodType::staticFn: {
      selfName = _name;
      if (parent->is_done_skill()) {
        parent->as_done_skill()->staticFunctions.push_back(this);
      } else {
        parent->as_expanded()->staticFunctions.push_back(this);
      }
      break;
    }
    case MethodType::fromConvertor: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->fromConvertors.push_back(this);
      } else {
        parent->as_expanded()->fromConvertors.push_back(this);
      }
      break;
    }
    case MethodType::toConvertor: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->toConvertors.push_back(this);
      } else {
        parent->as_expanded()->toConvertors.push_back(this);
      }
      break;
    }
    case MethodType::copyConstructor: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->copyConstructor = this;
      } else {
        parent->as_expanded()->copyConstructor = this;
      }
      break;
    }
    case MethodType::moveConstructor: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->moveConstructor = this;
      } else {
        parent->as_expanded()->moveConstructor = this;
      }
      break;
    }
    case MethodType::copyAssignment: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->copyAssignment = this;
      } else {
        parent->as_expanded()->copyAssignment = this;
      }
      break;
    }
    case MethodType::moveAssignment: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->moveAssignment = this;
      } else {
        parent->as_expanded()->moveAssignment = this;
      }
      break;
    }
    case MethodType::destructor: {
      if (parent->is_done_skill()) {
        parent->as_done_skill()->destructor = this;
      } else {
        parent->as_expanded()->destructor = this;
      }
      break;
    }
    case MethodType::binaryOperator: {
      selfName = _name;
      if (parent->is_done_skill()) {
        if (isVariation) {
          parent->as_done_skill()->variationBinaryOperators.push_back(this);
        } else {
          parent->as_done_skill()->normalBinaryOperators.push_back(this);
        }
      } else {
        if (isVariation) {
          parent->as_expanded()->variationBinaryOperators.push_back(this);
        } else {
          parent->as_expanded()->normalBinaryOperators.push_back(this);
        }
      }
      break;
    }
    case MethodType::unaryOperator: {
      selfName = _name;
      if (parent->is_done_skill()) {
        parent->as_done_skill()->unaryOperators.push_back(this);
      } else {
        parent->as_expanded()->unaryOperators.push_back(this);
      }
      break;
    }
  }
}

String memberFnTypeToString(MethodType type) {
  switch (type) {
    case MethodType::normal:
      return "normalMemberFn";
    case MethodType::staticFn:
      return "staticFunction";
    case MethodType::value_method:
      return "valuedMethod";
    case MethodType::fromConvertor:
      return "fromConvertor";
    case MethodType::toConvertor:
      return "toConvertor";
    case MethodType::constructor:
      return "constructor";
    case MethodType::copyConstructor:
      return "copyConstructor";
    case MethodType::moveConstructor:
      return "moveConstructor";
    case MethodType::copyAssignment:
      return "copyAssignment";
    case MethodType::moveAssignment:
      return "moveAssignment";
    case MethodType::destructor:
      return "destructor";
    case MethodType::binaryOperator:
      return "binaryOperator";
    case MethodType::unaryOperator:
      return "unaryOperator";
    case MethodType::defaultConstructor:
      return "defaultConstructor";
  }
}

Method::~Method() {}

Method* Method::Create(MemberParent* parent, bool is_variation, const Identifier& name, ReturnType* returnTy,
                       const Vec<Argument>& args, bool has_variadic_args, Maybe<FileRange> fileRange,
                       const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx) {
  Vec<Argument> args_info;
  args_info.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                       ReferenceType::get(is_variation, parent->get_parent_type(), irCtx), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new Method(MethodType::normal, is_variation, parent, name, returnTy, std::move(args_info), has_variadic_args,
                    false, fileRange, visibilityInfo, irCtx);
}

Method* Method::CreateValued(MemberParent* parent, const Identifier& name, Type* returnTy, const Vec<Argument>& args,
                             bool has_variadic_args, Maybe<FileRange> fileRange, const VisibilityInfo& visibilityInfo,
                             ir::Ctx* irCtx) {
  Vec<Argument> args_info;
  args_info.push_back(Argument::Create(Identifier("''", parent->get_type_range()), parent->get_parent_type(), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new Method(MethodType::value_method, false, parent, name, ir::ReturnType::get(returnTy), std::move(args_info),
                    has_variadic_args, false, fileRange, visibilityInfo, irCtx);
}

Method* Method::DefaultConstructor(MemberParent* parent, FileRange nameRange, const VisibilityInfo& visibInfo,
                                   Maybe<FileRange> fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0));
  return new Method(MethodType::defaultConstructor, true, parent, Identifier("default", nameRange),
                    // FIXME - Make file range optional instead of creating it using empty values
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, false, fileRange,
                    visibInfo, irCtx);
}

Method* Method::CopyConstructor(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                Maybe<FileRange> fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create({"''", parent->get_type_range()},
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(false, parent->get_parent_type(), irCtx), 0));
  return new Method(MethodType::copyConstructor, true, parent, {"copy", std::move(nameRange)},
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, false, fileRange,
                    VisibilityInfo::pub(), irCtx);
}

Method* Method::MoveConstructor(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                Maybe<FileRange> fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create({"''", parent->get_type_range()},
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  return new Method(MethodType::moveConstructor, true, parent, {"move", std::move(nameRange)},
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, false, fileRange,
                    VisibilityInfo::pub(), irCtx);
}

Method* Method::CopyAssignment(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                               Maybe<FileRange> fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  return new Method(MethodType::copyAssignment, true, parent, {"copy=", nameRange},
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, false, fileRange,
                    VisibilityInfo::pub(), irCtx);
}

Method* Method::MoveAssignment(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                               const FileRange& fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  return new Method(MethodType::moveAssignment, true, parent, {"move=", nameRange},
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, false, fileRange,
                    VisibilityInfo::pub(), irCtx);
}

Method* Method::CreateConstructor(MemberParent* parent, FileRange nameRange, const Vec<Argument>& args,
                                  bool hasVariadicArgs, Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo,
                                  ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0));
  for (const auto& arg : args) {
    argsInfo.push_back(arg);
  }
  return new Method(MethodType::constructor, true, parent, Identifier("from", std::move(nameRange)),
                    ReturnType::get(VoidType::get(irCtx->llctx)), argsInfo, hasVariadicArgs, false, fileRange,
                    visibInfo, irCtx);
}

Method* Method::CreateFromConvertor(MemberParent* parent, FileRange nameRange, Type* sourceType, const Identifier& name,
                                    Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0));
  SHOW("Created parent pointer argument")
  argsInfo.push_back(Argument::Create(name, sourceType, 1));
  SHOW("Created candidate type")
  return new Method(MethodType::fromConvertor, true, parent, Identifier("from", nameRange),
                    ReturnType::get(VoidType::get(irCtx->llctx)), argsInfo, false, false, fileRange, visibInfo, irCtx);
}

Method* Method::CreateToConvertor(MemberParent* parent, FileRange nameRange, Type* destType, Maybe<FileRange> fileRange,
                                  const VisibilityInfo& visibInfo, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(false, parent->get_parent_type(), irCtx), 0));
  return new Method(MethodType::toConvertor, false, parent, Identifier("to'" + destType->to_string(), nameRange),
                    ReturnType::get(destType), argsInfo, false, false, fileRange, visibInfo, irCtx);
}

Method* Method::CreateStatic(MemberParent* parent, const Identifier& name, Type* returnTy, const Vec<Argument>& args,
                             bool has_variadic_args, Maybe<FileRange> fileRange, const VisibilityInfo& visib_info,
                             ir::Ctx* irCtx //
) {
  return new Method(MethodType::staticFn, false, parent, name, ir::ReturnType::get(returnTy), args, has_variadic_args,
                    true, fileRange, visib_info, irCtx);
}

Method* Method::CreateDestructor(MemberParent* parent, FileRange nameRange, Maybe<FileRange> fileRange,
                                 ir::Ctx* irCtx) {
  SHOW("Creating destructor")
  return new Method(MethodType::destructor, true, parent, Identifier("end", nameRange),
                    ReturnType::get(VoidType::get(irCtx->llctx)),
                    Vec<Argument>({Argument::Create(Identifier("''", parent->get_type_range()),
                                                    ReferenceType::get(true, parent->get_parent_type(), irCtx), 0)}),
                    false, false, fileRange, VisibilityInfo::pub(), irCtx);
}

Method* Method::CreateOperator(MemberParent* parent, FileRange nameRange, bool isBinary, bool isVariationFn,
                               const String& opr, ReturnType* returnType, const Vec<Argument>& args,
                               Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, ir::Ctx* irCtx) {
  Vec<Argument> args_info;
  args_info.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                       ReferenceType::get(isVariationFn, parent->get_parent_type(), irCtx), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new Method(isBinary ? MethodType::binaryOperator : MethodType::unaryOperator, isVariationFn, parent,
                    Identifier(opr, nameRange), returnType, args_info, false, false, fileRange, visibInfo, irCtx);
}

bool Method::isMemberFunction() const { return true; }

String Method::get_full_name() const {
  return (parent->is_done_skill() ? parent->as_done_skill()->to_string() : parent->as_expanded()->to_string()) + ":" +
         (isVariation ? "var:" : "") + name.value;
}

void Method::update_overview() {
  Vec<JsonValue> localsJson;
  for (auto* block : blocks) {
    block->outputLocalOverview(localsJson);
  }
  ovRange = selfName.range;
  ovInfo._("fullName", get_full_name())
      ._("selfName", selfName)
      ._("isInSkill", isInSkill())
      ._("skillID", isInSkill() ? getParentSkill()->get_id() : "")
      ._("parentTypeID", parent->get_parent_type()->get_id())
      ._("moduleID", parent->is_done_skill() ? parent->as_done_skill()->get_module()->get_id()
                                             : parent->as_expanded()->get_module()->get_id())
      ._("isVariation", isVariation)
      ._("methodType", memberFnTypeToString(fnType))
      ._("visibility", visibility_info)
      ._("isVariadic", hasVariadicArguments)
      ._("locals", localsJson);
}

} // namespace qat::ir