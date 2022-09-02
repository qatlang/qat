#ifndef QAT_IR_CONTEXT_HPP
#define QAT_IR_CONTEXT_HPP

#include "../cli/color.hpp"
#include "../utils/file_range.hpp"
#include "./qat_module.hpp"
#include "function.hpp"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <string>
#include <vector>

namespace qat::IR {

enum class LoopType {
  nTimes,
  While,
  over,
  infinite,
};

class LoopInfo {
public:
  LoopInfo(String _name, IR::Block *_mainB, IR::Block *_condB,
           IR::Block *_restB, IR::LocalValue *_index, LoopType _type);

  String          name;
  IR::Block      *mainBlock;
  IR::Block      *condBlock;
  IR::Block      *restBlock;
  IR::LocalValue *index;
  LoopType        type;

  useit bool isTimes() const;
};

enum class BreakableType {
  loop,
  Switch,
};

class Breakable {
public:
  Breakable(Maybe<String> _tag, IR::Block *_restBlock);

  Maybe<String> tag;
  IR::Block    *restBlock;
};

class Context {
private:
  using IRBuilderTy =
      llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>;

public:
  Context();

  llvm::LLVMContext llctx;
  IRBuilderTy       builder;
  QatModule        *mod;
  IR::Function     *fn; // Active function
  llvm::Value      *selfVal;
  IR::CoreType     *activeType; // Active core type
  Vec<String>       exposed;
  bool              hasMain;
  Vec<LoopInfo *>   loopsInfo;
  Vec<Breakable *>  breakables;
  mutable u64       stringCount;

  // META

  Vec<fs::path> llvmOutputPaths;
  Vec<String>   nativeLibsToLink;

  useit QatModule *getMod() const; // Get the active IR module
  useit String     getGlobalStringName() const;
  useit utils::RequesterInfo getReqInfo() const;
  useit                      utils::VisibilityInfo
  getVisibInfo(Maybe<utils::VisibilityKind> kind) const;

  static void Error(const String &message, const utils::FileRange &fileRange);
  static void Warning(const String &message, const utils::FileRange &fileRange);
  static String highlightError(const String &message,
                               const char   *color = colors::yellow);
  static String highlightWarning(const String &message,
                                 const char   *color = colors::yellow);
};

} // namespace qat::IR

#endif