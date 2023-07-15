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

String memberFnTypeToString(MemberFnType type);

class ExpandedType;

/**
 *  MemberFunction represents a member function for a core type in
 * the language. It can be static or non-static
 *
 */
class MemberFunction : public Function {
private:
  ExpandedType* parent;
  bool          isStatic;
  bool          isVariation;
  MemberFnType  fnType;
  Identifier    selfName;

  MemberFunction(MemberFnType fnType, bool _isVariation, ExpandedType* _parent, const Identifier& _name,
                 QatType* returnType, bool _is_async, Vec<Argument> _args, bool has_variadic_arguments, bool _is_static,
                 const FileRange& _fileRange, const utils::VisibilityInfo& _visibility_info, llvm::LLVMContext& ctx);

public:
  static MemberFunction* Create(ExpandedType* parent, bool is_variation, const Identifier& name, QatType* return_type,
                                bool is_async, const Vec<Argument>& args, bool has_variadic_args,
                                const FileRange& fileRange, const utils::VisibilityInfo& visib_info,
                                llvm::LLVMContext& ctx);

  static MemberFunction* DefaultConstructor(ExpandedType* parent, FileRange nameRange,
                                            const utils::VisibilityInfo& visibInfo, FileRange fileRange,
                                            llvm::LLVMContext& ctx);

  static MemberFunction* CopyConstructor(ExpandedType* parent, FileRange nameRange, const Identifier& otherName,
                                         const FileRange& fileRange, llvm::LLVMContext& ctx);

  static MemberFunction* MoveConstructor(ExpandedType* parent, FileRange nameRange, const Identifier& otherName,
                                         const FileRange& fileRange, llvm::LLVMContext& ctx);

  static MemberFunction* CopyAssignment(ExpandedType* parent, FileRange nameRange, const Identifier& otherName,
                                        const FileRange& fileRange, llvm::LLVMContext& ctx);

  static MemberFunction* MoveAssignment(ExpandedType* parent, FileRange nameRange, const Identifier& otherName,
                                        const FileRange& fileRange, llvm::LLVMContext& ctx);

  static MemberFunction* CreateConstructor(ExpandedType* parent, FileRange nameRange, const Vec<Argument>& args,
                                           bool hasVariadicArgs, const FileRange& fileRange,
                                           const utils::VisibilityInfo& visibInfo, llvm::LLVMContext& ctx);

  static MemberFunction* CreateFromConvertor(ExpandedType* parent, FileRange nameRange, QatType* sourceType,
                                             const Identifier& name, const FileRange& fileRange,
                                             const utils::VisibilityInfo& visibInfo, llvm::LLVMContext& ctx);

  static MemberFunction* CreateToConvertor(ExpandedType* parent, FileRange nameRange, QatType* destType,
                                           const FileRange& fileRange, const utils::VisibilityInfo& visibInfo,
                                           llvm::LLVMContext& ctx);

  static MemberFunction* CreateDestructor(ExpandedType* parent, FileRange nameRange, const FileRange& fileRange,
                                          llvm::LLVMContext& ctx);

  static MemberFunction* CreateOperator(ExpandedType* parent, FileRange nameRange, bool isBinary, bool isVariationFn,
                                        const String& opr, IR::QatType* returnType, const Vec<Argument>& args,
                                        const FileRange& fileRange, const utils::VisibilityInfo& visibInfo,
                                        llvm::LLVMContext& ctx);

  static MemberFunction* CreateStatic(ExpandedType* parent, const Identifier& name, QatType* return_type, bool is_async,
                                      const Vec<Argument>& args, bool has_variadic_args, const FileRange& fileRange,
                                      const utils::VisibilityInfo& visib_info, llvm::LLVMContext& ctx);

  ~MemberFunction() override;

  useit Identifier    getName() const final;
  useit MemberFnType  getMemberFnType();
  useit bool          isVariationFunction() const;
  useit bool          isStaticFunction() const;
  useit String        getFullName() const final;
  useit bool          isMemberFunction() const final;
  useit ExpandedType* getParentType();
  void                updateOverview() final;
  useit Json          toJson() const;
};

} // namespace qat::IR

#endif