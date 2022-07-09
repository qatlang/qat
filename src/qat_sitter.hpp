#ifndef QAT_QAT_SITTER_HPP
#define QAT_QAT_SITTER_HPP

#include "./CLI/config.hpp"
#include "./IR/context.hpp"
#include "./IR/qat_module.hpp"
#include "./lexer/lexer.hpp"
#include "./parser/parser.hpp"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
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
  /**
   *  Top level modules found.
   *
   */
  std::vector<IR::QatModule *> top_modules;

  /**
   *  All modules found during the compilation phase
   *
   */
  std::vector<IR::QatModule *> modules;

  /**
   *  The ctx instance used by this class to control IR generation
   *
   */
  IR::Context *Generator;

  /**
   *  The lexer instance used to manage lexical analysis of files
   *
   */
  lexer::Lexer *Lexer;

  /**
   *  The parser instance that converts tokens to AST representation
   *
   */
  parser::Parser *Parser;

public:
  QatSitter();

  void init();

  std::vector<IR::QatModule *> handle_top_modules(fs::path path);

  ~QatSitter();
};

} // namespace qat

#endif