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

namespace qat::IR {

MemberParent::MemberParent(MemberParentType _parentTy, void* _data) : data(_data), parentType(_parentTy) {}

MemberParent* MemberParent::CreateFromExpanded(IR::ExpandedType* expTy) {
  return new MemberParent(MemberParentType::expandedType, (void*)expTy);
}

MemberParent* MemberParent::CreateFromDoSkill(IR::DoSkill* doneSkill) {
  return new MemberParent(MemberParentType::doSkill, (void*)doneSkill);
}

bool MemberParent::isExpanded() const { return parentType == MemberParentType::expandedType; }

bool MemberParent::isDoneSkill() const { return parentType == MemberParentType::doSkill; }

IR::ExpandedType* MemberParent::asExpanded() const { return (IR::ExpandedType*)data; }

IR::DoSkill* MemberParent::asDoneSkill() const { return (IR::DoSkill*)data; }

IR::QatType* MemberParent::getParentType() const { return isDoneSkill() ? asDoneSkill()->getType() : asExpanded(); }

FileRange MemberParent::getTypeRange() const {
  return isDoneSkill() ? asDoneSkill()->getTypeRange() : asExpanded()->getName().range;
}

LinkNames MemberFunction::getNameInfoFrom(MemberParent* parent, bool isStatic, Identifier name, bool isVar,
                                          MemberFnType fnType, Vec<Argument> args, QatType* retTy) {
  // FIXME - Update foreignID using meta info
  auto linkNames = parent->isDoneSkill() ? parent->asDoneSkill()->getLinkNames() : parent->asExpanded()->getLinkNames();
  switch (fnType) {
    case MemberFnType::staticFn: {
      linkNames.addUnit(LinkNameUnit(name.value, LinkUnitType::staticFunction), None);
      break;
    }
    case MemberFnType::normal: {
      linkNames.addUnit(LinkNameUnit(name.value, isVar ? LinkUnitType::variationMethod : LinkUnitType::method), None);
      break;
    }
    case MemberFnType::fromConvertor: {
      linkNames.addUnit(LinkNameUnit(args.at(1).getType()->getNameForLinking(), LinkUnitType::fromConvertor), None);
      break;
    }
    case MemberFnType::toConvertor: {
      linkNames.addUnit(LinkNameUnit(retTy->getNameForLinking(), LinkUnitType::toConvertor), None);
      break;
    }
    case MemberFnType::constructor: {
      Vec<LinkNames> argLinkNames;
      for (usize i = 1; i < args.size(); i++) {
        argLinkNames.push_back(LinkNames(
            {LinkNameUnit(args.at(i).getType()->getNameForLinking(), LinkUnitType::typeName)}, None, nullptr));
      }
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::constructor, None, argLinkNames), None);
      break;
    }
    case MemberFnType::copyConstructor: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::copyConstructor), None);
      break;
    }
    case MemberFnType::moveConstructor: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::moveConstructor), None);
      break;
    }
    case MemberFnType::copyAssignment: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::copyAssignment), None);
      break;
    }
    case MemberFnType::moveAssignment: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::moveAssignment), None);
      break;
    }
    case MemberFnType::destructor: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::destructor), None);
      break;
    }
    case MemberFnType::binaryOperator: {
      linkNames.addUnit(
          LinkNameUnit(name.value, isVar ? LinkUnitType::variationBinaryOperator : LinkUnitType::normalBinaryOperator,
                       None,
                       {LinkNames({LinkNameUnit(args.at(1).getType()->getNameForLinking(), LinkUnitType::typeName)},
                                  None, nullptr)}),
          None);
      break;
    }
    case MemberFnType::unaryOperator: {
      linkNames.addUnit(LinkNameUnit(name.value, LinkUnitType::unaryOperator), None);
      break;
    }
    case MemberFnType::defaultConstructor: {
      linkNames.addUnit(LinkNameUnit("", LinkUnitType::defaultConstructor), None);
      break;
    }
  }
  return linkNames;
}

