#ifndef QAT_QAT_SITTER_HPP
#define QAT_QAT_SITTER_HPP

#include "./IR/context.hpp"
#include "./IR/qat_module.hpp"
#include "./lexer/lexer.hpp"
#include "./parser/parser.hpp"
#include "utils/helpers.hpp"
#include <filesystem>
#include <thread>

namespace qat {

namespace fs = std::filesystem;

class QatSitter {
  friend class qat::ir::Ctx;

private:
  Deque<ir::Mod*> fileEntities;
  ir::Ctx*        ctx    = nullptr;
  lexer::Lexer*   Lexer  = nullptr;
  parser::Parser* Parser = nullptr;
  std::thread::id mainThread;

public:
  QatSitter();
  useit static QatSitter* get();
  static QatSitter*       instance;

  void initialise();
  void destroy();
  void remove_entity_with_path(const fs::path& path);
  void handle_path(const fs::path& path, ir::Ctx* irCtx);
  void display_stats();

  useit static bool check_exe_exists(const String& name);
  useit static bool is_name_valid(const String& name);

  useit static Maybe<Pair<String, fs::path>> detect_lib_file(const fs::path& path);

  ~QatSitter();
};

} // namespace qat

#endif