#ifndef QAT_PARSER_PARSER_HPP
#define QAT_PARSER_PARSER_HPP

#include "../ast/argument.hpp"
#include "../ast/box.hpp"
#include "../ast/bring_entities.hpp"
#include "../ast/bring_paths.hpp"
#include "../ast/constants/string_literal.hpp"
#include "../ast/define_choice_type.hpp"
#include "../ast/define_core_type.hpp"
#include "../ast/expression.hpp"
#include "../ast/expressions/plain_initialiser.hpp"
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
  Deque<lexer::Token>*          tokens = nullptr;
  Vec<fs::path>                 broughtPaths;
  Vec<fs::path>                 memberPaths;
  std::map<usize, lexer::Token> comments;
  ParserContext                 g_ctx;

  // Filter all comments from the original token sequence and set a new
  // sequence that maps comments to the relevant AST members
  //
  // This will be used for documentation of source code
  void filterComments();

public:
  Parser();
  ~Parser();

  static u64                            timeInMicroSeconds;
  u64                                   parseRecurseCount = 0;
  std::chrono::system_clock::time_point latestStartTime   = std::chrono::high_resolution_clock::now();

  void clearBroughtPaths();
  void setTokens(Deque<lexer::Token>* tokens);
  void parseCoreType(ParserContext& prev_ctx, usize from, usize upto, ast::DefineCoreType* coreTy);
  void parseMixType(ParserContext& prev_ctx, usize from, usize upto, Vec<Pair<Identifier, Maybe<ast::QatType*>>>& uRef,
                    Vec<FileRange>& fileRanges, Maybe<usize>& defaultVal);
  void parseChoiceType(usize from, usize upto, Vec<Pair<Identifier, Maybe<ast::DefineChoiceType::Value>>>& fields,
                       Maybe<usize>& defaultVal);
  void parseMatchContents(ParserContext& prev_ctx, usize from, usize upto,
                          Vec<Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>>& chain,
                          Maybe<Pair<Vec<ast::Sentence*>, FileRange>>& elseCase, bool isTypeMatch);
  exitFn void Error(const String& message, const FileRange& fileRange);
  static void Warning(const String& message, const FileRange& fileRange);

  useit ast::BringEntities* parseBroughtEntities(ParserContext& ctx, usize from, usize upto);
  useit ast::BringPaths* parseBroughtPaths(bool isMember, usize from, usize upto, utils::VisibilityKind kind,
                                           const FileRange& start);
  useit Vec<fs::path>& getBroughtPaths();
  useit Vec<fs::path>& getMemberPaths();
  void                 clearMemberPaths();
  useit ast::ModInfo* parseModuleInfo(usize from, usize upto, const FileRange& startRange);
  useit Pair<utils::VisibilityKind, usize> parseVisibilityKind(usize from);
  useit Pair<ast::QatType*, usize> parseType(ParserContext& prev_ctx, usize from, Maybe<usize> upto);
  useit Vec<ast::Node*> parse(ParserContext prevCtx = ParserContext(), usize from = -1, usize upto = -1);
  useit Pair<CacheSymbol, usize> parseSymbol(ParserContext& prev_ctx, usize start);
  useit Pair<Vec<ast::Argument*>, bool> parseFunctionParameters(ParserContext& prev_ctx, usize from, usize upto);
  useit Pair<ast::Expression*, usize> parseExpression(ParserContext& prev_ctx, const Maybe<CacheSymbol>& symbol,
                                                      usize from, Maybe<usize> upto, bool isMemberFn = false,
                                                      Vec<ast::Expression*> cachedExpressions = {});
  useit Vec<ast::Expression*> parseSeparatedExpressions(ParserContext& prev_ctx, usize from, usize upto);
  useit Vec<ast::Sentence*> parseSentences(ParserContext& prev_ctx, usize from, usize upto, bool onlyOne = false,
                                           bool isMemberFn = false);
  useit bool                isPrev(lexer::TokenType type, usize current);
  useit bool                isNext(lexer::TokenType type, usize current);
  useit bool                areOnlyPresentWithin(const Vec<lexer::TokenType>& kinds, usize from, usize upto);
  useit bool                isPrimaryWithin(lexer::TokenType candidate, usize from, usize upto);
  useit Maybe<usize> getPairEnd(lexer::TokenType startType, lexer::TokenType endType, usize current, bool respectScope);
  useit Maybe<usize> firstPrimaryPosition(lexer::TokenType candidate, usize from);
  useit Vec<usize> primaryPositionsWithin(lexer::TokenType candidate, usize from, usize upto);
  useit Vec<ast::GenericAbstractType*> parseGenericAbstractTypes(ParserContext& preCtx, usize from, usize upto);
  useit Vec<ast::QatType*> parseSeparatedTypes(ParserContext& prev_ctx, usize from, usize upto);
  useit ast::PlainInitialiser* parsePlainInitialiser(ParserContext& ctx, ast::QatType* type, usize from, usize upto);
};

} // namespace qat::parser

#endif