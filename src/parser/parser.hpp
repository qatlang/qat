#ifndef QAT_PARSER_PARSER_HPP
#define QAT_PARSER_PARSER_HPP

#include "../ast/argument.hpp"
#include "../ast/box.hpp"
#include "../ast/bring_entities.hpp"
#include "../ast/bring_paths.hpp"
#include "../ast/define_choice_type.hpp"
#include "../ast/define_core_type.hpp"
#include "../ast/expression.hpp"
#include "../ast/expressions/plain_initialiser.hpp"
#include "../ast/generics.hpp"
#include "../ast/meta_info.hpp"
#include "../ast/prerun/string_literal.hpp"
#include "../ast/sentence.hpp"
#include "../ast/sentences/match.hpp"
#include "../lexer/token.hpp"
#include "../lexer/token_type.hpp"
#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "./cache_symbol.hpp"
#include "./parser_context.hpp"
#include "./token_family.hpp"
#include <chrono>
#include <deque>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

namespace qat::parser {

//  Parser handles parsing of all tokens analysed by the lexer
class Parser {
private:
  Vec<lexer::Token>*            tokens = nullptr;
  Vec<fs::path>                 broughtPaths;
  Vec<fs::path>                 memberPaths;
  std::map<usize, lexer::Token> comments;
  ParserContext                 g_ctx;
  IR::Context*                  irCtx;

  // Filter all comments from the original token sequence and set a new
  // sequence that maps comments to the relevant AST members
  //
  // This will be used for documentation of source code
  void filter_comments();

public:
  explicit Parser(IR::Context* irCtx);
  useit static Parser* get(IR::Context* ctx);
  ~Parser();

  static u64 timeInMicroSeconds;
  u64        parseRecurseCount = 0;

  std::chrono::high_resolution_clock::time_point latestStartTime = std::chrono::high_resolution_clock::now();

  void clear_brought_paths();
  void set_tokens(Vec<lexer::Token>* tokens);
  void do_type_contents(ParserContext& prev_ctx, usize from, usize upto, ast::MemberParentLike* coreTy);
  void parse_mix_type(ParserContext& prev_ctx, usize from, usize upto,
                      Vec<Pair<Identifier, Maybe<ast::QatType*>>>& uRef, Vec<FileRange>& fileRanges,
                      Maybe<usize>& defaultVal);
  void do_choice_type(usize from, usize upto, Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>>& fields,
                      Maybe<usize>& defaultVal);
  void parse_match_contents(ParserContext& prev_ctx, usize from, usize upto,
                            Vec<Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>>& chain,
                            Maybe<Pair<Vec<ast::Sentence*>, FileRange>>& elseCase, bool isTypeMatch);

  void        add_error(const String& message, const FileRange& fileRange);
  String      color_error(const String& message, const char* color = colors::bold::yellow);
  static void add_warning(const String& message, const FileRange& fileRange);

  useit bool is_previous(lexer::TokenType type, usize current);
  useit bool is_next(lexer::TokenType type, usize current);
  useit bool are_only_present_within(const Vec<lexer::TokenType>& kinds, usize from, usize upto);
  useit bool is_primary_within(lexer::TokenType candidate, usize from, usize upto);

  useit ast::BringEntities* parse_bring_entities(ParserContext& ctx, Maybe<ast::VisibilitySpec> visibKind, usize from,
                                                 usize upto);
  useit ast::BringPaths* parse_bring_paths(bool isMember, usize from, usize upto, Maybe<ast::VisibilitySpec> spec,
                                           const FileRange& start);
  useit Vec<fs::path>& get_brought_paths();
  useit Vec<fs::path>& get_member_paths();
  void                 clear_member_paths();
  useit ast::MetaInfo do_meta_info(usize from, usize upto, FileRange fileRange);
  useit Pair<ast::VisibilitySpec, usize> do_visibility_kind(usize from);
  useit Vec<ast::FillGeneric*> do_generic_fill(ParserContext& prev_ctx, usize from, usize upto);
  useit Pair<ast::QatType*, usize> do_type(ParserContext& prev_ctx, usize from, Maybe<usize> upto);
  useit Vec<ast::Node*> parse(ParserContext prevCtx = ParserContext(), usize from = -1, usize upto = -1);
  useit Pair<CacheSymbol, usize> do_symbol(ParserContext& prev_ctx, usize start);
  useit Pair<Vec<ast::Argument*>, bool> do_function_parameters(ParserContext& prev_ctx, usize from, usize upto);
  useit Pair<ast::PrerunExpression*, usize> do_prerun_expression(ParserContext& preCtx, usize from, Maybe<usize> upto,
                                                                 bool returnOnFirstExp = false);
  useit Pair<ast::Expression*, usize> do_expression(ParserContext& prev_ctx, const Maybe<CacheSymbol>& symbol,
                                                    usize from, Maybe<usize> upto,
                                                    Maybe<ast::Expression*> cachedExpressions = None,
                                                    bool                    returnAtFirstExp  = false);
  useit Vec<ast::Expression*> do_separated_expressions(ParserContext& prev_ctx, usize from, usize upto);
  useit Vec<ast::PrerunExpression*> do_separated_prerun_expressions(ParserContext& prev_ctx, usize from, usize upto);
  useit Vec<ast::Sentence*> do_sentences(ParserContext& prev_ctx, usize from, usize upto);
  //   useit bool                isNotPartOfExpression(usize from, usize upto);
  useit Maybe<usize> get_pair_end(lexer::TokenType startType, lexer::TokenType endType, usize current);
  useit Maybe<usize> first_primary_position(lexer::TokenType candidate, usize from);
  useit Vec<usize> primary_positions_within(lexer::TokenType candidate, usize from, usize upto);
  useit Vec<ast::GenericAbstractType*> do_generic_abstracts(ParserContext& preCtx, usize from, usize upto);
  useit Vec<ast::QatType*> do_separated_types(ParserContext& prev_ctx, usize from, usize upto);
  useit ast::PlainInitialiser* do_plain_initialiser(ParserContext& ctx, Maybe<ast::QatType*> type, usize from,
                                                    usize upto);
};

} // namespace qat::parser

#endif