#include "./visibility.hpp"

namespace qat {
namespace utils {

#define VISIB_KIND "visibility_kind"
#define VISIB_VALUE "visibility_value"

bool RequesterInfo::has_access_to_box(const std::string other) const {
  return box.has_value() ? ((other == box.value()) ||
                            (other.find(box.value()) != std::string::npos))
                         : false;
}

bool RequesterInfo::has_access_to_lib(const std::string other) const {
  return lib.has_value() ? ((other == lib.value()) ||
                            (other.find(lib.value()) != std::string::npos))
                         : false;
}

bool RequesterInfo::is_same_file(const std::string other) const {
  return file == other;
}

bool RequesterInfo::is_same_type(const std::string other) const {
  return type.has_value() ? (type.value() == other) : false;
}

const std::map<VisibilityKind, std::string> Visibility::kind_value_map = {
    {VisibilityKind::pub, "public"},
    {VisibilityKind::type, "type"},
    {VisibilityKind::lib, "library"},
    {VisibilityKind::file, "file"},
    {VisibilityKind::box, "box"}};

const std::map<std::string, VisibilityKind> Visibility::value_kind_map = {
    {"public", VisibilityKind::pub},
    {"type", VisibilityKind::type},
    {"library", VisibilityKind::lib},
    {"file", VisibilityKind::file},
    {"box", VisibilityKind::box}};

llvm::GlobalValue::LinkageTypes
Visibility::get_linkage(const VisibilityKind kind) {
  switch (kind) {
  case VisibilityKind::pub:
  case VisibilityKind::file:
  case VisibilityKind::folder:
  case VisibilityKind::box: {
    return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
  }
  case VisibilityKind::lib:
  case VisibilityKind::type: {
    return llvm::GlobalValue::LinkageTypes::PrivateLinkage;
  }
  }
}

llvm::GlobalValue::VisibilityTypes
Visibility::get_llvm_visibility(const VisibilityKind kind) {
  switch (kind) {
  case VisibilityKind::pub:
  case VisibilityKind::file:
  case VisibilityKind::folder:
  case VisibilityKind::box: {
    return llvm::GlobalValue::VisibilityTypes::DefaultVisibility;
  }
  case VisibilityKind::lib:
  case VisibilityKind::type: {
    return llvm::GlobalValue::VisibilityTypes::HiddenVisibility;
  }
  }
}

void Visibility::set(llvm::GlobalVariable *global_var,
                     const VisibilityInfo access_info,
                     llvm::LLVMContext &context) {
  global_var->setLinkage(get_linkage(access_info.kind));
  global_var->setMetadata(
      VISIB_KIND,
      llvm::MDNode::get(
          context,
          llvm::MDString::get(context, kind_value_map.at(access_info.kind))));
  global_var->setMetadata(
      VISIB_VALUE, llvm::MDNode::get(context, llvm::MDString::get(
                                                  context, access_info.value)));
}

void Visibility::set(llvm::Function *function, const VisibilityInfo access_info,
                     llvm::LLVMContext &context) {
  function->setLinkage(get_linkage(access_info.kind));
  function->setMetadata(
      VISIB_KIND,
      llvm::MDNode::get(
          context,
          llvm::MDString::get(context, kind_value_map.at(access_info.kind))));
  function->setMetadata(
      VISIB_VALUE, llvm::MDNode::get(context, llvm::MDString::get(
                                                  context, access_info.value)));
}

void Visibility::set(llvm::Module *mod, const VisibilityInfo access_info,
                     llvm::LLVMContext &context) {

  auto kind_mdnode = llvm::MDNode::get(
      context,
      llvm::MDString::get(context, kind_value_map.at(access_info.kind)));
  auto kind_named_md = mod->getOrInsertNamedMetadata(VISIB_KIND);
  kind_named_md->addOperand(kind_mdnode);
  auto value_mdnode = llvm::MDNode::get(
      context, llvm::MDString::get(context, access_info.value));
  auto value_named_md = mod->getOrInsertNamedMetadata(VISIB_VALUE);
  value_named_md->addOperand(value_mdnode);
}

void Visibility::set(std::string name, std::string type,
                     const VisibilityInfo visibility_info,
                     llvm::LLVMContext &context) {
  auto name_mdnode =
      llvm::MDNode::get(context, llvm::MDString::get(context, name));
  // FIXME
}

VisibilityKind Visibility::get_kind(llvm::GlobalVariable *var) {
  auto kind = llvm::dyn_cast<llvm::MDString>(
                  var->getMetadata(VISIB_KIND)->getOperand(0))
                  ->getString()
                  .str();
  return value_kind_map.at(kind);
}

VisibilityKind Visibility::get_kind(llvm::Function *func) {
  auto kind = llvm::dyn_cast<llvm::MDString>(
                  func->getMetadata(VISIB_KIND)->getOperand(0))
                  ->getString()
                  .str();
  return value_kind_map.at(kind);
}

VisibilityKind Visibility::get_kind(llvm::Module *mod) {
  auto kind = llvm::dyn_cast<llvm::MDString>(
                  mod->getNamedMetadata(VISIB_KIND)->getOperand(0))
                  ->getString()
                  .str();
  return value_kind_map.at(kind);
}

VisibilityKind Visibility::get_kind(std::string name) {
  // FIXME
}

std::string Visibility::get_value(llvm::GlobalVariable *var) {
  return llvm::dyn_cast<llvm::MDString>(
             var->getMetadata(VISIB_VALUE)->getOperand(0))
      ->getString()
      .str();
}

std::string Visibility::get_value(llvm::Function *func) {
  return llvm::dyn_cast<llvm::MDString>(
             func->getMetadata(VISIB_VALUE)->getOperand(0))
      ->getString()
      .str();
}

std::string Visibility::get_value(llvm::Module *mod) {
  return llvm::dyn_cast<llvm::MDString>(
             mod->getNamedMetadata(VISIB_VALUE)->getOperand(0))
      ->getString()
      .str();
}

std::string Visibility::get_value(VisibilityKind kind) {
  return kind_value_map.find(kind)->second;
}

bool Visibility::isAccessible(const VisibilityInfo visibility,
                              const RequesterInfo req_info) {
  switch (visibility.kind) {
  case VisibilityKind::box: {
    if (req_info.box.has_value()) {
      return (req_info.box.value().find(visibility.value) == 0);
    } else {
      return false;
    }
  }
  case VisibilityKind::file: {
    return (visibility.value == req_info.file);
  }
  case VisibilityKind::folder: {
    return (req_info.file.find(visibility.value) == 0);
  }
  case VisibilityKind::lib: {
    if (req_info.lib.has_value()) {
      return (req_info.lib.value().find(visibility.value) == 0);
    } else {
      return false;
    }
  }
  case VisibilityKind::pub: {
    return true;
  }
  case VisibilityKind::type: {
    if (req_info.type.has_value()) {
      return (req_info.type.value().find(visibility.value) == 0);
    } else {
      return false;
    }
  }
  }
}

bool VisibilityInfo::isAccessible(const RequesterInfo &reqInfo) const {
  return Visibility::isAccessible(*this, reqInfo);
}

bool VisibilityInfo::operator==(VisibilityInfo other) const {
  return (kind == other.kind) && (value == other.value);
}

} // namespace utils
} // namespace qat