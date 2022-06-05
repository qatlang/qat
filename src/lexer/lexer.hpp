/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include "../CLI/color.hpp"
#include "../utils/file_placement.hpp"
#include "../utils/is_integer.hpp"
#include "./token.hpp"
#include "./token_type.hpp"
#include <chrono>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fsexp = std::experimental::filesystem;

namespace qat {
namespace lexer {
/**
 * @brief Lexer of the QAT Programming language. This handles lexical
 * analysis, emission of tokens and also will report on statistics
 * regarding the analysis
 *
 */
class Lexer {
private:
  /**
   * @brief Path to the current file
   *
   */
  std::string filePath;

  /**
   * @brief The current file that is being analysed by the lexer.
   * This is open only if the analysis is in progress.
   *
   */
  std::fstream file;

  /**
   * @brief Context that can provide information about the previous
   * token emitted by the Lexer
   *
   */
  std::string prev_ctx;

  /**
   * @brief Previous character read by the `read` function. This
   * is changed everytime a new character is read and is saved to the
   * current variable
   *
   */
  char prev;

  /**
   * @brief The latest character read by the `read` function. This
   * value is updated everytime a new character is read from the current
   * file
   *
   */
  char curr;

  /**
   * @brief Vector of all tokens found during lexical analysis. This is
   * updated after each invocation of the `tokeniser` function.
   *
   */
  std::vector<Token> tokens;

  /**
   * @brief A temporary buffer for tokens that have not yet been added to
   * the `tokens` vector
   *
   */
  std::vector<Token> buffer;

  /**
   * @brief Number of templateTypeStart tokens encountered that haven't been
   * closed by a templateTypeEnd token
   *
   */
  unsigned int template_type_start_count = 0;

  /**
   * @brief Total number of characters found in the file. This is for
   * reporting purposes
   *
   */
  unsigned long long total_char_count = 0;

  /**
   * @brief Time taken by the lexer to analyse the entire file, in
   * nanoseconds. This is later converted to appropriate higher units,
   * depending on the value
   *
   */
  unsigned long long timeInNS = 0;

public:
  Lexer() {}

  /**
   * @brief Whether the Lexer should emit tokens to the standard output.
   * If this is set to true, all tokens found in a file are dumped to the
   * standard output in a human-readable format.
   *
   */
  static bool emit_tokens;

  /**
   * @brief Whether the Lexer should display report on its behaviour and
   * analysis. For now, enabling this option shows the time taken by the
   * lexer to analyse a file.
   *
   */
  static bool show_report;

  /**
   * @brief Get the tokens that has been obtained so far from the analysis
   *
   * @return std::vector<Token>
   */
  std::vector<Token> get_tokens();

  /**
   * @brief Clear all the tokens obtained from the analysis so far
   *
   */
  void clear_tokens();

  /**
   * @brief Logs any error occuring during Lexical Analysis.
   *
   * @param message A meaningful message about the type of error
   */
  void throw_error(std::string message);

  /**
   * @brief Analyses the current file and emit tokens as identified
   * by the language
   *
   * @return std::vector<Token> Returns all tokens identified during
   * analysis.
   */
  void analyse();

  /**
   * @brief This function reads the next character in the active file
   * of the Lexer. This can be customised using the context string
   *
   * @param lexerContext A simple string that provides context for the
   * reader to customise reading behaviour.
   */
  void read(std::string lexerContext);

  /**
   * @brief Handles the change of the active file
   *
   * @param newFile Path to the new file that should be analysed by the
   * lexer next
   */

  void changeFile(std::string newFile);

  /**
   * @brief Most important part of the Lexer. This emits tokens one at a
   * time as it goes through the source file
   *
   * @return Token The latest token obtained during lexical analysis
   */
  Token tokeniser();

  /**
   * @brief The current line of the lexer. This is incremented everytime
   * the End of Line is encountered
   *
   */
  unsigned long long line_num = 1;

  /**
   * @brief The number pertaining to the current character read by the
   * lexer. This is incremented by the `read` function everytime a
   * character is read in the file. This is reset to zero after the End
   * of Line is encountered
   *
   */
  unsigned long long char_num = 0;

  /**
   * @brief Get the FilePosition object for a token
   *
   * @return utilities::FilePosition
   */
  utils::FilePlacement getPosition(unsigned long long length);

  /**
   * @brief Prints all status about the lexical analysis to the standard
   * output. This function's output is dependent on the CLI arguments
   * `--emit-tokens` and `--report` provided to the compiler
   *
   */
  void printStatus();
};
} // namespace lexer
} // namespace qat

#endif