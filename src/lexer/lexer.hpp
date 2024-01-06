#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include "../utils/file_range.hpp"
#include "./token.hpp"
#include <filesystem>
#include <fstream>

namespace qat::IR {
class Context;
}

namespace qat::lexer {

class Lexer {
private:
  fs::path      filePath;
  std::ifstream file;
  Vec<String>   content;
  char          prev;
  char          current;
  Vec<Token>*   tokens = nullptr;
  Deque<Token>  buffer;

  Vec<TokenType> bracketOccurences;

  u64 totalCharacterCount = 0;

  IR::Context* irCtx;

public:
  explicit Lexer(IR::Context* _irCtx) : irCtx(_irCtx){};
  useit static Lexer* get(IR::Context* ctx);

  ~Lexer();

  u64        lineNumber      = 1;
  u64        characterNumber = 0;
  Maybe<u64> previousLineEnd;
  static u64 timeInMicroSeconds;
  static u64 lineCount;

  void               clearTokens();
  void               throwError(const String& message, Maybe<usize> offset = None);
  void               analyse();
  void               read();
  void               changeFile(fs::path newFile);
  useit static Token wordToToken(const String& value, Lexer* lexInst);
  useit Vec<Token>* getTokens();
  useit Vec<String> getContent() const;
  useit Token       tokeniser();
  useit FileRange   getPosition(u64 length);
};

} // namespace qat::lexer

#endif