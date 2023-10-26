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

  useit inline bool isTimes() const { return type == LoopType::nTimes; }
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

  useit inline bool moduleAlreadyHasErrors(IR::QatModule* cand) {
    for (auto* module : modulesWithErrors) {
      if (module->getID() == cand->getID()) {
        return true;
      }
    }
    return false;
  }

  void addError(const String& message, Maybe<FileRange> fileRange);

  Vec<std::tuple<String, u64, u64>> getContentForDiagnostics(FileRange const& _range) const;

  void printRelevantFileContent(FileRange const& fileRange, bool isError) const;

  QatSitter* sitter = nullptr;

  Context();

  // NOTE - Single instance for now
  static Context* instance;

public:
  static inline Context* New() {
    if (instance) {
      return instance;
    } else {
      instance = new Context();
      return instance;
    }
  }

  useit inline llvm::LLVMContext& getllCtx() {
    std::cout << "llctx(" << &llctx << ")\n";
    return (this->llctx);
  }

  llvm::LLVMContext       llctx;
  clang::TargetInfo*      clangTargetInfo;
  Maybe<llvm::DataLayout> dataLayout;
  IRBuilderTy             builder;
  QatModule*              activeModule   = nullptr;
  IR::Function*           activeFunction = nullptr;
  Vec<IR::QatType*>       activeTypes;
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

  useit inline bool          hasActiveFunction() const { return activeFunction != nullptr; }
  useit inline IR::Function* getActiveFunction() const { return activeFunction; }
  useit inline IR::Function* setActiveFunction(IR::Function* fun) {
    auto* oldFn    = activeFunction;
    activeFunction = fun;
    return oldFn;
  }

  useit inline bool           hasActiveModule() const { return activeModule != nullptr; }
  useit inline IR::QatModule* getActiveModule() const { return activeModule; }
  useit inline IR::QatModule* setActiveModule(IR::QatModule* module) {
    auto* oldMod = activeModule;
    activeModule = module;
    return oldMod;
  }
  useit inline QatModule* getMod() const { return getActiveModule()->getActive(); }

  useit inline bool         hasActiveType() const { return !activeTypes.empty(); }
  useit inline IR::QatType* getActiveType() const { return activeTypes.back(); }
  inline void               setActiveType(IR::QatType* typ) { activeTypes.push_back(typ); }
  inline void               unsetActiveType() { activeTypes.pop_back(); }

  useit inline bool                 hasActiveGeneric() const { return !allActiveGenerics.empty(); }
  useit inline GenericEntityMarker& getActiveGeneric() const { return allActiveGenerics.back(); }
  useit bool                        hasGenericParameterFromLastMain(String const& name) const;
  useit GenericParameter*           getGenericParameterFromLastMain(String const& name) const;

  inline void addActiveGeneric(GenericEntityMarker marker, bool main) {
    if (main) {
      lastMainActiveGeneric.push_back(allActiveGenerics.size());
    }
    allActiveGenerics.push_back(marker);
  }

  inline void removeActiveGeneric() {
    if ((!lastMainActiveGeneric.empty()) && (allActiveGenerics.size() - 1 == lastMainActiveGeneric.back())) {
      lastMainActiveGeneric.pop_back();
    }
    allActiveGenerics.pop_back();
  }

  useit inline String joinActiveGenericNames(bool highlight) const {
    String result;
    for (usize i = 0; i < allActiveGenerics.size(); i++) {
      result.append(highlight ? highlightError(allActiveGenerics.at(i).name) : allActiveGenerics.at(i).name);
      if (i < allActiveGenerics.size() - 1) {
        result.append(" and ");
      }
    }
    return result;
  }

  useit inline clang::LangAS getProgramAddressSpaceAsLangAS() const {
    if (dataLayout) {
      return clang::getLangASFromTargetAS(dataLayout->getProgramAddressSpace());
    } else {
      return clang::LangAS::Default;
    }
  }

  void nameCheckInModule(const Identifier& name, const String& entityType, Maybe<String> genericID);

  inline void genericNameCheck(const String& name, const FileRange& range) {
    if (hasActiveFunction() && getActiveFunction()->hasGenericParameter(name)) {
      Error("A generic parameter named " + highlightError(name) +
                " is present in this function. This will lead to ambiguity.",
            range);
    } else if (hasActiveType() &&
               ((getActiveType()->isExpanded() && getActiveType()->asExpanded()->hasGenericParameter(name)) ||
                (getActiveType()->isOpaque() && getActiveType()->asOpaque()->hasGenericParameter(name)))) {
      Error("A generic parameter named " + highlightError(name) + " is present in the parent type " +
                highlightError(getActiveType()->toString()) + " so this will lead to ambiguity",
            range);
    }
  }

  useit inline String getGlobalStringName() const {
    auto res = "qat'str'" + std::to_string(stringCount);
    stringCount++;
    return res;
  }

  useit AccessInfo getAccessInfo() const;

  useit inline Maybe<AccessInfo> getReqInfoIfDifferentModule(IR::QatModule* otherMod) const {
    if ((getMod()->getID() == otherMod->getID()) || getMod()->isParentModuleOf(otherMod)) {
      return None;
    } else {
      return getAccessInfo();
    }
  }

  useit VisibilityInfo getVisibInfo(Maybe<VisibilityKind> kind) const;
  void                 writeJsonResult(bool status) const;

  exitFn void   Error(const String& message, Maybe<FileRange> fileRange);
  void          Warning(const String& message, const FileRange& fileRange) const;
  static String highlightError(const String& message, const char* color = colors::bold::yellow);
  static String highlightWarning(const String& message, const char* color = colors::bold::yellow);
  ~Context() = default;
};

} // namespace qat::IR

#endif
