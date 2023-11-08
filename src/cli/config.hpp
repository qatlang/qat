#ifndef QAT_CLI_CONFIG_HPP
#define QAT_CLI_CONFIG_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "llvm/Support/VersionTuple.h"
#include <iostream>

namespace qat::cli {

// CLI Configuration -
//
// This is a singleton and the instance is dynamically allocated. It is freed
// automatically when the program exits, but can also be freed by calling
// Config::destroy()
class Config {
private:
  // Construct a new Config object
  // This expects the number of arguments passed to the compiler and the
  // arguments, and it initialises the members and sets corresponding values
  Config(u64 count, const char** args);

  // The first CLI argument. Indicates the path specified to invoke the compiler
  String invokePath;

  // All paths provided to the compiler for compilation of qat source code
  Vec<fs::path> paths;

  // Path to be used for outputs
  Maybe<fs::path> outputPath;

  // Compile target value
  Maybe<String> targetTriple;

  Maybe<String> clangPath;

  Maybe<String> lldPath;

  // Sysroot for alternative system folder for the linker
  Maybe<String> sysRoot;

  // The latest commit at the time of the build of the compiler
  String buildCommit;

  llvm::VersionTuple versionTuple;

  // Whether the compiler should exit after initialisation of Config is
  // complete and arguments have been handled
  bool exitAfter;

  // Whether the user has chosen to be in Verbose mode
  bool verbose;

  // Whether comments in the source files should be analysed, parsed and then
  // saved along with the AST, to later be used for documentation
  bool saveDocs;

  // Whether reports about the timing and performance of the lexing, parsing,
  // generation and other processes should be displayed in the console.
  bool showReport;

  // Whether AST should be exported
  bool export_ast;

  // Whether user has chosen to compile
  bool compile;

  bool run;

  bool analyse;

  bool noColors;

  bool releaseMode;

  Maybe<bool> buildShared;

  Maybe<bool> buildStatic;

  bool keepLLVMFiles = false;

  bool exportCodeInfo = false;

public:
  /**
   *  The pointer to the only instance of Config. If this is nullptr, the
   * either the Config hasn't been initialised or it has been freed from memory
   *
   */
  static Config* instance;

  /**
   *  The initialisation function for Config. Multiple calls to this
   * function returns the same instance
   *
   * @param count The number of arguments provided to the compiler
   * @param args The arguments provided to the compiler
   * @return Config
   */
  static Config* init(u64          count,
                      const char** args); // NOLINT(modernize-avoid-c-arrays)

  /**
   *  This is technically the proper function to get the existing instance
   * of Config. Or may be not, since init already returns an instance
   *
   * @return Config
   */
  static Config* get();

  // Whether there is an instance of Config that has been initialised
  static bool hasInstance();

  /** Behaviour specific functions */

  useit bool isCompile() const;                 // Whether there are paths and the compilation can proceed
  useit bool isRun() const;                     // Whether the program should be run after building
  useit bool isAnalyse() const;                 // Whether files should be analysed and not compiled
  useit Vec<fs::path> getPaths() const;         // Get paths provided for the compilation stage
  useit bool          shouldSaveDocs() const;   //  Whether comments should be saved for documentation
  useit bool          shouldShowReport() const; //  Whether performance & timing reports should be shown
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