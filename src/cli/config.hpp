#ifndef QAT_CLI_CONFIG_HPP
#define QAT_CLI_CONFIG_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "./display.hpp"
#include <iostream>

namespace qat::cli {

enum class CompileTarget { normal, cpp, json };

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
  Config(u64 count, char** args);

  // The first CLI argument. Indicates the path specified to invoke the compiler
  String invokePath;

  // All paths provided to the compiler for compilation of qat source code
  Vec<fs::path> paths;

  // Path to be used for outputs
  Maybe<fs::path> outputPath;

  // Compile target value
  CompileTarget target;

  // The latest commit at the time of the build of the compiler
  String buildCommit;

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

  bool outputInTemporaryPath;

  bool noColors;

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
  static Config* init(u64   count,
                      char* args[]); // NOLINT(modernize-avoid-c-arrays)

  /**
   *  This is technically the proper function to get the existing instance
   * of Config. Or may be not, since init already returns an instance
   *
   * @return Config
   */
  static Config* get();

  // Parse the CompileTarget from the argument
  static CompileTarget parseCompileTarget(const String& val);

  // Whether there is an instance of Config that has been initialised
  static bool hasInstance();

  // Function that destroys/deletes the single instance of Config
  static void destroy();

  /** Behaviour specific functions */

  // Whether there are paths provided in the CLI and the compilation step
  // should proceed
  useit bool isCompile() const;

  // Whether the program should be run after building. This is valid only if
  // there is a valid main function present
  useit bool isRun() const;

  // Whether files should be analysed for errors, warnings and infos and not compiled
  useit bool isAnalyse() const;

  // Whether compiler should exit after arguments are handled by Config
  //
  // This is usually true for simple actions like version display, about... and
  // if there were errors during Config initialisation
  useit bool shouldExit() const;

  // Get paths provided to the compiler for the compilation stage
  useit Vec<fs::path> getPaths() const;

  //  Whether comments in source code files should be saved to be used for
  // documentation
  useit bool shouldSaveDocs() const;

  //  Whether different parts of the compiler should show reports about
  // the performance and timing statistics
  useit bool shouldShowReport() const;

  // Whether an output path is provided to the compiler
  useit bool hasOutputPath() const;

  // Get the output path provided to the compiler
  useit fs::path getOutputPath() const;

  // Whether compiler outputs should be verbose
  useit bool isVerbose() const;

  // Whether the AST should be exported
  useit bool shouldExportAST() const;

  // Get the compile-target provided by the user
  useit CompileTarget getTarget() const;

  useit bool outputToTempDir() const;

  useit bool noColorMode() const;

  ~Config();
};

} // namespace qat::cli

#endif