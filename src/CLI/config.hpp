#ifndef QAT_CLI_CONFIG_HPP
#define QAT_CLI_CONFIG_HPP

#include "../utils/types.hpp"
#include "./display.hpp"
#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace qat {
namespace CLI {

namespace fsexp = std::experimental::filesystem;

/**
 * @brief CLI Configuration -
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
   * @brief Construct a new Config object
   * This expects the number of arguments passed to the compiler and the
   * arguments, and it initialises the members and sets corresponding values
   *
   * @param count
   * @param args
   */
  Config(u64 count, const char **args);

  /**
   * @brief The first CLI argument. Indicates the path specified to invoke the
   * compiler
   *
   */
  std::string invoke_path;

  /**
   * @brief All paths provided to the compiler for compilation of qat source
   * code.
   *
   */
  std::vector<fsexp::path> paths;

  /**
   * @brief The latest commit at the time of the build of the compiler
   *
   */
  std::string build_commit;

  /**
   * @brief Whether the compiler should exit after initialisation of Config is
   * complete and arguments have been handled
   *
   */
  bool exit_after;

  /**
   * @brief Whether the user has chosen to be in Verbose mode
   *
   */
  bool verbose;

  /**
   * @brief Whether comments in the source files should be analysed, parsed and
   * then saved along with the AST, to later be used for documentation
   *
   */
  bool save_docs;

  /**
   * @brief Whether reports about the timing and performance of the lexing,
   * parsing, generation and other processes should be displayed in the console.
   *
   */
  bool show_report;

  /**
   * @brief Whether lexer should display the tokens analysed in the console
   *
   */
  bool lexer_emit_tokens;

public:
  /**
   * @brief The pointer to the only instance of Config. If this is nullptr, the
   * either the Config hasn't been initialised or it has been freed from memory
   *
   */
  static Config *instance;

  /**
   * @brief The initialisation function for Config. Multiple calls to this
   * function returns the same instance
   *
   * @param count The number of arguments provided to the compiler
   * @param args The arguments provided to the compiler
   * @return Config
   */
  static Config *init(u64 count, const char **args);

  /**
   * @brief This is technically the proper function to get the existing instance
   * of Config. Or may be not, since init already returns an instance
   *
   * @return Config
   */
  static Config *get();

  /**
   * @brief Whether there is an instance of Config that has been initialised
   *
   * @return true
   * @return false
   */
  static bool has_instance();

  /**
   * @brief Function that destroys/deletes the single instance of Config
   *
   */
  static void destroy();

  /** Behaviour specific functions */

  /**
   * @brief Whether there are paths provided in the CLI and the compilation step
   * should proceed
   *
   * @return true
   * @return false
   */
  bool is_compile() { return !paths.empty(); }

  /**
   * @brief Whether compiler should exit after arguments are handled by Config
   *
   * This is usually true for simple actions like version display, about... and
   * if there were errors during Config initialisation
   *
   * @return true
   * @return false
   */
  bool should_exit() { return exit_after; }

  /**
   * @brief Get paths provided to the compiler for the compilation stage
   *
   * @return std::vector<fsexp::path>
   */
  std::vector<fsexp::path> get_paths() { return paths; }

  /**
   * @brief Whether comments in source code files should be saved to be used for
   * documentation
   *
   * @return true
   * @return false
   */
  bool should_save_docs() { return save_docs; }

  /**
   * @brief Whether different parts of the compiler should show reports about
   * the performance and timing statistics
   *
   * @return true
   * @return false
   */
  bool should_show_report() { return show_report; }

  /**
   * @brief Whether lexer should display the analysed tokens in the console
   *
   * @return true
   * @return false
   */
  bool should_lexer_emit_tokens() { return lexer_emit_tokens; }

  ~Config();
};

} // namespace CLI
} // namespace qat

#endif