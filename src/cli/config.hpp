#ifndef QAT_CLI_CONFIG_HPP
#define QAT_CLI_CONFIG_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/VersionTuple.h"
#include <iostream>

namespace qat::cli {

class Config {
private:
  static Config* instance;

  fs::path qatDirPath;
  String   buildCommit;
  String   invokePath;

  Maybe<fs::path> stdLibPath;
  Maybe<fs::path> outputPath;

  Vec<fs::path> paths;
  Maybe<String> targetTriple;
  Maybe<String> clangPath;
  Maybe<String> linkerPath;
  Maybe<String> sysRoot;

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
  bool isNoStd        = false;
  bool diagnostic     = false;

  Maybe<bool> buildShared;
  Maybe<bool> buildStatic;

public:
  Config(u64 count, const char** args);

  static Config const* init(u64          count,
                            const char** args); // NOLINT(modernize-avoid-c-arrays)
  static Config const* get();
  static bool          hasInstance();

  static Maybe<std::filesystem::path> getExePathFromEnvPath(String name);
  static Maybe<std::filesystem::path> getExePath(String name);

  void setupEnvForQat();

  /** Behaviour specific functions */

  useit inline bool isCompile() const { return compile; }
  useit inline bool isRun() const { return run; }
  useit inline bool isAnalyse() const { return analyse; }
  useit inline bool shouldShowReport() const { return showReport; }
  useit inline bool isVerbose() const { return verbose; }
  useit inline bool shouldExportAST() const { return export_ast; }
  useit inline bool shouldSaveDocs() const { return saveDocs; }
  useit inline bool hasOutputPath() const { return outputPath.has_value(); }
  useit inline bool keepLLVM() const { return keepLLVMFiles; }
  useit inline bool hasTargetTriple() const { return targetTriple.has_value(); }
  useit inline bool hasStdLibPath() const { return stdLibPath.has_value(); }
  useit inline bool isNoStdEnabled() const { return isNoStd; }
  useit inline bool exportCodeMetadata() const { return exportCodeInfo; }
  useit inline bool noColorMode() const { return noColors; }
  useit inline bool isDebugMode() const { return !releaseMode; }
  useit inline bool isReleaseMode() const { return releaseMode; }
  useit inline bool hasSysroot() const { return sysRoot.has_value(); }
  useit inline bool hasClangPath() const { return clangPath.has_value(); }
  useit inline bool hasLinkerPath() const { return linkerPath.has_value(); }
  useit inline bool shouldBuildStatic() const { return buildShared.has_value() ? buildStatic.has_value() : true; }
  useit inline bool shouldBuildShared() const { return buildShared.value_or(false); }
  useit inline bool shouldExit() const { return exitAfter; }
  useit inline bool doDiagnostics() const { return diagnostic; }

  useit inline String getTargetTriple() const { return targetTriple.value_or(LLVM_HOST_TRIPLE); }
  useit inline String getSysroot() const { return sysRoot.value(); }
  useit inline String getClangPath() const { return clangPath.value(); }
  useit inline String getLinkerPath() const { return linkerPath.value(); }

  useit inline fs::path      getStdLibPath() const { return stdLibPath.value(); }
  useit inline fs::path      getOutputPath() const { return outputPath.value_or(fs::current_path()); }
  useit inline Vec<fs::path> getPaths() const { return paths; }

  useit inline const llvm::VersionTuple& getVersionTuple() const { return versionTuple; }

  ~Config() = default;
};

} // namespace qat::cli

#endif