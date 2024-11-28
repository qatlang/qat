#include "./method.hpp"
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

std::map<MethodType, String> Method::methodTypes = {{MethodType::normal, "normalMethod"},
                                                    {MethodType::staticFn, "staticFunction"},
                                                    {MethodType::valueMethod, "valueMethod"},
                                                    {MethodType::fromConvertor, "fromConvertor"},
                                                    {MethodType::toConvertor, "toConvertor"},
                                                    {MethodType::constructor, "constructor"},
                                                    {MethodType::copyConstructor, "copyConstructor"},
                                                    {MethodType::moveConstructor, "moveConstructor"},
                                                    {MethodType::copyAssignment, "copyAssignment"},
                                                    {MethodType::moveAssignment, "moveAssignment"},
                                                    {MethodType::destructor, "destructor"},
                                                    {MethodType::binaryOperator, "binaryOperator"},
                                                    {MethodType::unaryOperator, "unaryOperator"},
                                                    {MethodType::defaultConstructor, "defaultConstructor"}};

Vec<MethodParent*> MethodParent::allMemberParents = {};

MethodParent::MethodParent(MethodParentType _parentTy, void* _data) : data(_data), parentType(_parentTy) {}

MethodParent* MethodParent::create_expanded_type(ir::ExpandedType* expTy) {
  for (auto mem : allMemberParents) {
    if (mem->is_expanded()) {
      if (mem->as_expanded()->is_same(expTy)) {
        return mem;
      }
    }
  }
  return std::construct_at(OwnNormal(MethodParent), MethodParentType::expandedType, (void*)expTy);
}

MethodParent* MethodParent::create_do_skill(ir::DoneSkill* doneSkill) {
  for (auto mem : allMemberParents) {
    if (mem->is_done_skill()) {
      if (mem->as_done_skill()->get_id() == doneSkill->get_id()) {
        return mem;
      }
    }
  }
  return std::construct_at(OwnNormal(MethodParent), MethodParentType::doSkill, (void*)doneSkill);
}

bool MethodParent::is_same(ir::MethodParent* other) {
  if (is_done_skill() && other->is_done_skill()) {
    return as_done_skill()->get_id() == other->as_done_skill()->get_id();
  } else if (is_expanded() && other->is_expanded()) {
    return as_expanded()->get_id() == other->as_expanded()->get_id();
  } else {
    return false;
  }
}

bool MethodParent::is_expanded() const { return parentType == MethodParentType::expandedType; }

bool MethodParent::is_done_skill() const { return parentType == MethodParentType::doSkill; }

ir::ExpandedType* MethodParent::as_expanded() const { return (ir::ExpandedType*)data; }

ir::DoneSkill* MethodParent::as_done_skill() const { return (ir::DoneSkill*)data; }

ir::Mod* MethodParent::get_module() const {
  if (is_done_skill()) {
    return as_done_skill()->get_module();
  } else {
    return as_expanded()->get_module();
  }
}

ir::Type* MethodParent::get_parent_type() const {
  return is_done_skill() ? as_done_skill()->get_ir_type() : as_expanded();
}

FileRange MethodParent::get_type_range() const {
  return is_done_skill() ? as_done_skill()->get_type_range() : as_expanded()->get_name().range;
}

