#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include "../CLI/color.hpp"
#include "../CLI/config.hpp"
#include "../utils/file_placement.hpp"
#include "../utils/is_integer.hpp"
#include "./token.hpp"
#include "./token_type.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace qat {
namespace lexer {

/**
 *  Lexer of the QAT Programming language. This handles lexical
 * analysis, emission of tokens and also will report on statistics
 * regarding the analysis
 *
 */
class Lexer {
private:
  /**
   *  Path to the current file
   *
   */
  std::string filePath;

  /**
   *  The current file that is being analysed by the lexer.
   * This is open only if the analysis is in progress.
   *
   */
  std::fstream file;

  /**
   *  Context that can provide information about the previous
   * token emitted by the Lexer
   *
   */
  std::string prev_ctx;

  /**
   *  Previous character read by the `read` function. This
   * is changed everytime a new character is read and is saved to the
   * current variable
   *
   */
  char prev;

  /**
   *  The latest character read by the `read` function. This
   * value is updated everytime a new character is read from the current
   * file
   *
   */
  char curr;

  /**
   *  Vector of all tokens found during lexical analysis. This is
   * updated after each invocation of the `tokeniser` function.
   *
   */
  std::vector<Token> tokens;

  /**
   *  A temporary buffer for tokens that have not yet been added to
   * the `tokens` vector
   *
   */
  std::vector<Token> buffer;

  /**
   *  Number of templateTypeStart tokens encountered that haven't been
   * closed by a templateTypeEnd token
   *
   */
  unsigned int template_type_start_count = 0;

  /**
   *  Total number of characters found in the file. This is for
   * reporting purposes
   *
   */
  unsigned long long total_char_count = 0;

  /**
   *  Time taken by the lexer to analyse the entire file, in
   * nanoseconds. This is later converted to appropriate higher units,
   * depending on the value
   *
   */
  unsigned long long timeInNS = 0;

public:
  Lexer();

  /**
   *  Whether the Lexer should emit tokens to the standard output.
   * If this is set to true, all tokens found in a file are dumped to the
   * standard output in a human-readable format.
   *
   */
  static bool emit_tokens;

  /**
   *  Whether the Lexer should display report on its behaviour and
   * analysis. For now, enabling this option shows the time taken by the
   * lexer to analyse a file.
   *
   */
  static bool show_report;

  /**
   *  Get the tokens that has been obtained so far from the analysis
   *
   * @return std::vector<Token>
   */
  std::vector<Token> &get_tokens();

  /**
   *  Clear all the tokens obtained from the analysis so far
   *
   */
  void clear_tokens();

  /**
   *  Logs any error occuring during Lexical Analysis.
   *
   * @param message A meaningful message about the type of error
   */
  void throw_error(std::string message);

  /**
   *  Analyses the current file and emit tokens as identified
   * by the language
   *
   * @return std::vector<Token> Returns all tokens identified during
   * analysis.
   */
  void analyse();

  /**
   *  This function reads the next character in the active file
   * of the Lexer. This can be customised using the context string
   *
   * @param lexerContext A simple string that provides context for the
   * reader to customise reading behaviour.
   */
  void read(std::string lexerContext);

  /**
   *  Handles the change of the active file
   *
   * @param newFile Path to the new file that should be analysed by the
   * lexer next
   */

  void changeFile(std::string newFile);

  /**
   *  Most important part of the Lexer. This emits tokens one at a
   * time as it goes through the source file
   *
   * @return Token The latest token obtained during lexical analysis
   */
  Token tokeniser();

  /**
   *  The current line of the lexer. This is incremented everytime
   * the End of Line is encountered
   *
   */
  unsigned long long line_num = 1;

  /**
   *  The number pertaining to the current character read by the
   * lexer. This is incremented by the `read` function everytime a
   * character is read in the file. This is reset to zero after the End
   * of Line is encountered
   *
   */
  unsigned long long char_num = 0;

  /**
   *  Get the FilePosition object for a token
   *
   * @return utilities::FilePosition
   */
  utils::FilePlacement getPosition(unsigned long long length);

  /**
   *  Prints all status about the lexical analysis to the standard
   * output. This function's output is dependent on the CLI arguments
   * `--emit-tokens` and `--report` provided to the compiler
   *
   */
  void printStatus();
};
} // namespace lexer
} // namespace qat

#endif