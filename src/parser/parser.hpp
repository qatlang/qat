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
  //  Reference to a std::deque<lexer::Token> which is
  // usually a member of the corresponding IO::QatFile
  Deque<lexer::Token>* tokens;
  Vec<fs::path>        broughtPaths;
  // Comments mapped to indices of the next AST member in the original
  // analysed sequence
  std::map<usize, lexer::Token> comments;
  // Time taken by the parser to completely parse all tokens
  // in the current IO::QatFile
  u64 timeInNanoSeconds = 0;
  // Context of this parser. This is specific to the current file and
  // changes when the file changes
  ParserContext g_ctx;

  // Filter all comments from the original token sequence and set a new
  // sequence that maps comments to the relevant AST members
  //
  // This will be used for documentation of source code
  void filterComments();

public:
  // Parser handles parsing of all tokens analysed by the lexer
  Parser();

  // Set the Tokens object
  void setTokens(Deque<lexer::Token>* tokens);

  // Parse brought entities in a "bring" sentence
  useit ast::BringEntities* parseBroughtEntities(ParserContext& ctx, usize from, usize upto);

  // Parse all files or folders brought into this module
  useit ast::BringPaths* parseBroughtPaths(usize from, usize upto, utils::VisibilityKind kind,
                                           const utils::FileRange& start);

  useit Vec<fs::path>& getBroughtPaths();

  void clearBroughtPaths();

  useit Pair<utils::VisibilityKind, usize> parseVisibilityKind(usize from);

  // Parse a type represented by the next series of tokens
  useit Pair<ast::QatType*, usize> parseType(ParserContext& prev_ctx, usize from, Maybe<usize> upto);

  // Top-level parsing function
  useit Vec<ast::Node*> parse(ParserContext prevCtx = ParserContext(), usize from = -1, usize upto = -1);

  // Parse a symbol represented by the next series of tokens
  useit Pair<CacheSymbol, usize> parseSymbol(ParserContext& prev_ctx, usize start);

  // Parse the contents of a CoreType
  void parseCoreType(ParserContext& prev_ctx, usize from, usize upto, ast::DefineCoreType* coreTy);

  void parseMixType(ParserContext& prev_ctx, usize from, usize upto, Vec<Pair<String, Maybe<ast::QatType*>>>& uRef,
                    Vec<utils::FileRange>& fileRanges, Maybe<usize>& defaultVal);

  void parseChoiceType(usize from, usize upto,
                       Vec<Pair<ast::DefineChoiceType::Field, Maybe<ast::DefineChoiceType::Value>>>& fields,
                       Maybe<usize>&                                                                 defaultVal);

  void parseMatchContents(ParserContext& prev_ctx, usize from, usize upto,
                          Vec<Pair<ast::MatchValue*, Vec<ast::Sentence*>>>& chain, Maybe<Vec<ast::Sentence*>>& elseCase,
                          bool isTypeMatch);

  // Parse the parameters of a function
  useit Pair<Vec<ast::Argument*>, bool> parseFunctionParameters(ParserContext& prev_ctx, usize from, usize upto);

  // Parse an expression. This returns only a single expression. Because
  // of the nature of expressions, multiple expressions can be nested into one
  // expression
  useit Pair<ast::Expression*, usize> parseExpression(ParserContext& prev_ctx, const Maybe<CacheSymbol>& symbol,
                                                      usize from, Maybe<usize> upto, bool isMemberFn = false,
                                                      Vec<ast::Expression*> cachedExpressions = {});

  // Parse a series of expressions separated by a separator (comma).
  useit Vec<ast::Expression*> parseSeparatedExpressions(ParserContext& prev_ctx, usize from, usize upto);

  // Parse multiple sentences
  useit Vec<ast::Sentence*> parseSentences(ParserContext& prev_ctx, usize from, usize upto, bool onlyOne = false,
                                           bool isMemberFn = false);

  // Get the end token belonging to a token pair.
  useit Maybe<usize> getPairEnd(lexer::TokenType startType, lexer::TokenType endType, usize current, bool respectScope);

  // Whether the previous token respective to the current position is of
  // the specified type
  useit bool isPrev(lexer::TokenType type, usize current);

  // Whether the next token respective to the current position is of the
  // specified type
  useit bool isNext(lexer::TokenType type, usize current);

  // Whether the tokens found within the range (from, to) are only
  // of the TokenType present in the vector
  useit bool areOnlyPresentWithin(const Vec<lexer::TokenType>& kinds, usize from, usize upto);

  // Whether the provided token has a first degree presence within the
  // range. That is, the presence of the token is not nested within other scope
  // limiting tokens
  useit bool isPrimaryWithin(lexer::TokenType candidate, usize from, usize upto);

  // If the provided token has a primary position, find the first such
  // position
  useit Maybe<usize> firstPrimaryPosition(lexer::TokenType candidate, usize from);

  // If the provided token has one or more primary positions within the
  // provided range, find all such positions
  useit Vec<usize> primaryPositionsWithin(lexer::TokenType candidate, usize from, usize upto);

  useit Vec<ast::TemplatedType*> parseTemplateTypes(usize from, usize upto);

  useit Vec<ast::QatType*> parseSeparatedTypes(ParserContext& prev_ctx, usize from, usize upto);

  useit ast::PlainInitialiser* parsePlainInitialiser(ParserContext& ctx, ast::QatType* type, usize from, usize upto);

  // Throws error with a formatted error message and exits the program
  void Error(const String& message, const utils::FileRange& fileRange);

  // Shows warning to the user
  static void Warning(const String& message, const utils::FileRange& fileRange);
};

} // namespace qat::parser

#endif