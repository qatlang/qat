#ifndef QAT_IR_CONTEXT_HPP
#define QAT_IR_CONTEXT_HPP

#include "../cli/color.hpp"
#include "../utils/file_range.hpp"
#include "./qat_module.hpp"
#include "function.hpp"
#include "clang/Basic/AddressSpaces.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <chrono>
#include <string>
#include <vector>

using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

namespace qat {

class QatSitter;

} // namespace qat

namespace qat::IR {

enum class LoopType {
  nTimes,
  While,
  over,
  infinite,
};

enum class GenericEntityType {
  function,
  memberFunction,
  coreType,
  mixType,
  typeDefinition,
};

struct GenericEntityMarker {
  String                     name;
  GenericEntityType          type;
  FileRange                  fileRange;
  u64                        warningCount = 0;
  Vec<IR::GenericParameter*> generics{};

  bool hasGenericParameter(const String& name) const {
    for (auto* gen : generics) {
      if (gen->isSame(name)) {
        return true;
      }
    }
    return false;
  }

  GenericParameter* getGenericParameter(const String& name) const {
    for (auto* gen : generics) {
      if (gen->isSame(name)) {
        return gen;
      }
    }
    return nullptr;
  }
};

class LoopInfo {
public:
  LoopInfo(String _name, IR::Block* _mainB, IR::Block* _condB, IR::Block* _restB, IR::LocalValue* _index,
           LoopType _type);

  ~LoopInfo() = default;

  String          name;
  IR::Block*      mainBlock;
  IR::Block*      condBlock;
  IR::Block*      restBlock;
  IR::LocalValue* index;
  LoopType        type;

  useit bool isTimes() const;
};

enum class BreakableType {
  loop,
  match,
};

class Breakable {
public:
  Breakable(Maybe<String> _tag, IR::Block* _restBlock, IR::Block* _trueBlock);

  ~Breakable() = default;

  Maybe<String> tag;
  IR::Block*    restBlock;
  IR::Block*    trueBlock;
};

class CodeProblem {
  bool             isError;
  String           message;
  Maybe<FileRange> range;

public:
  CodeProblem(bool isError, String message, Maybe<FileRange> range);
  operator Json() const;
};

class Context {
  friend class qat::QatSitter;

private:
  using IRBuilderTy = llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>;

  Vec<IR::QatModule*> modulesWithErrors;
  bool                moduleAlreadyHasErrors(IR::QatModule* mod);
  void                addError(const String& message, Maybe<FileRange> fileRange);

  Vec<std::tuple<String, u64, u64>> getContentForDiagnostics(FileRange const& _range) const;

  void printRelevantFileContent(FileRange const& fileRange, bool isError) const;

  QatSitter* sitter = nullptr;

  Context();

  // NOTE - Single instance for now
  static Context* instance;

public:
  static Context* New();

  llvm::LLVMContext& getllCtx();

  llvm::LLVMContext       llctx;
  clang::TargetInfo*      clangTargetInfo;
  Maybe<llvm::DataLayout> dataLayout;
  IRBuilderTy             builder;
  QatModule*              mod        = nullptr;
  IR::Function*           fn         = nullptr; // Active function
  IR::ExpandedType*       activeType = nullptr; // Active core type
  Vec<LoopInfo>           loopsInfo;
  Vec<Breakable>          breakables;
  Vec<fs::path>           executablePaths;

  // META
  bool                             hasMain;
  mutable u64                      stringCount;
  Vec<fs::path>                    llvmOutputPaths;
  Vec<String>                      nativeLibsToLink;
  mutable Vec<GenericEntityMarker> allActiveGenerics;
  mutable Vec<usize>               lastMainActiveGeneric;
  mutable Vec<CodeProblem>         codeProblems;
  mutable Vec<usize>               binarySizes;
  mutable Maybe<HighResTimePoint>  qatStartTime;
  mutable Maybe<HighResTimePoint>  qatEndTime;
  mutable Maybe<HighResTimePoint>  clangLinkStartTime;
  mutable Maybe<HighResTimePoint>  clangLinkEndTime;

  useit bool                 hasActiveGeneric() const;
  useit GenericEntityMarker& getActiveGeneric() const;
  useit bool                 hasGenericParameterFromLastMain(String const& name) const;
  useit GenericParameter*    getGenericParameterFromLastMain(String const& name) const;
  void                       addActiveGeneric(GenericEntityMarker marker, bool main);
  void                       removeActiveGeneric();
  String                     joinActiveGenericNames(bool highlight) const;
  useit clang::LangAS getProgramAddressSpaceAsLangAS() const;
  void                nameCheckInModule(const Identifier& name, const String& entityType, Maybe<String> genericID);
  void                genericNameCheck(const String& name, const FileRange& range);
  useit QatModule*    getMod() const; // Get the active IR module
  useit String        getGlobalStringName() const;
  useit AccessInfo    getAccessInfo() const;
  useit Maybe<AccessInfo> getReqInfoIfDifferentModule(IR::QatModule* otherMod) const;
  useit VisibilityInfo    getVisibInfo(Maybe<VisibilityKind> kind) const;
  void                    writeJsonResult(bool status) const;

  exitFn void   Error(const String& message, Maybe<FileRange> fileRange);
  void          Warning(const String& message, const FileRange& fileRange) const;
  static String highlightError(const String& message, const char* color = colors::bold::yellow);
  static String highlightWarning(const String& message, const char* color = colors::bold::yellow);
  ~Context() = default;
};

} // namespace qat::IR

#endif