MemberFunction::MemberFunction(MemberFnType _fnType, bool _isVariation, MemberParent* _parent, const Identifier& _name,
                               ReturnType* returnType, Vec<Argument> _args, bool is_variable_arguments, bool _is_static,
                               Maybe<FileRange> _fileRange, const VisibilityInfo& _visibility_info, IR::Context* ctx)
    : Function(_parent->isDoneSkill() ? _parent->asDoneSkill()->getParent() : _parent->asExpanded()->getParent(),
               Identifier(_name.value, _name.range),
               getNameInfoFrom(_parent, _is_static, _name, _isVariation, _fnType, _args, returnType->getType()),
               {/* Generics */}, returnType, std::move(_args), is_variable_arguments, _fileRange, _visibility_info, ctx,
               true),
      parent(_parent), isStatic(_is_static), isVariation(_isVariation), fnType(_fnType), selfName(_name) {
  switch (fnType) {
    case MemberFnType::defaultConstructor: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->defaultConstructor = this;
      } else {
        parent->asExpanded()->defaultConstructor = this;
      }
      break;
    }
    case MemberFnType::constructor: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->constructors.push_back(this);
      } else {
        parent->asExpanded()->constructors.push_back(this);
      }
      break;
    }
    case MemberFnType::normal: {
      selfName = _name;
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->memberFunctions.push_back(this);
      } else {
        parent->asExpanded()->memberFunctions.push_back(this);
      }
      break;
    }
    case MemberFnType::staticFn: {
      selfName = _name;
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->staticFunctions.push_back(this);
      } else {
        parent->asExpanded()->staticFunctions.push_back(this);
      }
      break;
    }
    case MemberFnType::fromConvertor: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->fromConvertors.push_back(this);
      } else {
        parent->asExpanded()->fromConvertors.push_back(this);
      }
      break;
    }
    case MemberFnType::toConvertor: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->toConvertors.push_back(this);
      } else {
        parent->asExpanded()->toConvertors.push_back(this);
      }
      break;
    }
    case MemberFnType::copyConstructor: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->copyConstructor = this;
      } else {
        parent->asExpanded()->copyConstructor = this;
      }
      break;
    }
    case MemberFnType::moveConstructor: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->moveConstructor = this;
      } else {
        parent->asExpanded()->moveConstructor = this;
      }
      break;
    }
    case MemberFnType::copyAssignment: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->copyAssignment = this;
      } else {
        parent->asExpanded()->copyAssignment = this;
      }
      break;
    }
    case MemberFnType::moveAssignment: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->moveAssignment = this;
      } else {
        parent->asExpanded()->moveAssignment = this;
      }
      break;
    }
    case MemberFnType::destructor: {
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->destructor = this;
      } else {
        parent->asExpanded()->destructor = this;
      }
      break;
    }
    case MemberFnType::binaryOperator: {
      selfName = _name;
      if (parent->isDoneSkill()) {
        if (isVariation) {
          parent->asDoneSkill()->variationBinaryOperators.push_back(this);
        } else {
          parent->asDoneSkill()->normalBinaryOperators.push_back(this);
        }
      } else {
        if (isVariation) {
          parent->asExpanded()->variationBinaryOperators.push_back(this);
        } else {
          parent->asExpanded()->normalBinaryOperators.push_back(this);
        }
      }
      break;
    }
    case MemberFnType::unaryOperator: {
      selfName = _name;
      if (parent->isDoneSkill()) {
        parent->asDoneSkill()->unaryOperators.push_back(this);
      } else {
        parent->asExpanded()->unaryOperators.push_back(this);
      }
      break;
    }
  }
}

String memberFnTypeToString(MemberFnType type) {
  switch (type) {
    case MemberFnType::normal:
      return "normalMemberFn";
    case MemberFnType::staticFn:
      return "staticFunction";
    case MemberFnType::fromConvertor:
      return "fromConvertor";
    case MemberFnType::toConvertor:
      return "toConvertor";
    case MemberFnType::constructor:
      return "constructor";
    case MemberFnType::copyConstructor:
      return "copyConstructor";
    case MemberFnType::moveConstructor:
      return "moveConstructor";
    case MemberFnType::copyAssignment:
      return "copyAssignment";
    case MemberFnType::moveAssignment:
      return "moveAssignment";
    case MemberFnType::destructor:
      return "destructor";
    case MemberFnType::binaryOperator:
      return "binaryOperator";
    case MemberFnType::unaryOperator:
      return "unaryOperator";
    case MemberFnType::defaultConstructor:
      return "defaultConstructor";
  }
}

