#ifndef QAT_PARSER_PARSER_HPP
#define QAT_PARSER_PARSER_HPP

#include "../ast/meta_info.hpp"
#include "../lexer/token.hpp"
#include "../lexer/token_type.hpp"
#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "./cache_symbol.hpp"
#include "./parser_context.hpp"

#include <chrono>
#include <map>
#include <optional>

namespace qat::ast {

class PlainInitialiser;
class Argument;
class MatchValue;
class Sentence;
class MemberParentLike;
class FillGeneric;

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
	Vec<lexer::Token>*            tokens = nullptr;
	Vec<fs::path>                 broughtPaths;
	Vec<fs::path>                 memberPaths;
	std::map<usize, lexer::Token> comments;
	ParserContext                 g_ctx;
	ir::Ctx*                      irCtx;

	// Filter all comments from the original token sequence and set a new
	// sequence that maps comments to the relevant AST members
	//
	// This will be used for documentation of source code
	void filter_comments();

  public:
	explicit Parser(ir::Ctx* irCtx);
	useit static Parser* get(ir::Ctx* irCtx);
	~Parser();

	static u64 timeInMicroSeconds;
	static u64 tokenCount;
	u64        parseRecurseCount = 0;

	std::chrono::high_resolution_clock::time_point latestStartTime = std::chrono::high_resolution_clock::now();

	void clear_brought_paths();

	void set_tokens(Vec<lexer::Token>* tokens);

	void do_type_contents(ParserContext& prev_ctx, usize from, usize upto, ast::MemberParentLike* coreTy);

	void parse_mix_type(ParserContext& prev_ctx, usize from, usize upto, Vec<Pair<Identifier, Maybe<ast::Type*>>>& uRef,
	                    Vec<FileRange>& fileRanges, Maybe<usize>& defaultVal);

	void do_choice_type(usize from, usize upto, Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>>& fields,
	                    Maybe<usize>& defaultVal);

	void parse_match_contents(ParserContext& prev_ctx, usize from, usize upto,
	                          Vec<Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>>& chain,
	                          Maybe<Pair<Vec<ast::Sentence*>, FileRange>>& elseCase, bool isTypeMatch);

	void add_error(const String& message, const FileRange& fileRange);

	String color_error(const String& message);

	static void add_warning(const String& message, const FileRange& fileRange);

	useit bool is_previous(lexer::TokenType type, usize current);

	useit bool is_next(lexer::TokenType type, usize current) {
		if ((current + 1) < tokens->size()) {
			return tokens->at(current + 1).type == type;
		} else {
			return false;
		}
	}

	useit bool are_only_present_within(const Vec<lexer::TokenType>& kinds, usize from, usize upto);

	useit bool is_primary_within(lexer::TokenType candidate, usize from, usize upto);

	useit ast::BringEntities* parse_bring_entities(ParserContext& ctx, Maybe<ast::VisibilitySpec> visibKind, usize from,
	                                               usize upto);

	useit ast::BringPaths* parse_bring_paths(bool isMember, usize from, usize upto, Maybe<ast::VisibilitySpec> spec,
	                                         const FileRange& start);

	useit Vec<fs::path>& get_brought_paths();

	useit Vec<fs::path>& get_member_paths();

	void clear_member_paths();

	useit EntityMetadata do_entity_metadata(ParserContext& parserCtx, usize from, String entityType,
	                                        usize genericLength);

	useit ast::MetaInfo do_meta_info(usize from, usize upto, FileRange fileRange);

	useit Pair<ast::VisibilitySpec, usize> do_visibility_kind(usize from);

	useit Vec<ast::FillGeneric*> do_generic_fill(ParserContext& prevCtx, usize from, usize upto);

	useit Pair<ast::Type*, usize> do_type(ParserContext& prevCtx, usize from, Maybe<usize> upto);

	useit Vec<ast::Node*> parse(ParserContext prevCtx = ParserContext(), usize from = -1, usize upto = -1);

	useit Pair<CacheSymbol, usize> do_symbol(ParserContext& prevCtx, usize start);

	useit Pair<Vec<ast::Argument*>, bool> do_function_parameters(ParserContext& prevCtx, usize from, usize upto);

	useit Pair<ast::PrerunExpression*, usize> do_prerun_expression(ParserContext& prevCtx, usize from,
	                                                               Maybe<usize> upto, bool returnOnFirstExp = false);

	useit Pair<ast::Expression*, usize> do_expression(ParserContext& prevCtx, const Maybe<CacheSymbol>& symbol,
	                                                  usize from, Maybe<usize> upto,
	                                                  Maybe<ast::Expression*> cachedExpressions = None,
	                                                  bool                    returnAtFirstExp  = false);

	useit Vec<ast::Expression*> do_separated_expressions(ParserContext& prevCtx, usize from, usize upto);

	useit Vec<ast::PrerunExpression*> do_separated_prerun_expressions(ParserContext& prevCtx, usize from, usize upto);

	useit Vec<ast::Sentence*> do_sentences(ParserContext& prevCtx, usize from, usize upto);

	useit Maybe<usize> get_pair_end(lexer::TokenType startType, lexer::TokenType endType, usize current);

	useit Maybe<usize> first_primary_position(lexer::TokenType candidate, usize from);

	useit Vec<usize> primary_positions_within(lexer::TokenType candidate, usize from, usize upto);

	useit Vec<ast::GenericAbstractType*> do_generic_abstracts(ParserContext& prevCtx, usize from, usize upto);

	useit Vec<ast::Type*> do_separated_types(ParserContext& prevCtx, usize from, usize upto);

	useit ast::PlainInitialiser* do_plain_initialiser(ParserContext& prevCtx, Maybe<ast::Type*> type, usize from,
	                                                  usize upto);
};

} // namespace qat::parser

#endif
