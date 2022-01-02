#ifndef QAT_IR_GENERATOR_HPP
#define QAT_IR_GENERATOR_HPP

#include <vector>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

namespace qat
{
    namespace IR
    {
        /**
         * @brief The IR Generator for QAT
         */
        class Generator
        {
        private:
            /**
             * @brief Module is a Top level container of all LLVM IR objects
             * Multiple modules can represent multiple libraries
             */
            std::vector<std::unique_ptr<llvm::Module>> modules;
            std::size_t activeModuleIndex = 0;

        public:
            /**
             * @brief The LLVMContext determines the context and scope of all
             * instructions, functions, blocks and variables
             */
            llvm::LLVMContext llvmContext = llvm::LLVMContext();

            /**
             * @brief Create a Function object pointer. This is not accompanying Function AST as
             * the module is a private variable
             * 
             * @param name Name of the function
             * @param parameterTypes LLVM Type of the parameters of the function
             * @param returnType LLVM Type of the return value of the function
             * @param isVariableArguments Whether the function has variable number of arguments
             * @param linkageType How to link the function symbol
             * @return llvm::Function* 
             */
            llvm::Function *createFunction(
                std::string name,
                std::vector<llvm::Type *> parameterTypes,
                llvm::Type *returnType,
                bool isVariableArguments,
                llvm::GlobalValue::LinkageTypes linkageType);

            /**
             * @brief Provided of Basic API to create instructions and then append
             * it to the end of a BasicBlock or to a specified location  
             */
            std::unique_ptr<llvm::IRBuilder<>> builder = std::make_unique<llvm::IRBuilder<>>(
                llvm::IRBuilder(
                    llvmContext,
                    llvm::ConstantFolder(),
                    llvm::IRBuilderDefaultInserter(),
                    nullptr,
                    llvm::None) //
            );
        };
    }
}

#endif