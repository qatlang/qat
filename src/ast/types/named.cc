#include "./named.hpp"
#include "../../IR/stdlib.hpp"
#include "../../IR/types/choice.hpp"
#include "../../IR/types/region.hpp"
#include "../type_definition.hpp"

namespace qat::ast {

void NamedType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                    EmitCtx* ctx) {
  //
  auto currentMod = ctx->mod;
  auto mod        = ctx->mod;
  auto reqInfo    = ctx->get_access_info();
  if (relative != 0) {
    if (ctx->mod->has_nth_parent(relative)) {
      mod = ctx->mod->get_nth_parent(relative);
    } else {
      ctx->Error("The active scope does not have " + std::to_string(relative) + " parents", fileRange);
    }
  }
  if (names.size() > 1) {
    for (usize i = 0; i < (names.size() - 1); i++) {
      auto split = names.at(i);
      if (split.value == "std" && ir::StdLib::is_std_lib_found()) {
        mod = ir::StdLib::stdLib;
        continue;
      } else if (relative == 0 && i == 0 && mod->has_entity_with_name(split.value)) {
        ent->addDependency(
            ir::EntityDependency{mod->get_entity(split.value), expect.value_or(ir::DependType::complete), phase});
        break;
      }
      auto fullName = Identifier::fullName(names);
      SHOW("UpdateDeps :: " << fullName.value)
      if (mod->has_lib(split.value, reqInfo) || mod->has_brought_lib(split.value, reqInfo) ||
          mod->has_lib_in_imports(split.value, reqInfo).first) {
        SHOW("Has lib")
        mod = mod->get_lib(split.value, reqInfo);
      } else if (mod->has_brought_mod(split.value, reqInfo) ||
                 mod->has_brought_mod_in_imports(split.value, reqInfo).first) {
        SHOW("Has brought module")
        mod = mod->get_brought_mod(split.value, reqInfo);
      } else {
        SHOW("Update deps")
        ctx->Error("No lib named " + ctx->color(split.value) + " found inside " + ctx->color(mod->get_full_name()) +
                       " or any of its submodules",
                   split.range);
      }
    }
    SHOW("Mod retrieval complete")
  }
  auto entName = names.back();
  SHOW("Checking entity")
  if (mod->has_entity_with_name(entName.value)) {
    SHOW("Has entity with name")
    auto entVal = mod->get_entity(entName.value);
    if ((entVal->type == ir::EntityType::typeDefinition) && entVal->astNode) {
      typeSize = ((ast::TypeDefinition*)(entVal->astNode))->typeSize;
    }
    ent->addDependency(ir::EntityDependency{entVal, expect.value_or(ir::DependType::complete), phase});
  }
}