LinkNames Method::get_link_names_from(MethodParent* parent, bool isStatic, Identifier name, bool isVar,
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
    case MethodType::valueMethod: {
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

Method::Method(MethodType _fnType, bool _isVariation, MethodParent* _parent, const Identifier& _name, bool _isInline,
               ReturnType* returnType, Vec<Argument> _args, bool _is_static, Maybe<FileRange> _fileRange,
               const VisibilityInfo& _visibility_info, ir::Ctx* irCtx)
    : Function(_parent->get_module(), Identifier(_name.value, _name.range),
               get_link_names_from(_parent, _is_static, _name, _isVariation, _fnType, _args, returnType->get_type()),
               {/* Generics */}, _isInline, returnType, _args, _fileRange, _visibility_info, irCtx, true),
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
    case MethodType::valueMethod: {
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

String method_type_to_string(MethodType type) { return Method::methodTypes[type]; }

Method::~Method() {}

Method* Method::Create(MethodParent* parent, bool is_variation, const Identifier& name, bool isInline,
                       ReturnType* returnTy, const Vec<Argument>& args, Maybe<FileRange> fileRange,
                       const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx) {
  Vec<Argument> args_info;
  args_info.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                       ReferenceType::get(is_variation, parent->get_parent_type(), irCtx), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new Method(MethodType::normal, is_variation, parent, name, isInline, returnTy, std::move(args_info), false,
                    fileRange, visibilityInfo, irCtx);
}

Method* Method::CreateValued(MethodParent* parent, const Identifier& name, bool isInline, Type* returnTy,
                             const Vec<Argument>& args, Maybe<FileRange> fileRange,
                             const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx) {
  Vec<Argument> args_info;
  args_info.push_back(Argument::Create(Identifier("''", parent->get_type_range()), parent->get_parent_type(), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new Method(MethodType::valueMethod, false, parent, name, isInline, ReturnType::get(returnTy),
                    std::move(args_info), false, fileRange, visibilityInfo, irCtx);
}

Method* Method::DefaultConstructor(MethodParent* parent, FileRange nameRange, bool isInline,
                                   const VisibilityInfo& visibInfo, Maybe<FileRange> fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0));
  return new Method(MethodType::defaultConstructor, true, parent, Identifier("default", nameRange),
                    // FIXME - Make file range optional instead of creating it using empty values
                    isInline, ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, fileRange,
                    visibInfo, irCtx);
}

Method* Method::CopyConstructor(MethodParent* parent, FileRange nameRange, bool isInline, const Identifier& otherName,
                                Maybe<FileRange> fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create({"''", parent->get_type_range()},
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(false, parent->get_parent_type(), irCtx), 0));
  return new Method(MethodType::copyConstructor, true, parent, {"copy", std::move(nameRange)}, isInline,
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, fileRange,
                    VisibilityInfo::pub(), irCtx);
}

Method* Method::MoveConstructor(MethodParent* parent, FileRange nameRange, bool isInline, const Identifier& otherName,
                                Maybe<FileRange> fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create({"''", parent->get_type_range()},
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  return new Method(MethodType::moveConstructor, true, parent, {"move", std::move(nameRange)}, isInline,
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, fileRange,
                    VisibilityInfo::pub(), irCtx);
}

Method* Method::CopyAssignment(MethodParent* parent, FileRange nameRange, bool isInline, const Identifier& otherName,
                               Maybe<FileRange> fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  return new Method(MethodType::copyAssignment, true, parent, {"copy=", nameRange}, isInline,
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, fileRange,
                    VisibilityInfo::pub(), irCtx);
}

Method* Method::MoveAssignment(MethodParent* parent, FileRange nameRange, bool isInline, const Identifier& otherName,
                               const FileRange& fileRange, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->get_parent_type(), irCtx), 0u));
  return new Method(MethodType::moveAssignment, true, parent, {"move=", nameRange}, isInline,
                    ReturnType::get(ir::VoidType::get(irCtx->llctx)), std::move(argsInfo), false, fileRange,
                    VisibilityInfo::pub(), irCtx);
}

Method* Method::CreateConstructor(MethodParent* parent, FileRange nameRange, bool isInline, const Vec<Argument>& args,
                                  Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0));
  for (const auto& arg : args) {
    argsInfo.push_back(arg);
  }
  return new Method(MethodType::constructor, true, parent, Identifier("from", std::move(nameRange)), isInline,
                    ReturnType::get(VoidType::get(irCtx->llctx)), argsInfo, false, fileRange, visibInfo, irCtx);
}

Method* Method::CreateFromConvertor(MethodParent* parent, FileRange nameRange, bool isInline, Type* sourceType,
                                    const Identifier& name, Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo,
                                    ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(true, parent->get_parent_type(), irCtx), 0));
  SHOW("Created parent pointer argument")
  argsInfo.push_back(Argument::Create(name, sourceType, 1));
  SHOW("Created candidate type")
  auto result = new Method(MethodType::fromConvertor, true, parent, Identifier("from", nameRange), isInline,
                           ReturnType::get(VoidType::get(irCtx->llctx)), argsInfo, false, fileRange, visibInfo, irCtx);
  return result;
}

