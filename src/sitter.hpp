#ifndef QAT_QAT_SITTER_HPP
#define QAT_QAT_SITTER_HPP

#include "./IR/context.hpp"
#include "./IR/qat_module.hpp"
#include "./lexer/lexer.hpp"
#include "./parser/parser.hpp"

#include <helpers/deque.hpp>
#include <helpers/maybe.hpp>
#include <helpers/pair.hpp>
#include <helpers/string.hpp>

namespace qat {

class QatSitter {
	friend class qat::ir::Ctx;

  private:
	Deque<ir::Mod*> fileEntities;
	ir::Ctx*        ctx    = nullptr;
	lexer::Lexer*   Lexer  = nullptr;
	parser::Parser* Parser = nullptr;

  public:
	QatSitter();
	static QatSitter* get();
	static QatSitter* instance;

	void initialise();
	void destroy();
	void remove_entity_with_path(FilePath const& path);
	void handle_path(FilePath const& path, ir::Ctx* irCtx);
	void display_stats();

	static bool is_name_valid(String const& name);

	static Maybe<Pair<String, FilePath>> detect_lib_file(FilePath const& path);

	~QatSitter();
};

} // namespace qat

#endif