MemberFunction::~MemberFunction() { delete parent; }

MemberFunction* MemberFunction::Create(MemberParent* parent, bool is_variation, const Identifier& name,
                                       ReturnType* returnTy, const Vec<Argument>& args, bool has_variadic_args,
                                       Maybe<FileRange> fileRange, const VisibilityInfo& visibilityInfo,
                                       IR::Context* ctx) {
  Vec<Argument> args_info;
  args_info.push_back(Argument::Create(Identifier("''", parent->getTypeRange()),
                                       ReferenceType::get(is_variation, parent->getParentType(), ctx), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new MemberFunction(MemberFnType::normal, is_variation, parent, name, returnTy, std::move(args_info),
                            has_variadic_args, false, fileRange, visibilityInfo, ctx);
}

MemberFunction* MemberFunction::DefaultConstructor(MemberParent* parent, FileRange nameRange,
                                                   const VisibilityInfo& visibInfo, Maybe<FileRange> fileRange,
                                                   IR::Context* ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->getTypeRange()),
                                      ReferenceType::get(true, parent->getParentType(), ctx), 0));
  return new MemberFunction(MemberFnType::defaultConstructor, true, parent, Identifier("default", nameRange),
                            // FIXME - Make file range optional instead of creating it using empty values
                            ReturnType::get(IR::VoidType::get(ctx->llctx)), std::move(argsInfo), false, false,
                            fileRange, visibInfo, ctx);
}

MemberFunction* MemberFunction::CopyConstructor(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                                Maybe<FileRange> fileRange, IR::Context* ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create({"''", parent->getTypeRange()}, ReferenceType::get(true, parent->getParentType(), ctx), 0));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(false, parent->getParentType(), ctx), 0));
  return new MemberFunction(MemberFnType::copyConstructor, true, parent, {"copy", std::move(nameRange)},
                            ReturnType::get(IR::VoidType::get(ctx->llctx)), std::move(argsInfo), false, false,
                            fileRange, VisibilityInfo::pub(), ctx);
}

MemberFunction* MemberFunction::MoveConstructor(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                                Maybe<FileRange> fileRange, IR::Context* ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create({"''", parent->getTypeRange()}, ReferenceType::get(true, parent->getParentType(), ctx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->getParentType(), ctx), 0u));
  return new MemberFunction(MemberFnType::moveConstructor, true, parent, {"move", std::move(nameRange)},
                            ReturnType::get(IR::VoidType::get(ctx->llctx)), std::move(argsInfo), false, false,
                            fileRange, VisibilityInfo::pub(), ctx);
}

MemberFunction* MemberFunction::CopyAssignment(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                               Maybe<FileRange> fileRange, IR::Context* ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->getTypeRange()),
                                      ReferenceType::get(true, parent->getParentType(), ctx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->getParentType(), ctx), 0u));
  return new MemberFunction(MemberFnType::copyAssignment, true, parent, {"copy=", nameRange},
                            ReturnType::get(IR::VoidType::get(ctx->llctx)), std::move(argsInfo), false, false,
                            fileRange, VisibilityInfo::pub(), ctx);
}

MemberFunction* MemberFunction::MoveAssignment(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                               const FileRange& fileRange, IR::Context* ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->getTypeRange()),
                                      ReferenceType::get(true, parent->getParentType(), ctx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent->getParentType(), ctx), 0u));
  return new MemberFunction(MemberFnType::moveAssignment, true, parent, {"move=", nameRange},
                            ReturnType::get(IR::VoidType::get(ctx->llctx)), std::move(argsInfo), false, false,
                            fileRange, VisibilityInfo::pub(), ctx);
}

MemberFunction* MemberFunction::CreateConstructor(MemberParent* parent, FileRange nameRange, const Vec<Argument>& args,
                                                  bool hasVariadicArgs, Maybe<FileRange> fileRange,
                                                  const VisibilityInfo& visibInfo, IR::Context* ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->getTypeRange()),
                                      ReferenceType::get(true, parent->getParentType(), ctx), 0));
  for (const auto& arg : args) {
    argsInfo.push_back(arg);
  }
  return new MemberFunction(MemberFnType::constructor, true, parent, Identifier("from", std::move(nameRange)),
                            ReturnType::get(VoidType::get(ctx->llctx)), argsInfo, hasVariadicArgs, false, fileRange,
                            visibInfo, ctx);
}

