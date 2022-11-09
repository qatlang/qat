#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include "../utils/file_range.hpp"
#include "./token.hpp"
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
  u32           genericStartCount   = 0;
  u64           totalCharacterCount = 0;

public:
  Lexer() = default;

  u64        lineNumber      = 1;
  u64        characterNumber = 0;
  Maybe<u64> previousLineEnd;
  static u64 timeInMicroSeconds;

  void  clearTokens();
  void  throwError(const String& message);
  void  analyse();
  void  read();
  void  changeFile(fs::path newFile);
  useit Deque<Token>* getTokens();
  useit Vec<String> getContent() const;
  useit Token       tokeniser();
  useit utils::FileRange getPosition(u64 length);
};

} // namespace qat::lexer

#endif