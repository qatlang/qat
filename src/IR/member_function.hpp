#ifndef QAT_IR_MEMBER_FUNCTION_HPP
#define QAT_IR_MEMBER_FUNCTION_HPP

#include "../utils/visibility.hpp"
#include "./argument.hpp"
#include "./function.hpp"
#include "types/qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

#include <string>
#include <vector>

namespace qat::IR {

enum class MemberFnType {
  normal,
  staticFn,
  fromConvertor,
  toConvertor,
  constructor,
  copyConstructor,
  moveConstructor,
  copyAssignment,
  moveAssignment,
  destructor,
  binaryOperator,
  unaryOperator,
  defaultConstructor,
};

class CoreType;

/**
 *  MemberFunction represents a member function for a core type in
 * the language. It can be static or non-static
 *
 */
class MemberFunction : public Function {
private:
  CoreType    *parent;
  bool         isStatic;
  bool         isVariation;
  MemberFnType fnType;
  String       selfName;

  MemberFunction(MemberFnType fnType, bool _isVariation, CoreType *_parent,
                 const String &_name, QatType *returnType,
                 bool isReturnTypeVariable, bool _is_async, Vec<Argument> _args,
                 bool has_variadic_arguments, bool _is_static,
                 const utils::FileRange      &_fileRange,
                 const utils::VisibilityInfo &_visibility_info,
                 llvm::LLVMContext           &ctx);

public:
  static MemberFunction *
  Create(CoreType *parent, bool is_variation, const String &name,
         QatType *return_type, bool isReturnTypeVariable, bool is_async,
         const Vec<Argument> &args, bool has_variadic_args,
         const utils::FileRange      &fileRange,
         const utils::VisibilityInfo &visib_info, llvm::LLVMContext &ctx);

  static MemberFunction *
  DefaultConstructor(CoreType *parent, const utils::VisibilityInfo &visibInfo,
                     utils::FileRange fileRange, llvm::LLVMContext &ctx);

  static MemberFunction *CopyConstructor(CoreType               *parent,
                                         const String           &otherName,
                                         const utils::FileRange &fileRange,
                                         llvm::LLVMContext      &ctx);

  static MemberFunction *MoveConstructor(CoreType               *parent,
                                         const String           &otherName,
                                         const utils::FileRange &fileRange,
                                         llvm::LLVMContext      &ctx);

  static MemberFunction *CopyAssignment(CoreType               *parent,
                                        const String           &otherName,
                                        const utils::FileRange &fileRange,
                                        llvm::LLVMContext      &ctx);

  static MemberFunction *MoveAssignment(CoreType               *parent,
                                        const String           &otherName,
                                        const utils::FileRange &fileRange,
                                        llvm::LLVMContext      &ctx);

  static MemberFunction *
  CreateConstructor(CoreType *parent, const Vec<Argument> &args,
                    bool hasVariadicArgs, const utils::FileRange &fileRange,
                    const utils::VisibilityInfo &visibInfo,
                    llvm::LLVMContext           &ctx);

  static MemberFunction *
  CreateFromConvertor(CoreType *parent, QatType *sourceType, const String &name,
                      const utils::FileRange      &fileRange,
                      const utils::VisibilityInfo &visibInfo,
                      llvm::LLVMContext           &ctx);

  static MemberFunction *CreateToConvertor(
      CoreType *parent, QatType *destType, const utils::FileRange &fileRange,
      const utils::VisibilityInfo &visibInfo, llvm::LLVMContext &ctx);

  static MemberFunction *CreateDestructor(CoreType               *parent,
                                          const utils::FileRange &fileRange,
                                          llvm::LLVMContext      &ctx);

  static MemberFunction *CreateOperator(CoreType *parent, bool isBinary,
                                        bool isVariationFn, const String &opr,
                                        IR::QatType                 *returnType,
                                        const Vec<Argument>         &args,
                                        const utils::FileRange      &fileRange,
                                        const utils::VisibilityInfo &visibInfo,
                                        llvm::LLVMContext           &ctx);

  static MemberFunction *
  CreateStatic(CoreType *parent, const String &name, QatType *return_type,
               bool is_return_type_variable, bool is_async,
               const Vec<Argument> &args, bool has_variadic_args,
               const utils::FileRange      &fileRange,
               const utils::VisibilityInfo &visib_info, llvm::LLVMContext &ctx);

  useit String       getName() const final;
  useit MemberFnType getMemberFnType();
  useit bool         isVariationFunction() const;
  useit bool         isStaticFunction() const;
  useit String       getFullName() const final;
  useit bool         isMemberFunction() const final;
  useit CoreType    *getParentType();
  useit Json         toJson() const;
  void               emitCPP(cpp::File &file) const;
};

} // namespace qat::IR

#endif