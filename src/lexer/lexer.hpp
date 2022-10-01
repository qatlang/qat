#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include "../cli/color.hpp"
#include "../cli/config.hpp"
#include "../utils/file_range.hpp"
#include "../utils/is_integer.hpp"
#include "./token.hpp"
#include "./token_type.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace qat::lexer {

// Lexer of the QAT Programming language. This handles lexical
// analysis, emission of tokens and also will report on statistics
// regarding the analysis
class Lexer {
private:
  fs::path      filePath;
  std::fstream  file;
  Vec<String>   content;
  char          prev;
  char          current;
  Deque<Token>* tokens;
  Deque<Token>  buffer;
  u32           template_type_start_count = 0;
  u64           total_char_count          = 0;
  u64           timeInNS                  = 0;

public:
  Lexer();

  u64        lineNumber      = 1;
  u64        characterNumber = 0;
  Maybe<u64> previousLineEnd;

  void        clear_tokens();
  void        throw_error(const String& message);
  void        analyse();
  void        read();
  void        changeFile(fs::path newFile);
  void        printStatus();
  static bool show_report;
  useit Deque<Token>* get_tokens();
  useit Vec<String> getContent() const;
  useit Token       tokeniser();
  useit utils::FileRange getPosition(u64 length);
};

} // namespace qat::lexer

#endif