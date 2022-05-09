#include "./global_declaration.hpp"

qat::AST::GlobalDeclaration::GlobalDeclaration(
    std::string _name, llvm::Optional<Space> _parentSpace,
    llvm::Optional<QatType> _type, Expression _value, bool _isVariable,
    utilities::FilePlacement _filePlacement)
    : name(_name), parent_space(_parentSpace), type(_type), value(_value),
      is_variable(_isVariable), Node(_filePlacement) {}

llvm::Value *
qat::AST::GlobalDeclaration::generate(qat::IR::Generator *generator) {
  auto gen_value = value.generate(generator);
  return new llvm::GlobalVariable(
      (type.hasValue() ? type.getValue().generate(generator)
                       : gen_value->getType()),
      !is_variable, llvm::GlobalValue::LinkageTypes::CommonLinkage,
      llvm::dyn_cast<llvm::Constant>(gen_value),
      llvm::Twine(parent_space.hasValue()
                      ? (parent_space.getValue().generate() + name)
                      : name),
      llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, 0, false);
}

qat::AST::NodeType qat::AST::GlobalDeclaration::nodeType() {
  return qat::AST::NodeType::globalDeclaration;
}