MemberFunction* MemberFunction::CreateFromConvertor(MemberParent* parent, FileRange nameRange, QatType* sourceType,
                                                    const Identifier& name, Maybe<FileRange> fileRange,
                                                    const VisibilityInfo& visibInfo, IR::Context* ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->getTypeRange()),
                                      ReferenceType::get(true, parent->getParentType(), ctx), 0));
  SHOW("Created parent pointer argument")
  argsInfo.push_back(Argument::Create(name, sourceType, 1));
  SHOW("Created candidate type")
  return new MemberFunction(MemberFnType::fromConvertor, true, parent, Identifier("from", nameRange),
                            ReturnType::get(VoidType::get(ctx->llctx)), argsInfo, false, false, fileRange, visibInfo,
                            ctx);
}

MemberFunction* MemberFunction::CreateToConvertor(MemberParent* parent, FileRange nameRange, QatType* destType,
                                                  Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo,
                                                  IR::Context* ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create(Identifier("''", parent->getTypeRange()),
                                      ReferenceType::get(false, parent->getParentType(), ctx), 0));
  return new MemberFunction(MemberFnType::toConvertor, false, parent,
                            Identifier("to'" + destType->toString(), nameRange), ReturnType::get(destType), argsInfo,
                            false, false, fileRange, visibInfo, ctx);
}

MemberFunction* MemberFunction::CreateStatic(MemberParent* parent, const Identifier& name, QatType* returnTy,
                                             const Vec<Argument>& args, bool has_variadic_args,
                                             Maybe<FileRange> fileRange, const VisibilityInfo& visib_info,
                                             IR::Context* ctx //
) {
  return new MemberFunction(MemberFnType::staticFn, false, parent, name, IR::ReturnType::get(returnTy), args,
                            has_variadic_args, true, fileRange, visib_info, ctx);
}

MemberFunction* MemberFunction::CreateDestructor(MemberParent* parent, FileRange nameRange, Maybe<FileRange> fileRange,
                                                 IR::Context* ctx) {
  SHOW("Creating destructor")
  return new MemberFunction(
      MemberFnType::destructor, true, parent, Identifier("end", nameRange), ReturnType::get(VoidType::get(ctx->llctx)),
      Vec<Argument>({Argument::Create(Identifier("''", parent->getTypeRange()),
                                      ReferenceType::get(true, parent->getParentType(), ctx), 0)}),
      false, false, fileRange, VisibilityInfo::pub(), ctx);
}

