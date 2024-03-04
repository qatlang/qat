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
  bool exportAST      = false;
  bool compile        = false;
  bool run            = false;
  bool analyse        = false;
  bool noColors       = false;
  bool releaseMode    = false;
  bool clearLLVMFiles = false;
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

  static Maybe<std::filesystem::path> get_exe_path_from_env(String name);
  static Maybe<std::filesystem::path> get_exe_path(String name);

  void setupEnvForQat();

  /** Behaviour specific functions */

  useit inline bool is_workflow_compile() const { return compile; }
  useit inline bool is_workflow_run() const { return run; }
  useit inline bool is_workflow_analyse() const { return analyse; }
  useit inline bool should_show_report() const { return showReport; }
  useit inline bool is_verbose() const { return verbose; }
  useit inline bool should_export_ast() const { return exportAST; }
  useit inline bool should_save_docs() const { return saveDocs; }
  useit inline bool has_output_path() const { return outputPath.has_value(); }
  useit inline bool clear_llvm() const { return clearLLVMFiles; }
  useit inline bool has_target_triple() const { return targetTriple.has_value(); }
  useit inline bool has_std_lib_path() const { return stdLibPath.has_value(); }
  useit inline bool is_no_std_enabled() const { return isNoStd; }
  useit inline bool export_code_metadata() const { return exportCodeInfo; }
  useit inline bool no_color_mode() const { return noColors; }
  useit inline bool is_build_mode_debug() const { return !releaseMode; }
  useit inline bool is_build_mode_release() const { return releaseMode; }
  useit inline bool has_sysroot() const { return sysRoot.has_value(); }
  useit inline bool has_clang_path() const { return clangPath.has_value(); }
  useit inline bool has_linker_path() const { return linkerPath.has_value(); }
  useit inline bool should_build_static() const { return buildShared.has_value() ? buildStatic.has_value() : true; }
  useit inline bool should_build_shared() const { return buildShared.value_or(false); }
  useit inline bool should_exit() const { return exitAfter; }
  useit inline bool should_do_diagnostics() const { return diagnostic; }

  useit inline String get_target_triple() const { return targetTriple.value_or(LLVM_HOST_TRIPLE); }
  useit inline String get_sysroot() const { return sysRoot.value(); }
  useit inline String get_clang_path() const { return clangPath.value(); }
  useit inline String get_linker_path() const { return linkerPath.value(); }

  useit inline fs::path      get_std_lib_path() const { return stdLibPath.value(); }
  useit inline fs::path      get_output_path() const { return outputPath.value_or(fs::current_path()); }
  useit inline Vec<fs::path> get_paths() const { return paths; }

  useit inline const llvm::VersionTuple& get_version_tuple() const { return versionTuple; }

  ~Config() = default;
};

} // namespace qat::cli

#endif