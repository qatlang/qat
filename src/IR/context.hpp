#ifndef QAT_IR_CONTEXT_HPP
#define QAT_IR_CONTEXT_HPP

#include "../cli/color.hpp"
#include "../utils/file_range.hpp"
#include "./qat_module.hpp"
#include "function.hpp"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <chrono>
#include <string>
#include <vector>

namespace qat::IR {

enum class LoopType {
  nTimes,
  While,
  over,
  infinite,
};

enum class TemplateEntityType {
  function,
  memberFunction,
  coreType,
  mixType,
  typeDefinition,
};

struct TemplateEntityMarker {
  String             name;
  TemplateEntityType type;
  utils::FileRange   fileRange;
  u64                warningCount = 0;
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
  utils::FileRange range;

public:
  CodeProblem(bool isError, String message, utils::FileRange range);
  operator Json() const;
};

class Context {
private:
  using IRBuilderTy = llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>;

public:
  Context();

  llvm::LLVMContext llctx;
  IRBuilderTy       builder;
  QatModule*        mod        = nullptr;
  IR::Function*     fn         = nullptr; // Active function
  llvm::Value*      selfVal    = nullptr;
  IR::CoreType*     activeType = nullptr; // Active core type
  Vec<LoopInfo*>    loopsInfo;
  Vec<Breakable*>   breakables;
  Vec<fs::path>     executablePaths;

  // META
  bool                                                 hasMain;
  mutable u64                                          stringCount;
  Vec<fs::path>                                        llvmOutputPaths;
  Vec<String>                                          nativeLibsToLink;
  mutable Maybe<TemplateEntityMarker>                  activeTemplate;
  mutable Vec<CodeProblem>                             codeProblems;
  mutable Maybe<std::chrono::steady_clock::time_point> qatStartTime;
  mutable Maybe<std::chrono::steady_clock::time_point> qatEndTime;
  mutable Maybe<std::chrono::steady_clock::time_point> clangLinkStartTime;
  mutable Maybe<std::chrono::steady_clock::time_point> clangLinkEndTime;

  useit QatModule* getMod() const; // Get the active IR module
  useit String     getGlobalStringName() const;
  useit utils::RequesterInfo getReqInfo() const;
  useit utils::VisibilityInfo getVisibInfo(Maybe<utils::VisibilityKind> kind) const;
  void                        writeJsonResult(bool status) const;

  void          Error(const String& message, const utils::FileRange& fileRange) const;
  void          Warning(const String& message, const utils::FileRange& fileRange) const;
  static String highlightError(const String& message, const char* color = colors::yellow);
  static String highlightWarning(const String& message, const char* color = colors::yellow);
  ~Context();
};

} // namespace qat::IR

#endif