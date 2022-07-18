#ifndef QAT_PARSER_PARSER_HPP
#define QAT_PARSER_PARSER_HPP

#include "../ast/box.hpp"
#include "../ast/expression.hpp"
#include "../ast/expressions/binary_expression.hpp"
#include "../ast/expressions/custom_float_literal.hpp"
#include "../ast/expressions/custom_integer_literal.hpp"
#include "../ast/expressions/entity.hpp"
#include "../ast/expressions/float_literal.hpp"
#include "../ast/expressions/function_call.hpp"
#include "../ast/expressions/integer_literal.hpp"
#include "../ast/expressions/member_function_call.hpp"
#include "../ast/expressions/null_pointer.hpp"
#include "../ast/expressions/string_literal.hpp"
#include "../ast/expressions/to_conversion.hpp"
#include "../ast/expressions/tuple_value.hpp"
#include "../ast/expressions/unsigned_literal.hpp"
#include "../ast/function_definition.hpp"
#include "../ast/function_prototype.hpp"
#include "../ast/node.hpp"
#include "../ast/sentences/assignment.hpp"
#include "../ast/sentences/expression_sentence.hpp"
#include "../ast/sentences/give_sentence.hpp"
#include "../ast/sentences/local_declaration.hpp"
#include "../ast/sentences/say_sentence.hpp"
#include "../ast/types/array.hpp"
#include "../ast/types/float.hpp"
#include "../ast/types/integer.hpp"
#include "../ast/types/named.hpp"
#include "../ast/types/pointer.hpp"
#include "../ast/types/qat_type.hpp"
#include "../ast/types/reference.hpp"
#include "../ast/types/tuple.hpp"
#include "../ast/types/unsigned.hpp"
#include "../cli/color.hpp"
#include "../lexer/token.hpp"
#include "../lexer/token_type.hpp"
#include "../utils/types.hpp"
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

/**
 *  Parser handles parsing of all tokens analysed by the lexer
 *
 * This is divided to separate functions
 *
 */
class Parser {
private:
  /**
   *  Reference to a std::deque<lexer::Token> which is
   * usually a member of the corresponding IO::QatFile
   *
   */
  std::vector<lexer::Token> tokens;

  /**
   *  Comments mapped to indices of the next AST member in the original
   * analysed sequence
   *
   */
  std::map<unsigned, lexer::Token> comments;

  /**
   *  Time taken by the parser to completely parse all tokens
   * in the current IO::QatFile
   *
   */
  unsigned long long time_in_nanoseconds = 0;

  /**
   *  Context of this parser. This is specific to the current file and
   * changes when the file changes
   *
   */
  ParserContext g_ctx;

  /**
   *  Filter all comments from the original token sequence and set a new
   * sequence that maps comments to the relevant AST members
   *
   * This will be used for documenation of source code
   *
   */
  void filterComments();

public:
  /**
   *  Parser handles parsing of all tokens analysed by the lexer
   *
   * This is divided to separate functions
   *
   */
  Parser();

  /**
   *  Set the Tokens object
   *
   * @param tokens Reference to the vector containing the tokens
   */
  void setTokens(const std::vector<lexer::Token> tokens);

  std::vector<std::string> parseBroughtEntities(ParserContext &ctx,
                                                const std::size_t from,
                                                const std::size_t to);

  /**
   *  Parse all files or folders brought into this module
   *
   * @param tokens Reference to the deque containing the tokens
   * @return std::vector<std::string> Relative URL of all files brought into
   * this file
   */
  std::vector<std::string> parseBroughtFilesOrFolders(const std::size_t from,
                                                      const std::size_t to);

  /**
   *  Parse a type represented by the next series of tokens
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which parsing starts
   * @param to The position before which parsing stops
   * @return std::pair<ast::QatType, std::size_t> The pair containing QatType
   * obtained and the index at which the parser stopped
   */
  std::pair<ast::QatType *, std::size_t>
  parseType(ParserContext &prev_ctx, const std::size_t from,
            const std::optional<std::size_t> to);

  // NOTE - Change to accept a range so as for container-like contexts
  std::vector<ast::Node *> parse(ParserContext prevCtx = ParserContext(),
                                 std::size_t from = -1, std::size_t to = -1);

