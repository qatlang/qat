#ifndef QAT_QAT_SITTER_HPP
#define QAT_QAT_SITTER_HPP

#include "./IR/context.hpp"
#include "./IR/qat_module.hpp"
#include "./cli/config.hpp"
#include "./lexer/lexer.hpp"
#include "./parser/parser.hpp"
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace qat {

namespace fs = std::filesystem;

class QatSitter {
private:
  std::vector<IR::QatModule *> fileEntities;

  // The Context instance used by this class to control IR generation
  IR::Context *Context;

  // The lexer instance used to manage lexical analysis of files
  lexer::Lexer *Lexer;

  // The parser instance that converts tokens to AST representation
  parser::Parser *Parser;

public:
  QatSitter();

  // Initialise QatSitter
  void init();

  void handlePath(fs::path path);

  ~QatSitter();
};

} // namespace qat

#endif