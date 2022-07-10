#ifndef QAT_CLI_CONFIG_HPP
#define QAT_CLI_CONFIG_HPP

#include "../utils/types.hpp"
#include "./display.hpp"
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace qat {
namespace CLI {

namespace fs = std::filesystem;

enum class CompileTarget { normal, cpp, json };

/**
 *  CLI Configuration -
 *
 * This is a singleton and the instance is dynamically allocated. It is freed
 * automatically when the program exits, but can also be freed by calling
 * Config::destroy()
 *
 */
class Config {
  // TODO - Add support for output paths
private:
  /**
   *  Construct a new Config object
   * This expects the number of arguments passed to the compiler and the
   * arguments, and it initialises the members and sets corresponding values
   *
   * @param count
   * @param args
   */
  Config(u64 count, const char **args);

  // The first CLI argument. Indicates the path specified to invoke the compiler
  std::string invokePath;

  // All paths provided to the compiler for compilation of qat source code
  std::vector<fs::path> paths;

  // Path to be used for outputs
  std::optional<fs::path> outputPath;

  // Compile target value
  CompileTarget target;

  // The latest commit at the time of the build of the compiler
  std::string buildCommit;

  // Whether the compiler should exit after initialisation of Config is
  // complete and arguments have been handled
  bool exitAfter;

  // Whether the user has chosen to be in Verbose mode
  bool verbose;

  /**
   *  Whether comments in the source files should be analysed, parsed and
   * then saved along with the AST, to later be used for documentation
   *
   */
  bool saveDocs;

  /**
   *  Whether reports about the timing and performance of the lexing,
   * parsing, generation and other processes should be displayed in the console.
   *
   */
  bool showReport;

  /**
   *  Whether lexer should display the tokens analysed in the console
   *
   */
  bool lexer_emit_tokens;

public:
  /**
   *  The pointer to the only instance of Config. If this is nullptr, the
   * either the Config hasn't been initialised or it has been freed from memory
   *
   */
  static Config *instance;

  /**
   *  The initialisation function for Config. Multiple calls to this
   * function returns the same instance
   *
   * @param count The number of arguments provided to the compiler
   * @param args The arguments provided to the compiler
   * @return Config
   */
  static Config *init(u64 count, const char **args);

  /**
   *  This is technically the proper function to get the existing instance
   * of Config. Or may be not, since init already returns an instance
   *
   * @return Config
   */
  static Config *get();

  // Get the CompileTarget from the argument
  static CompileTarget getCompileTarget(std::string val);

  /**
   *  Whether there is an instance of Config that has been initialised
   *
   * @return true
   * @return false
   */
  static bool hasInstance();

  /**
   *  Function that destroys/deletes the single instance of Config
   *
   */
  static void destroy();

  /** Behaviour specific functions */

  // Whether there are paths provided in the CLI and the compilation step
  // should proceed
  bool isCompile() const;

  // Whether compiler should exit after arguments are handled by Config
  //
  // This is usually true for simple actions like version display, about... and
  // if there were errors during Config initialisation
  bool shouldExit() const;

  // Get paths provided to the compiler for the compilation stage
  std::vector<fs::path> getPaths() const;

  //  Whether comments in source code files should be saved to be used for
  // documentation
  bool shouldSaveDocs() const;

  //  Whether different parts of the compiler should show reports about
  // the performance and timing statistics
  bool shouldShowReport() const;

  //  Whether lexer should display the analysed tokens in the console
  bool shouldLexerEmitTokens() const;

  // Whether an output path is provided to the compiler
  bool hasOutputPath() const;

  bool isVerbose() const;

  // Get the compile-target provided by the user
  CompileTarget getTarget() const;

  ~Config();
};

} // namespace CLI
} // namespace qat

#endif