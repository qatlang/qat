#ifndef QAT_CLI_CONFIG_HPP
#define QAT_CLI_CONFIG_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "llvm/Support/VersionTuple.h"
#include <iostream>

namespace qat::cli {

class Config {
private:
  Config(u64 count, const char** args);

  String          invokePath;
  Vec<fs::path>   paths;
  Maybe<fs::path> outputPath;
  Maybe<String>   targetTriple;
  Maybe<String>   clangPath;
  // FIXME - Use this value
  Maybe<String>      lldPath;
  Maybe<String>      sysRoot;
  String             buildCommit;
  llvm::VersionTuple versionTuple;

  bool exitAfter      = false;
  bool verbose        = false;
  bool saveDocs       = false;
  bool showReport     = false;
  bool export_ast     = false;
  bool compile        = false;
  bool run            = false;
  bool analyse        = false;
  bool noColors       = false;
  bool releaseMode    = false;
  bool keepLLVMFiles  = false;
  bool exportCodeInfo = false;

  Maybe<bool> buildShared;
  Maybe<bool> buildStatic;

public:
  static Config* instance;

  static Config* init(u64          count,
                      const char** args); // NOLINT(modernize-avoid-c-arrays)
  static Config* get();
  static bool    hasInstance();

  /** Behaviour specific functions */

  useit bool isCompile() const;
  useit bool isRun() const;
  useit bool isAnalyse() const;
  useit Vec<fs::path> getPaths() const;
  useit bool          shouldSaveDocs() const;
  useit bool          shouldShowReport() const;
  useit bool          hasOutputPath() const;
  useit fs::path getOutputPath() const;
  useit bool     isVerbose() const;
  useit bool     shouldExportAST() const;
  useit String   getTargetTriple() const;
  useit bool     noColorMode() const;
  useit bool     isDebugMode() const;
  useit bool     isReleaseMode() const;
  useit bool     hasSysroot() const;
  useit String   getSysroot() const;
  useit bool     hasClangPath() const;
  useit String   getClangPath() const;
  useit bool     shouldBuildStatic() const;
  useit bool     shouldBuildShared() const;
  useit bool     keepLLVM() const;
  useit bool     exportCodeMetadata() const;
  useit const llvm::VersionTuple& getVersionTuple() const;

  // Whether compiler should exit after arguments are handled by Config
  //
  // This is usually true for simple actions like version display, about... and
  // if there were errors during Config initialisation
  useit bool shouldExit() const;

  ~Config() = default;
};

} // namespace qat::cli

#endif