MemberFunction* MemberFunction::CreateOperator(MemberParent* parent, FileRange nameRange, bool isBinary,
                                               bool isVariationFn, const String& opr, ReturnType* returnType,
                                               const Vec<Argument>& args, Maybe<FileRange> fileRange,
                                               const VisibilityInfo& visibInfo, IR::Context* ctx) {
  Vec<Argument> args_info;
  args_info.push_back(Argument::Create(Identifier("''", parent->getTypeRange()),
                                       ReferenceType::get(isVariationFn, parent->getParentType(), ctx), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new MemberFunction(isBinary ? MemberFnType::binaryOperator : MemberFnType::unaryOperator, isVariationFn,
                            parent, Identifier(opr, nameRange), returnType, args_info, false, false, fileRange,
                            visibInfo, ctx);
}

bool MemberFunction::isMemberFunction() const { return true; }

String MemberFunction::getFullName() const {
  return (parent->isDoneSkill() ? parent->asDoneSkill()->toString() : parent->asExpanded()->toString()) + ":" +
         (isVariation ? "var:" : "") + name.value;
}

// Pair<bool, Maybe<FileRange>> MemberFunction::checkMissingMemberInits(IR::Context* ctx, usize index,
//                                                                      FileRange range) const {
//   std::function<Pair<bool, Maybe<FileRange>>(IR::Block*)> blockChecker =
//       [&](IR::Block* block) -> Pair<bool, Maybe<FileRange>> {
//     SHOW("   Checking block: " << block->getName())
//     const auto bName = block->getName();
//     for (auto initMem : block->initTypeMembers) {
//       if (initMem.first == index) {
//         return {true, None};
//       }
//     }
//     SHOW(bName << ": Checking children blocks")
//     Vec<Pair<bool, Maybe<FileRange>>> childrenResult;
//     for (usize i = 0; i < block->children.size(); i++) {
//       if (block->children.at(i)->hasNextBlock()) {
//         auto startBlock = block->children.at(i);
//         while (((i + 1) < block->children.size()) &&
//                block->children.at(i)->getNextBlock()->getID() == block->children.at(i + 1)->getID()) {
//           i++;
//         }
//         if (i < block->children.size()) {
//           auto startRes = blockChecker(startBlock);
//           childrenResult.push_back(startRes);
//         } else {
//           SHOW(bName << ": All child blocks are connected")
//           auto startRes = blockChecker(startBlock);
//           if (startRes.first) {
//             return startRes;
//           } else {
//             childrenResult.push_back(startRes);
//             break;
//           }
//         }
//       } else {
//         SHOW(bName << ": There is no next block for child")
//         auto childRes = blockChecker(block->children.at(i));
//         childrenResult.push_back(childRes);
//       }
//       if (!childrenResult.back().first) {
//         SHOW(bName << ": Children result is false")
//         break;
//       }
//     }
//     bool childResult     = true;
//     bool oneChildHasInit = false;
//     for (auto child : childrenResult) {
//       if (!child.first) {
//         childResult = false;
//       } else {
//         oneChildHasInit = true;
//       }
//     }
//     for (auto child : childrenResult) {
//       if (oneChildHasInit && !child.first) {
//         return child;
//       }
//     }
//     if (childResult) {
//       return {true, None};
//     }
//     if (block->nextBlock) {
//       SHOW("Has next block: " << block->nextBlock->getName())
//       auto nextRes = blockChecker(block->nextBlock);
//       if (nextRes.first) {
//         SHOW("   Ending check: " << block->getName())
//         return nextRes;
//       } else {
//         FileRange resRange = range;
//         bool      isFirst  = true;
//         for (auto child : childrenResult) {
//           if (!child.first) {
//             if (isFirst) {
//               resRange = block->getFileRange().value_or(child.second.value());
//               if (block->getFileRange().has_value()) {
//                 resRange = resRange.trimTo(child.second.value().start);
//               }
//             }
//             resRange = resRange.spanTo(child.second.value());
//           }
//         }
//         return {false, resRange};
//       }
//     } else {
//       FileRange resRange = range;
//       bool      isFirst  = true;
//       for (auto child : childrenResult) {
//         if (!child.first) {
//           if (isFirst) {
//             resRange = block->getFileRange().value_or(child.second.value());
//             if (block->getFileRange().has_value()) {
//               resRange = resRange.trimTo(child.second.value().start);
//             }
//           }
//           resRange = resRange.spanTo(child.second.value());
//         }
//       }
//       return {false, resRange};
//     }
//     SHOW("   Ending check: " << block->getName())
//     return {false, block->getFileRange().value_or(range)};
//   };
//   if (getFirstBlock()) {
//     return blockChecker(getFirstBlock());
//   } else {
//     return {false, range};
//   }
// }

void MemberFunction::updateOverview() {
  Vec<JsonValue> localsJson;
  for (auto* block : blocks) {
    block->outputLocalOverview(localsJson);
  }
  ovRange = selfName.range;
  ovInfo._("fullName", getFullName())
      ._("selfName", selfName)
      ._("isInSkill", isInSkill())
      ._("skillID", isInSkill() ? getParentSkill()->getID() : "")
      ._("parentTypeID", parent->getParentType()->getID())
      ._("moduleID", parent->isDoneSkill() ? parent->asDoneSkill()->getParent()->getID()
                                           : parent->asExpanded()->getParent()->getID())
      ._("isStatic", isStatic)
      ._("isVariation", isVariation)
      ._("memberFunctionType", memberFnTypeToString(fnType))
      ._("visibility", visibility_info)
      ._("isVariadic", hasVariadicArguments)
      ._("locals", localsJson);
}

} // namespace qat::IR