ir::Type* NamedType::emit(EmitCtx* ctx) {
  auto* mod        = ctx->mod;
  auto* currentMod = mod;
  SHOW("Getting access info")
  auto reqInfo = ctx->get_access_info();
  SHOW("Got req info")
  if (relative != 0) {
    if (ctx->mod->has_nth_parent(relative)) {
      mod = ctx->mod->get_nth_parent(relative);
    } else {
      ctx->Error("The active scope does not have " + std::to_string(relative) + " parents", fileRange);
    }
  }
  auto entityName = names.back();
  if (names.size() == 1) {
    SHOW("Name count 1 " << entityName.value)
    SHOW("Checking generic parameters")
    SHOW("Has Fn " << ctx->has_fn())
    SHOW("Has member parent " << ctx->has_member_parent())
    if (ctx->has_fn() && ctx->get_fn()->hasGenericParameter(entityName.value)) {
      SHOW("Checking function generic params")
      auto genParam = ctx->get_fn()->getGenericParameter(entityName.value);
      if (genParam->is_typed()) {
        return genParam->as_typed()->get_type();
      } else if (genParam->is_prerun()) {
        auto* exp = genParam->as_prerun()->get_expression();
        if (exp->get_ir_type()->is_typed()) {
          return exp->get_ir_type()->as_typed()->get_subtype();
        } else {
          ctx->Error("Generic parameter " + ctx->color(entityName.value) +
                         " is a normal prerun expression and hence cannot be used as a type",
                     fileRange);
        }
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    } else if (ctx->has_member_parent()) {
      SHOW("Checking member parent generic params")
      ir::GenericParameter* genVal = nullptr;
      if (ctx->get_member_parent()->is_expanded() && ctx->get_member_parent()->as_expanded()->is_generic() &&
          ctx->get_member_parent()->as_expanded()->has_generic_parameter(entityName.value)) {
        genVal = ctx->get_member_parent()->as_expanded()->get_generic_parameter(entityName.value);
      } else if (ctx->get_member_parent()->is_done_skill() && ctx->get_member_parent()->as_done_skill()->is_generic() &&
                 ctx->get_member_parent()->as_done_skill()->has_generic_parameter(entityName.value)) {
        genVal = ctx->get_member_parent()->as_done_skill()->get_generic_parameter(entityName.value);
      }
      if (genVal) {
        if (genVal->is_typed()) {
          return genVal->as_typed()->get_type();
        } else if (genVal->is_prerun()) {
          auto exp = genVal->as_prerun()->get_expression();
          if (exp->get_ir_type()->is_typed()) {
            return exp->get_ir_type()->as_typed()->get_subtype();
          } else {
            ctx->Error("The generic parameter " + ctx->color(entityName.value) + " is an expression of type " +
                           ctx->color(exp->get_ir_type()->to_string()) + ", and hence cannot be used as a type",
                       fileRange);
          }
        } else {
          ctx->Error("Invalid generic kind", genVal->get_range());
        }
      }
    }
    SHOW("Checked generic parameters")
  }
  if (names.size() > 1) {
    for (usize i = 0; i < (names.size() - 1); i++) {
      auto split = names.at(i);
      if (split.value == "std" && ir::StdLib::is_std_lib_found()) {
        mod = ir::StdLib::stdLib;
        continue;
      }
      if (mod->has_lib(split.value, reqInfo) || mod->has_brought_lib(split.value, reqInfo) ||
          mod->has_lib_in_imports(split.value, reqInfo).first) {
        SHOW("Has lib")
        mod = mod->get_lib(split.value, reqInfo);
        mod->add_mention(split.range);
      } else if (mod->has_brought_mod(split.value, ctx->get_access_info()) ||
                 mod->has_brought_mod_in_imports(split.value, reqInfo).first) {
        SHOW("Has brought module")
        mod = mod->get_brought_mod(split.value, reqInfo);
        mod->add_mention(split.range);
      } else {
        SHOW("Emit fn")
        ctx->Error("No lib named " + ctx->color(split.value) + " found inside " + ctx->color(mod->get_full_name()) +
                       " or any of its submodules",
                   split.range);
      }
    }
  }
  SHOW("Handled modules")
  SHOW("Mod is " << mod)
  SHOW("mod->hasOpaqueType " << mod->has_opaque_type(entityName.value, reqInfo))
  //   SHOW("Module name is " << mod->get_file_path())
  //   SHOW("Module is " << mod->get_name() << " at file: " << mod->get_file_path()
  //                     << " has submods: " << (mod->has_submodules() ? "true" : "false"))
  if (mod->has_opaque_type(entityName.value, reqInfo) || mod->has_brought_opaque_type(entityName.value, reqInfo) ||
      mod->has_opaque_type_in_imports(entityName.value, reqInfo).first) {
    SHOW("Has opaque")
    auto* oTy = mod->get_opaque_type(entityName.value, reqInfo);
    if (oTy->is_generic()) {
      ctx->Error(oTy->is_subtype_struct()
                     ? "Core type "
                     : (oTy->is_subtype_mix() ? "Mix type " : "Type ") + ctx->color(oTy->to_string()) +
                           " is a generic type and hence cannot be used as a normal type",
                 fileRange);
    }
    if (!oTy->get_visibility().is_accessible(reqInfo)) {
      ctx->Error((oTy->is_subtype_struct() ? "Core type "
                                           : (oTy->is_subtype_mix() ? "Mix type " : "Incomplete opaque type ")) +
                     ctx->color(oTy->get_full_name()) + " inside module " + ctx->color(mod->get_full_name()) +
                     " is not accessible here",
                 entityName.range);
    }
    oTy->add_mention(entityName.range);
    SHOW("Returning opaque type")
    return oTy;
  } else if (mod->has_struct_type(entityName.value, reqInfo) ||
             mod->has_brought_struct_type(entityName.value, ctx->get_access_info()) ||
             mod->has_struct_type_in_imports(entityName.value, reqInfo).first) {
    SHOW("Has struct")
    auto* cTy = mod->get_struct_type(entityName.value, reqInfo);
    if (!cTy->get_visibility().is_accessible(reqInfo)) {
      ctx->Error("Core type " + ctx->color(cTy->get_full_name()) + " inside module " +
                     ctx->color(mod->get_full_name()) + " is not accessible here",
                 entityName.range);
    }
    cTy->add_mention(entityName.range);
    SHOW("Added mention, returning struct " << entityName.value)
    return cTy;
  } else if (mod->has_type_definition(entityName.value, reqInfo) ||
             mod->has_brought_type_definition(entityName.value, ctx->get_access_info()) ||
             mod->has_type_definition_in_imports(entityName.value, reqInfo).first) {
    SHOW("Has type def")
    auto* dTy = mod->get_type_def(entityName.value, reqInfo);
    SHOW("Checking accessibility")
    if (!dTy->get_visibility().is_accessible(reqInfo)) {
      ctx->Error("Type definition " + ctx->color(dTy->get_full_name()) + " inside module " +
                     ctx->color(mod->get_full_name()) + " is not accessible here",
                 entityName.range);
    }
    SHOW("Adding mention")
    dTy->add_mention(entityName.range);
    SHOW("Returning type def " << entityName.value)
    return dTy;
  } else if (mod->has_mix_type(entityName.value, reqInfo) ||
             mod->has_brought_mix_type(entityName.value, ctx->get_access_info()) ||
             mod->has_mix_type_in_imports(entityName.value, reqInfo).first) {
    SHOW("Has mix type")
    auto* mTy = mod->get_mix_type(entityName.value, reqInfo);
    if (!mTy->get_visibility().is_accessible(reqInfo)) {
      ctx->Error("Mix type " + ctx->color(mTy->get_full_name()) + " inside module " + ctx->color(mod->get_full_name()) +
                     " is not accessible here",
                 entityName.range);
    }
    mTy->add_mention(entityName.range);
    return mTy;
  } else if (mod->has_choice_type(entityName.value, reqInfo) ||
             mod->has_brought_choice_type(entityName.value, ctx->get_access_info()) ||
             mod->has_choice_type_in_imports(entityName.value, reqInfo).first) {
    SHOW("Has choice type")
    auto* chTy = mod->get_choice_type(entityName.value, reqInfo);
    if (!chTy->get_visibility().is_accessible(reqInfo)) {
      ctx->Error("Choice type " + ctx->color(chTy->get_full_name()) + " inside module " +
                     ctx->color(mod->get_full_name()) + " is not accessible here",
                 entityName.range);
    }
    chTy->add_mention(entityName.range);
    return chTy;
  } else if (mod->has_region(entityName.value, reqInfo) ||
             mod->has_brought_region(entityName.value, ctx->get_access_info()) ||
             mod->has_region_in_imports(entityName.value, reqInfo).first) {
    SHOW("Has region")
    auto* reg = mod->get_region(entityName.value, reqInfo);
    if (!reg->get_visibility().is_accessible(reqInfo)) {
      ctx->Error("Region " + ctx->color(reg->get_full_name()) + " inside module " + ctx->color(mod->get_full_name()) +
                     " is not accessible here",
                 entityName.range);
    }
    reg->add_mention(entityName.range);
    return reg;
  } else {
    ctx->Error("No type named " + Identifier::fullName(names).value + " found in scope", fileRange);
  }
  return nullptr;
}

String NamedType::get_name() const { return Identifier::fullName(names).value; }

u32 NamedType::getRelative() const { return relative; }

AstTypeKind NamedType::type_kind() const { return AstTypeKind::FLOAT; }

Json NamedType::to_json() const {
  Vec<JsonValue> nameJs;
  for (auto const& nam : names) {
    nameJs.push_back(JsonValue(nam));
  }
  return Json()._("type_kind", "named")._("names", nameJs)._("fileRange", fileRange);
}

String NamedType::to_string() const { return Identifier::fullName(names).value; }

} // namespace qat::ast