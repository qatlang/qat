#ifndef QAT_PARSER_PARSER_HPP
#define QAT_PARSER_PARSER_HPP

#include "../ast/argument.hpp"
#include "../ast/box.hpp"
#include "../ast/bring_entities.hpp"
#include "../ast/expression.hpp"
#include "../ast/expressions/string_literal.hpp"
#include "../ast/sentence.hpp"
#include "../cli/color.hpp"
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
  Vec<lexer::Token> tokens;
  // Comments mapped to indices of the next AST member in the original
  // analysed sequence
  std::map<unsigned, lexer::Token> comments;
  // Time taken by the parser to completely parse all tokens
  // in the current IO::QatFile
  u64 time_in_nanoseconds = 0;
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
  void setTokens(const Vec<lexer::Token> &tokens);

  // Parse brought entities in a "bring" sentence
  ast::BringEntities *parseBroughtEntities(ParserContext &ctx, usize from,
                                           usize to);

  /**
   * Parse all files or folders brought into this module
   *
   * @param tokens Reference to the deque containing the tokens
   * @return Vec<String> Relative URL of all files brought into
   * this file
   */
  Vec<String> parseBroughtFilesOrFolders(usize from, usize to);

  /**
   *  Parse a type represented by the next series of tokens
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which parsing starts
   * @param to The position before which parsing stops
   * @return Pair<ast::QatType, usize> The pair containing QatType
   * obtained and the index at which the parser stopped
   */
  Pair<ast::QatType *, usize> parseType(ParserContext &prev_ctx, usize from,
                                        Maybe<usize> to);

  // Top-level parsing function
  Vec<ast::Node *> parse(ParserContext prevCtx = ParserContext(),
                         usize from = -1, usize to = -1);

  /**
   *  Parse a symbol represented by the next series of tokens
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param start The position where parsing starts
   * @return Pair<CacheSymbol, usize> The pair containing CacheSymbol
   * obtained and the index at which the parser stopped
   */
  Pair<CacheSymbol, usize> parseSymbol(ParserContext &prev_ctx, usize start);

  /**
   *  Parse the contents of a CoreType
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which the parsing starts
   * @param to The position before which the parsing ends
   * @param name The name of the CoreType in question
   * @return Vec<ast::Node> Vector of nodes parsed within the
   * CoreType
   */
  Vec<ast::Node *> parseCoreTypeContents(ParserContext &prev_ctx, usize from,
                                         usize to, const String &name);

  /**
   *  Parse the parameters of a function
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which the parsing starts
   * @param to The position before which the parsing ends
   * @return results::FunctionParameters All parameters obtained from the
   * parsing
   */
  Pair<Vec<ast::Argument *>, bool>
  parseFunctionParameters(ParserContext &prev_ctx, usize from, usize to);

  /**
   *  Parse an expression. This returns only a single expression. Because
   * of the nature of expressions, multiple expressions can be nested into one
   * expression
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param symbol An optional CacheSymbol instance
   * @param from The position after which the parsing starts
   * @param to The position before which the parsing ends
   * @return ast::Expression An expression obtained from the expression
   */
  ast::Expression *parseExpression(ParserContext            &prev_ctx,
                                   const Maybe<CacheSymbol> &symbol, usize from,
                                   usize to);

  /**
   *  Parse a series of expressions separated by a separator (comma).
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which the parsing starts
   * @param to The position before which the parsing ends
   * @return Vec<ast::Expression> Multiple Expressions parsed
   */
  Vec<ast::Expression *> parseSeparatedExpressions(ParserContext &prev_ctx,
                                                   usize from, usize to);

  /**
   *  Parse multiple sentences
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which the parsing starts
   * @param to The position before which the parsing ends
   * @return Vec<ast::Sentence>
   */
  Vec<ast::Sentence *> parseSentences(ParserContext &prev_ctx, usize from,
                                      usize to);

  /**
   *  Get the end token belonging to a token pair.
   *
   * @param startType The first token nature of the pair
   * @param endType The second token nature of the pair
   * @param current The current position - Parsing will start one position after
   * this
   * @param respectScope Whether the parser should respect the presence of
   * tokens that can limit the scope of other tokens and objects.
   * @return Maybe<usize> Have to be manually checked for
   * validity
   */
  Maybe<usize> getPairEnd(lexer::TokenType startType, lexer::TokenType endType,
                          usize current, bool respectScope);

  /**
   *  Whether the previous token respective to the current position is of
   * the specified type
   *
   * @param type The TokenType to be checked for.
   * @param current The current position. The check will happen one position
   * before this.
   * @return true - If the previous position contains the specified position.
   * @return false - If the previous position is of a different type.
   */
  bool isPrev(lexer::TokenType type, usize current);

  /**
   *  Whether the next token respective to the current position is of the
   * specified type
   *
   * @param type The TokenType to be checked for.
   * @param current The current position. The check will happen one position
   * after this.
   * @return true - If the next position contains the specified position.
   * @return false - If the next position is of a different type.
   */
  bool isNext(lexer::TokenType type, usize current);

  /**
   *  Whether the tokens found within the range (from, to) are only
   * of the TokenType present in the vector
   *
   * @param kinds Vector of TokenType to be checked within the range
   * @param from The starting position of the range - exclusive
   * @param to The ending position of the range - exclusive
   * @return true If no other TokenType is found in the range
   * @return false If other TokenType is found in the range
   */
  bool areOnlyPresentWithin(const Vec<lexer::TokenType> &kinds, usize from,
                            usize to);

  /**
   *  Whether the provided token has a first degree presence within the
   * range. That is, the presence of the token is not nested within other scope
   * limiting tokens
   *
   * @param candidate The token to check for
   * @param from The position after which parsing starts
   * @param to The position before which parsing ends
   * @return true If the token is primary
   * @return false IF the token is not primary or not present at all
   */
  bool isPrimaryWithin(lexer::TokenType candidate, usize from, usize to);

  /**
   *  If the provided token has a primary position, find the first such
   * position
   *
   * @param candidate The token to look for
   * @param from The position after which parsing starts
   * @return Maybe<usize> An optional index position of the found
   * token
   */
  Maybe<usize> firstPrimaryPosition(lexer::TokenType candidate, usize from);

  /**
   *  If the provided token has one or more primary positions within the
   * provided range, find all such positions
   *
   * @param candidate The token to look for
   * @param from The position after which parsing starts
   * @param to The position before which parsing ends
   * @return Vec<usize> All positions found. This will be empty if
   * there is no primary position for the token in the range
   */
  Vec<usize> primaryPositionsWithin(lexer::TokenType candidate, usize from,
                                    usize to);

  /**
   *  Throws error with a formatted error message and exits the program
   *
   * @param message Custom message
   * @param fileRange Position in the file corresponding to the error
   */
  void throwError(const String &message, const utils::FileRange &fileRange);

  // Shows warning to the user
  static void showWarning(const String           &message,
                          const utils::FileRange &fileRange);
};

} // namespace qat::parser

#endif