Method* Method::CreateToConvertor(MethodParent* parent, FileRange nameRange, bool isInline, Type* destType,
                                  Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, ir::Ctx* irCtx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                      ReferenceType::get(false, parent->get_parent_type(), irCtx), 0));
  return new Method(MethodType::toConvertor, false, parent, Identifier("to'" + destType->to_string(), nameRange),
                    isInline, ReturnType::get(destType), argsInfo, false, fileRange, visibInfo, irCtx);
}

Method* Method::CreateStatic(MethodParent* parent, const Identifier& name, bool isInline, Type* returnTy,
                             const Vec<Argument>& args, Maybe<FileRange> fileRange, const VisibilityInfo& visib_info,
                             ir::Ctx* irCtx //
) {
  return new Method(MethodType::staticFn, false, parent, name, isInline, ir::ReturnType::get(returnTy), args, true,
                    fileRange, visib_info, irCtx);
}

Method* Method::CreateDestructor(MethodParent* parent, FileRange nameRange, bool isInline, Maybe<FileRange> fileRange,
                                 ir::Ctx* irCtx) {
  SHOW("Creating destructor")
  return new Method(MethodType::destructor, true, parent, Identifier("end", nameRange), isInline,
                    ReturnType::get(VoidType::get(irCtx->llctx)),
                    Vec<Argument>({Argument::Create(Identifier("''", parent->get_type_range()),
                                                    ReferenceType::get(true, parent->get_parent_type(), irCtx), 0)}),
                    false, fileRange, VisibilityInfo::pub(), irCtx);
}

Method* Method::CreateOperator(MethodParent* parent, FileRange nameRange, bool isBinary, bool isVariationFn,
                               const String& opr, bool isInline, ReturnType* returnType, const Vec<Argument>& args,
                               Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, ir::Ctx* irCtx) {
  Vec<Argument> args_info;
  args_info.push_back(Argument::Create(Identifier("''", parent->get_type_range()),
                                       ReferenceType::get(isVariationFn, parent->get_parent_type(), irCtx), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new Method(isBinary ? MethodType::binaryOperator : MethodType::unaryOperator, isVariationFn, parent,
                    Identifier(opr, nameRange), isInline, returnType, args_info, false, fileRange, visibInfo, irCtx);
}

String Method::get_full_name() const {
  return (parent->is_done_skill() ? parent->as_done_skill()->to_string() : parent->as_expanded()->to_string()) + ":" +
         (isVariation ? "var:" : "") + name.value;
}

void Method::update_overview() {
  Vec<JsonValue> localsJson;
  for (auto* block : blocks) {
    block->output_local_overview(localsJson);
  }
  Vec<JsonValue> argsJSON;
  for (auto arg : arguments) {
    if (arg.get_type() != nullptr) {
      argsJSON.push_back(Json()
                             ._("isVar", arg.get_variability())
                             ._("name", arg.get_name().value != "" ? arg.get_name().value : JsonValue())
                             ._("typeID", arg.get_type()->get_id()));
    }
  }
  ovRange = selfName.range;
  ovInfo._("fullName", get_full_name())
      ._("name", selfName)
      ._("arguments", argsJSON)
      ._("isInSkill", is_in_skill())
      ._("skillID", is_in_skill() ? get_parent_skill()->get_id() : "")
      ._("parentTypeID", parent->get_parent_type()->get_id())
      ._("moduleID", parent->is_done_skill() ? parent->as_done_skill()->get_module()->get_id()
                                             : parent->as_expanded()->get_module()->get_id())
      ._("isVariation", isVariation)
      ._("methodType", method_type_to_string(fnType))
      ._("visibility", visibilityInfo)
      ._("isVariadic", hasVariadicArguments)
      ._("locals", localsJson);
}

} // namespace qat::ir
