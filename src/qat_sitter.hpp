#ifndef QAT_QAT_SITTER_HPP
#define QAT_QAT_SITTER_HPP

#include "./IR/context.hpp"
#include "./IR/qat_module.hpp"
#include "./cli/config.hpp"
#include "./lexer/lexer.hpp"
#include "./parser/parser.hpp"
#include "utils/helpers.hpp"
#include "llvm/IR/LLVMContext.h"
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
  Vec<IR::QatModule*> fileEntities;
  IR::Context*        Context;
  lexer::Lexer*       Lexer;
  parser::Parser*     Parser;
  Vec<fs::path>       queuedPaths;

public:
  QatSitter();

  void              init();
  void              queuePath(fs::path path);
  void              handlePath(const fs::path& path, llvm::LLVMContext& llctx);
  useit static bool checkExecutableExists(const String& name);

  ~QatSitter();
};

} // namespace qat

#endif