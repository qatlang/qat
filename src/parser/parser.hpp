#ifndef QAT_PARSER_PARSER_HPP
#define QAT_PARSER_PARSER_HPP

#include "../ast/meta_info.hpp"
#include "../ast/type_like.hpp"
#include "../ast/types/variadics.hpp"
#include "../lexer/token.hpp"
#include "../lexer/token_type.hpp"
#include "../utils/identifier.hpp"
#include "./cache_symbol.hpp"
#include "./parser_context.hpp"

#include <chrono>
#include <helpers/files.hpp>
#include <helpers/hashmap.hpp>
#include <helpers/integers.hpp>
#include <helpers/maybe.hpp>
#include <helpers/vec.hpp>

namespace qat::ast {

class PlainInitialiser;
class Argument;
class MatchValue;
class Sentence;
class MemberParentLike;
class FillGeneric;
class DefineFlagType;

} // namespace qat::ast

namespace qat::parser {

struct EntityMetadata {
	ast::PrerunExpression* defineChecker;
	ast::PrerunExpression* genericConstraint;
	Maybe<ast::MetaInfo>   metaInfo;
	usize                  lastIndex;

	EntityMetadata(ast::PrerunExpression* _defineCheck, ast::PrerunExpression* _generic, Maybe<ast::MetaInfo> _metaInfo,
	               usize _lastInd)
	    : defineChecker(_defineCheck), genericConstraint(_generic), metaInfo(_metaInfo), lastIndex(_lastInd) {}
};

class Parser {
  private:
	Vec<lexer::Token>&           tokens;
	Vec<FilePath>                importedPaths;
	Vec<FilePath>                memberPaths;
	HashMap<usize, lexer::Token> comments;
	ParserContext                g_ctx;
	ir::Ctx*                     irCtx;

	// Filter all comments from the original token sequence and set a new
	// sequence that maps comments to the relevant AST members
	//
	// This will be used for documentation of source code
	void filter_comments();

  public:
	explicit Parser(ir::Ctx* irCtx, Vec<lexer::Token>& tokens);
	static Parser* get(ir::Ctx* irCtx, Vec<lexer::Token>& tokens);
	~Parser();

	static u64 timeInNanoseconds;
	static u64 tokenCount;

	void clear_imported_paths();

	void do_type_contents(ParserContext& prev_ctx, usize from, usize upto, ast::MemberParentLike* memberParent);

	void parse_mix_type(ParserContext& prev_ctx, usize from, usize upto, Vec<Pair<Identifier, Maybe<ast::Type*>>>& uRef,
	                    Maybe<FileRangePtr>& noneVariant, Vec<FileRangePtr>& fileRanges, Maybe<usize>& defaultVal);

	void do_choice_type(usize from, usize upto, Vec<Pair<Vec<Identifier>, Maybe<ast::PrerunExpression*>>>& fields,
	                    Maybe<usize>& defaultVal, bool& hasNoneVariant);

	Pair<ast::DefineFlagType*, usize> do_flag_type(usize from, Identifier name, ast::Type* providedType,
	                                               Maybe<ast::VisibilitySpec> visibSpec, FileRangePtr startRange);

	void parse_match_contents(ParserContext& prev_ctx, usize from, usize upto,
	                          Vec<Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>>& chain,
	                          Maybe<Pair<Vec<ast::Sentence*>, FileRangePtr>>&        elseCase);

	void add_error(const String& message, FileRangePtr fileRange);

	String color_error(const String& message);

	static void add_warning(const String& message, FileRangePtr fileRange);

	bool is_previous(lexer::TokenType type, usize current);

	bool is_next(lexer::TokenType type, usize current) {
		if ((current + 1) < tokens.size()) {
			return tokens.at(current + 1).type == type;
		} else {
			return false;
		}
	}

	bool are_only_present_within(const Vec<lexer::TokenType>& kinds, usize from, usize upto);

	bool is_primary_within(lexer::TokenType candidate, usize from, usize upto);

	ast::ImportEntities* parse_import_entities(ParserContext& ctx, Maybe<ast::VisibilitySpec> visibKind, usize from,
	                                           usize upto);

	ast::ImportPaths* parse_import_paths(bool isMember, usize from, usize upto, Maybe<ast::VisibilitySpec> spec,
	                                     FileRangePtr start);

	Vec<FilePath>& get_imported_paths();

	Vec<FilePath>& get_member_paths();

	void clear_member_paths();

	EntityMetadata do_entity_metadata(ParserContext& parserCtx, usize from, String entityType, usize genericLength);

	ast::MetaInfo do_meta_info(usize from, usize upto, FileRangePtr fileRange);

	Pair<ast::VisibilitySpec, usize> do_visibility_kind(usize from);

	Vec<ast::FillGeneric*> do_generic_fill(ParserContext& prevCtx, usize from, usize upto);

	Pair<ast::Type*, usize> do_type(ParserContext& prevCtx, usize from, Maybe<usize> upto,
	                                bool isPartOfExpression = false);

	Pair<ast::DefineSkill*, usize> do_skill(Maybe<ast::VisibilitySpec> visibSpec, usize from);

	Vec<ast::Node*> begin_parsing();

	Vec<ast::Node*> parse(ParserContext prevCtx = ParserContext(), usize from = -1, usize upto = 0);

	Pair<CacheSymbol, usize> do_symbol(ParserContext& prevCtx, usize start);

	Pair<Vec<ast::Argument*>, Maybe<ast::Variadics>> do_function_parameters(ParserContext& prevCtx, usize from,
	                                                                        usize upto);

	Pair<ast::PrerunExpression*, usize> do_prerun_expression(ParserContext& prevCtx, usize from, Maybe<usize> upto,
	                                                         bool returnOnFirstExp = false);

	Pair<ast::Expression*, usize> do_expression(ParserContext& prevCtx, const Maybe<CacheSymbol>& symbol, usize from,
	                                            Maybe<usize> upto, Maybe<ast::Expression*> cachedExpressions = None,
	                                            bool returnAtFirstExp = false);

	Vec<ast::Expression*> do_separated_expressions(ParserContext& prevCtx, usize from, usize upto);

	Vec<ast::PrerunExpression*> do_separated_prerun_expressions(ParserContext& prevCtx, usize from, usize upto);

	Vec<ast::Sentence*> do_sentences(ParserContext& prevCtx, usize from, usize upto);

	Pair<Vec<ast::PrerunSentence*>, usize> do_prerun_sentences(ParserContext& preCtx, usize from);

	Maybe<usize> get_pair_end(lexer::TokenType startType, lexer::TokenType endType, usize current);

	Maybe<usize> first_primary_position(lexer::TokenType candidate, usize from);

	Vec<usize> primary_positions_within(lexer::TokenType candidate, usize from, usize upto);

	Vec<ast::GenericAbstractType*> do_generic_abstracts(ParserContext& prevCtx, usize from, usize upto);

	Vec<ast::Type*> do_separated_types(ParserContext& prevCtx, usize from, usize upto);

	ast::PlainInitialiser* do_plain_initialiser(ParserContext& prevCtx, ast::TypeLike type, usize from, usize upto);
};

} // namespace qat::parser

#endif
