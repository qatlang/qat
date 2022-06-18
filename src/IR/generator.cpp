#include "generator.hpp"
namespace qat {
namespace IR {

llvm::Function *
Generator::create_function(const std::string name,
                           const std::vector<llvm::Type *> parameterTypes,
                           llvm::Type *returnType, bool isVariableArguments,
                           const llvm::GlobalValue::LinkageTypes linkageType //
) {
  return llvm::Function::Create(
      llvm::FunctionType::get(returnType,
                              llvm::ArrayRef<llvm::Type *>(parameterTypes),
                              isVariableArguments),
      linkageType, llvm::Twine(name),
      unit->llvmModule //
  );
}

llvm::GlobalVariable *
Generator::create_global_variable(const std::string name, llvm::Type *type,
                                  llvm::Value *value, const bool is_constant,
                                  const utils::VisibilityInfo access_info) {
  auto global = new llvm::GlobalVariable(
      *unit->llvmModule, type, is_constant,
      llvm::GlobalValue::LinkageTypes::ExternalLinkage,
      llvm::dyn_cast<llvm::Constant>(value), name, nullptr,
      llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, llvm::None, false);
  utils::Visibility::set(global, access_info, llvmContext);
}

llvm::Function *Generator::get_function(const std::string name) {
  return unit->llvmModule->getFunction(name);
}

llvm::GlobalVariable *Generator::get_global_variable(const std::string name) {
  return unit->llvmModule->getGlobalVariable(name);
}

bool Generator::does_function_exist(const std::string name) {
  return get_function(name) != nullptr;
}

std::string Generator::get_name(std::string item) { return name_prefix + item; }

bool Generator::has_type(const std::string name, const bool full_name) {
  auto types_md = unit->llvmModule->getOrInsertNamedMetadata("types");
  for (std::size_t i = 0; i < types_md->getNumOperands(); i++) {
    auto value = llvm::dyn_cast<llvm::MDString>(types_md->getOperand(i))
                     ->getString()
                     .str();
    return full_name ? (value == name) : (value == (name_prefix + name));
  }
}

Generator::AddResult Generator::add_type(const std::string self_name) {
  if (has_lib(self_name, false)) {
    return AddResult::False("lib");
  }
  if (has_box(self_name, false)) {
    return AddResult::False("box");
  }
  if (has_type(self_name, false)) {
    return AddResult::False("type");
  } else {
    auto box_mdnode = llvm::MDNode::get(
        llvmContext, llvm::MDString::get(llvmContext, name_prefix + self_name));
    auto boxes_md = unit->llvmModule->getOrInsertNamedMetadata("types");
    boxes_md->addOperand(box_mdnode);
    name_prefix += (self_name + ":");
    return AddResult::True();
  }
}

bool Generator::has_box(const std::string name, const bool full_name) {
  auto boxes_md = unit->llvmModule->getOrInsertNamedMetadata("boxes");
  for (std::size_t i = 0; i < boxes_md->getNumOperands(); i++) {
    auto value = llvm::dyn_cast<llvm::MDString>(boxes_md->getOperand(i))
                     ->getString()
                     .str();
    return full_name ? (value == name) : (value == (name_prefix + name));
  }
  return false;
}

Generator::AddResult Generator::add_box(const std::string self_name) {
  if (has_lib(self_name, false)) {
    return AddResult::False("lib");
  }
  if (has_type(self_name, false)) {
    return AddResult::False("type");
  }
  if (!has_box(self_name, false)) {
    auto box_mdnode = llvm::MDNode::get(
        llvmContext, llvm::MDString::get(llvmContext, name_prefix + self_name));
    auto boxes_md = unit->llvmModule->getOrInsertNamedMetadata("boxes");
    boxes_md->addOperand(box_mdnode);
    name_prefix += (self_name + ":");
  }
  return AddResult::True();
}

void Generator::close_box() {
  auto pos = 0;
  for (std::size_t i = 0; i < (name_prefix.length() - 1); i++) {
    if (name_prefix.at(i) == ':') {
      pos = i;
    }
  }
  name_prefix = name_prefix.substr(0, pos);
}

bool Generator::has_lib(const std::string name, const bool full_name) {
  auto libs_md = unit->llvmModule->getOrInsertNamedMetadata("libs");
  for (std::size_t i = 0; i < libs_md->getNumOperands(); i++) {
    auto value = llvm::dyn_cast<llvm::MDString>(libs_md->getOperand(i))
                     ->getString()
                     .str();
    if (full_name ? (value == name) : (value == (name_prefix + name))) {
      return true;
    }
  }
  return false;
}

Generator::AddResult Generator::add_lib(const std::string self_name) {
  if (has_type(self_name, false)) {
    return AddResult::False("type");
  }
  if (has_box(self_name, false)) {
    return AddResult::False("box");
  }
  if (has_lib(self_name, false)) {
    return AddResult::False("lib");
  } else {
    auto box_mdnode = llvm::MDNode::get(
        llvmContext, llvm::MDString::get(llvmContext, name_prefix + self_name));
    auto boxes_md = unit->llvmModule->getOrInsertNamedMetadata("libs");
    boxes_md->addOperand(box_mdnode);
    name_prefix += (self_name + ":");
    return AddResult::True();
  }
}

void Generator::close_lib() {
  auto pos = name_prefix.find_last_of(':');
  name_prefix = name_prefix.substr(0, pos);
}

void Generator::throw_error(const std::string message,
                            const utils::FilePlacement placement) {
  std::cout << colors::red << "[ COMPILER ERROR ] " << colors::bold::green
            << placement.file.string() << ":" << placement.start.line << ":"
            << placement.start.character << " - " << placement.end.line << ":"
            << placement.end.character << colors::reset << "\n"
            << "   " << message << "\n";
  exit(0);
}

} // namespace IR
} // namespace qat