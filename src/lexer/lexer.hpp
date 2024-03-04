#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include "../utils/file_range.hpp"
#include "./token.hpp"
#include <filesystem>
#include <fstream>

namespace qat::ir {
class Ctx;
}

namespace qat::lexer {

class Lexer {
private:
  fs::path      filePath;
  std::ifstream file;
  char          prev;
  char          current;
  Vec<Token>*   tokens = nullptr;
  Deque<Token>  buffer;

  Vec<TokenType> bracketOccurences;

  ir::Ctx* irCtx;

public:
  explicit Lexer(ir::Ctx* _irCtx) : irCtx(_irCtx){};
  useit static Lexer* get(ir::Ctx* irCtx);

  ~Lexer();

  u64        lineNumber      = 1;
  u64        characterNumber = 0;
  Maybe<u64> previousLineEnd;
  static u64 timeInMicroSeconds;
  static u64 lineCount;

  void clear_tokens();
  void throw_error(const String& message, Maybe<usize> offset = None);
  void analyse();
  void read();
  void change_file(fs::path newFile);

  useit static Token word_to_token(const String& value, Lexer* lexInst);
  useit Vec<Token>* get_tokens();
  useit Token       tokeniser();

  useit FileRange get_position(u64 length);
};

} // namespace qat::lexer

#endif