  /**
   *  Parse a symbol represented by the next series of tokens
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param start The position where parsing starts
   * @return std::pair<CacheSymbol, std::size_t> The pair containing CacheSymbol
   * obtained and the index at which the parser stopped
   */
  std::pair<CacheSymbol, std::size_t> parseSymbol(ParserContext &prev_ctx,
                                                  const std::size_t start);

  /**
   *  Parse the contents of a CoreType
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which the parsing starts
   * @param to The position before which the parsing ends
   * @param name The name of the CoreType in question
   * @return std::vector<ast::Node> Vector of nodes parsed within the
   * CoreType
   */
  std::vector<ast::Node *> parseCoreTypeContents(ParserContext &prev_ctx,
                                                 const std::size_t from,
                                                 const std::size_t to,
                                                 const std::string name);

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
  std::pair<std::vector<ast::Argument *>, bool>
  parseFunctionParameters(ParserContext &prev_ctx, const std::size_t from,
                          const std::size_t to);

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
  ast::Expression *parseExpression(ParserContext &prev_ctx,
                                   const llvm::Optional<CacheSymbol> symbol,
                                   const std::size_t from,
                                   const std::size_t to);

  /**
   *  Parse a series of expressions separated by a separator (comma).
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which the parsing starts
   * @param to The position before which the parsing ends
   * @return std::vector<ast::Expression> Multiple Expressions parsed
   */
  std::vector<ast::Expression *>
  parseSeparatedExpressions(ParserContext &prev_ctx, const std::size_t from,
                            const std::size_t to);

  /**
   *  Parse multiple sentences
   *
   * @param prev_ctx Reference to the ParserContext instance of the previous
   * scope
   * @param from The position after which the parsing starts
   * @param to The position before which the parsing ends
   * @return std::vector<ast::Sentence>
   */
  std::vector<ast::Sentence *> parseSentences(ParserContext &prev_ctx,
                                              const std::size_t from,
                                              const std::size_t to);

  /**
   *  Get the end token belonging to a token pair.
   *
   * @param startType The first token kind of the pair
   * @param endType The second token kind of the pair
   * @param current The current position - Parsing will start one position after
   * this
   * @param respectScope Whether the parser should respect the presence of
   * tokens that can limit the scope of other tokens and objects.
   * @return llvm::Optional<std::size_t> Have to be manually checked for
   * validity
   */
  llvm::Optional<std::size_t> getPairEnd(const lexer::TokenType startType,
                                         const lexer::TokenType endType,
                                         const std::size_t current,
                                         const bool respectScope);
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
  bool isPrev(const lexer::TokenType type, const std::size_t current);

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
  bool isNext(const lexer::TokenType type, const std::size_t current);

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
  bool areOnlyPresentWithin(const std::vector<lexer::TokenType> kinds,
                            const std::size_t from, const std::size_t to);

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
  bool isPrimaryWithin(const lexer::TokenType candidate, const std::size_t from,
                       const std::size_t to);

  /**
   *  If the provided token has a primary position, find the first such
   * position
   *
   * @param candidate The token to look for
   * @param from The position after which parsing starts
   * @return llvm::Optional<std::size_t> An optional index position of the found
   * token
   */
  llvm::Optional<std::size_t>
  firstPrimaryPosition(const lexer::TokenType candidate,
                       const std::size_t from);

  /**
   *  If the provided token has one or more primary positions within the
   * provided range, find all such positions
   *
   * @param candidate The token to look for
   * @param from The position after which parsing starts
   * @param to The position before which parsing ends
   * @return std::vector<std::size_t> All positions found. This will be empty if
   * there is no primary position for the token in the range
   */
  std::vector<std::size_t>
  primaryPositionsWithin(const lexer::TokenType candidate,
                         const std::size_t from, const std::size_t to);

  /**
   *  Throws error with a formatted error message and exits the program
   *
   * @param message Custom message
   * @param fileRange Position in the file corresponding to the error
   */
  void throw_error(const std::string message, const utils::FileRange fileRange);
};

} // namespace qat::parser

#endif