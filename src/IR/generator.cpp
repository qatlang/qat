#include "generator.hpp"

llvm::Function *qat::IR::Generator::createFunction(
    std::string name,
    std::vector<llvm::Type *> parameterTypes,
    llvm::Type *returnType,
    bool isVariableArguments,
    llvm::GlobalValue::LinkageTypes linkageType //
)
{
    return llvm::Function::Create(
        llvm::FunctionType::get(
            returnType,
            llvm::ArrayRef<llvm::Type *>(parameterTypes),
            isVariableArguments),
        linkageType,
        llvm::Twine(name),
        *modules.at(activeModuleIndex) //
    );
}