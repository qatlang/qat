#include "./entity.hpp"

namespace qat {
namespace AST {

llvm::Value *Entity::generate(IR::Generator *generator) {
  auto func = generator->builder.GetInsertBlock()->getParent();
  auto &e_block = func->getEntryBlock();
  auto insert_p = e_block.begin();
  llvm::AllocaInst *var_alloca = nullptr;
  llvm::Argument *arg_val = nullptr;
  llvm::LoadInst *var_load = nullptr;
  for (auto &instruction : e_block) {
    if (llvm::isa<llvm::AllocaInst>(instruction)) {
      auto temp_alloc = llvm::dyn_cast<llvm::AllocaInst>(&instruction);
      if (temp_alloc->hasName()) {
        auto var_name = temp_alloc->getName();
        if (var_name.str() == name) {
          var_alloca = temp_alloc;
          break;
        }
      }
    } else {
      break;
    }
  }
  if (var_alloca) {
    if (isExpectedKind(ExpressionKind::assignable)) {
      if (utils::PointerKind::is_reference(var_alloca)) {
        auto result = generator->builder.CreateLoad(var_alloca, name);
        utils::Variability::propagate(generator->llvmContext, var_alloca,
                                      result);
        return result;
      } else {
        return var_alloca;
      }
    } else {
      var_load = generator->builder.CreateLoad(var_alloca->getType(),
                                               var_alloca, false, name);
      if (utils::PointerKind::is_reference(var_alloca)) {
        var_load = generator->builder.CreateLoad(var_load->getType(), var_load,
                                                 false, name);
      }
      utils::Variability::propagate(generator->llvmContext, var_alloca,
                                    var_load);
      return var_load;
    }
  } else {
    for (std::size_t i = 0; i < func->arg_size(); i++) {
      auto candidate = func->getArg(i);
      if (candidate->getName().str() == name) {
        arg_val = candidate;
        break;
      }
    }
    if (arg_val) {
      if (isExpectedKind(ExpressionKind::assignable)) {
        if (utils::PointerKind::is_reference(arg_val)) {
          auto result = generator->builder.CreateLoad(arg_val, name);
          utils::Variability::propagate(generator->llvmContext, arg_val,
                                        result);
          return result;
        } else {
          return arg_val;
        }
      } else {
        var_load = generator->builder.CreateLoad(arg_val, name);
        if (utils::PointerKind::is_reference(arg_val)) {
          var_load = generator->builder.CreateLoad(var_load->getType(),
                                                   var_load, false, name);
        }
        utils::Variability::propagate(generator->llvmContext, arg_val,
                                      var_load);
        return var_load;
      }
    } else {
      auto globalVariable = generator->get_global_variable(name);
      if (globalVariable) {
        if (isExpectedKind(ExpressionKind::assignable)) {
          if (utils::PointerKind::is_reference(globalVariable)) {
            auto result = generator->builder.CreateLoad(globalVariable, name);
            utils::Variability::propagate(generator->llvmContext,
                                          globalVariable, result);
            return result;
          } else {
            return globalVariable;
          }
        } else {
          var_load = generator->builder.CreateLoad(globalVariable->getType(),
                                                   globalVariable, false, name);
          if (utils::PointerKind::is_reference(globalVariable)) {
            var_load = generator->builder.CreateLoad(var_load->getType(),
                                                     var_load, false, name);
          }
          utils::Variability::set(generator->llvmContext, var_load,
                                  globalVariable->isConstant());
          return var_load;
        }
      } else {
        for (std::size_t i = 0; i < generator->exposed.size(); i++) {
          auto space_name = generator->exposed.at(i).generate() + name;
          globalVariable = generator->get_global_variable(space_name);
          if (globalVariable) {
            if (isExpectedKind(ExpressionKind::assignable)) {
              if (utils::PointerKind::is_reference(globalVariable)) {
                auto result =
                    generator->builder.CreateLoad(globalVariable, space_name);
                utils::Variability::propagate(generator->llvmContext,
                                              globalVariable, result);
                return result;
              } else {
                return globalVariable;
              }
            } else {
              var_load = generator->builder.CreateLoad(
                  globalVariable->getValueType(), globalVariable, false,
                  llvm::Twine(space_name));
              utils::Variability::set(generator->llvmContext, var_load,
                                      globalVariable->isConstant());
              return var_load;
            }
          }
        }
      }

      generator->throw_error("Entity `" + name +
                                 "` cannot be found in function `" +
                                 e_block.getParent()->getName().str() +
                                 "` and is not a global or static value",
                             file_placement);
    }
  }
}

} // namespace AST
} // namespace qat