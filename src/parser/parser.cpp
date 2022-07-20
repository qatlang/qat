#include "./parser.hpp"
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
#include "../ast/lib.hpp"
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
#include "../show.hpp"

// NOTE - Check if file fileRange values are making use of the new merge
// functionality
namespace qat::parser {

Parser::Parser() : tokens() {}

void Parser::setTokens(const std::vector<lexer::Token> allTokens) {
  g_ctx = ParserContext();
  tokens = allTokens;
}

std::vector<std::string> Parser::parseBroughtEntities(ParserContext &ctx,
                                                      const std::size_t from,
                                                      const std::size_t to) {
  using lexer::TokenType;

  std::optional<CacheSymbol> csym = std::nullopt;

  std::vector<std::string> result;
  for (std::size_t i = from + 1; i < to; i++) {
    auto token = tokens.at(i);
    switch (token.type) {
    case TokenType::identifier: {
      auto sym_res = parseSymbol(ctx, i);
      csym = sym_res.first;
      i = sym_res.second;
      break;
    }
    case TokenType::separator: {
      if (csym.has_value()) {
        result.push_back(csym.value().name);
      } else {
      }
      break;
    }
    }
  }
}

std::vector<std::string>
Parser::parseBroughtFilesOrFolders(const std::size_t from,
                                   const std::size_t to) {
  using lexer::TokenType;
  std::vector<std::string> result;
  for (std::size_t i = from + 1; (i < to) && (i < tokens.size()); i++) {
    auto token = tokens.at(i);
    switch (token.type) {
    case TokenType::StringLiteral: {
      if (isNext(TokenType::StringLiteral, i)) {
        throw_error(
            "Implicit concatenation of adjacent string literals are not "
            "supported in bring sentences, to avoid ambiguity and confusion. "
            "If this is supposed to be another file or folder to bring, then "
            "add a separator between these. Or else fix the sentence.",
            tokens.at(i + 1).fileRange);
      }
      result.push_back(token.value);
      break;
    }
    case TokenType::separator: {
      if (isNext(TokenType::StringLiteral, i) || isNext(TokenType::stop, i)) {
        continue;
      } else {
        if (isNext(TokenType::separator, i)) {
          // NOTE - This might be too much
          throw_error(
              "Multiple adjacent separators found. This is not supported to "
              "discourage code clutter. Please remove this",
              tokens.at(i + 1).fileRange);
        } else {
          throw_error("Unexpected token in bring sentence",
                      tokens.at(i).fileRange);
        }
      }
      break;
    }
    default: {
      throw_error("Unexpected token in bring sentence", tokens.at(i).fileRange);
    }
    }
  }
  return result;
}

std::pair<ast::QatType *, std::size_t>
Parser::parseType(ParserContext &prev_ctx, const std::size_t from,
                  const std::optional<std::size_t> to) {
  using lexer::Token;
  using lexer::TokenType;

  bool variable = false;
  auto getVariability = [&variable]() {
    auto value = variable;
    variable = false;
    return value;
  };

  auto ctx = ParserContext(prev_ctx);
  std::size_t i = 0;
  std::optional<ast::QatType *> cacheTy;

  for (i = from + 1; i < (to.has_value() ? to.value() : tokens.size()); i++) {
    Token &token = tokens.at(i);
    switch (token.type) {
    case TokenType::var: {
      variable = true;
      break;
    }
    case TokenType::parenthesisOpen: {
      if (cacheTy.has_value()) {
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i, false);
        if (pCloseRes.hasValue()) {
          auto hasPrimary =
              isPrimaryWithin(TokenType::semiColon, i, pCloseRes.getValue());
          if (hasPrimary) {
            throw_error(
                "Tuple type found after another type",
                utils::FileRange(token.fileRange,
                                 tokens.at(pCloseRes.getValue()).fileRange));
          } else {
            return std::pair(cacheTy.value(), i - 1);
          }
        } else {
          throw_error("Expected )", token.fileRange);
        }
      }
      auto pCloseResult = getPairEnd(TokenType::parenthesisOpen,
                                     TokenType::parenthesisClose, i, false);
      if (!pCloseResult.hasValue()) {
        throw_error("Expected )", token.fileRange);
      }
      if (to.has_value() && (pCloseResult.getValue() > to.value())) {
        throw_error("Invalid position for )",
                    tokens.at(pCloseResult.getValue()).fileRange);
      }
      auto pClose = pCloseResult.getValue();
      std::vector<ast::QatType *> subTypes;
      for (std::size_t j = i; j < pClose; j++) {
        if (isPrimaryWithin(TokenType::semiColon, j, pClose)) {
          auto semiPosResult = firstPrimaryPosition(TokenType::semiColon, j);
          if (!semiPosResult.hasValue()) {
            throw_error("Invalid position of ; separator", token.fileRange);
          }
          auto semiPos = semiPosResult.getValue();
          subTypes.push_back(parseType(ctx, j, semiPos).first);
          j = semiPos - 1;
        } else if (j != (pClose - 1)) {
          subTypes.push_back(parseType(ctx, j, pClose).first);
          j = pClose;
        }
      }
      i = pClose;
      cacheTy = new ast::TupleType(subTypes, false, getVariability(),
                                   token.fileRange);
      break;
    }
    case TokenType::voidType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      cacheTy = new ast::VoidType(getVariability(), token.fileRange);
      break;
    }
    case TokenType::boolType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      cacheTy = new ast::UnsignedType(1, getVariability(), token.fileRange);
      break;
    }
    case TokenType::unsignedIntegerType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      cacheTy = new ast::UnsignedType(std::stoi(token.value), getVariability(),
                                      token.fileRange);
      break;
    }
    case TokenType::integerType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      cacheTy = new ast::IntegerType(std::stoi(token.value), getVariability(),
                                     token.fileRange);
      break;
    }
    case TokenType::floatType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      if (token.value == "brain") {
        cacheTy = new ast::FloatType(IR::FloatTypeKind::_brain,
                                     getVariability(), token.fileRange);
      } else if (token.value == "half") {
        cacheTy = new ast::FloatType(IR::FloatTypeKind::_half, getVariability(),
                                     token.fileRange);
      } else if (token.value == "32") {
        cacheTy = new ast::FloatType(IR::FloatTypeKind::_32, getVariability(),
                                     token.fileRange);
      } else if (token.value == "64") {
        cacheTy = new ast::FloatType(IR::FloatTypeKind::_64, getVariability(),
                                     token.fileRange);
      } else if (token.value == "80") {
        cacheTy = new ast::FloatType(IR::FloatTypeKind::_80, getVariability(),
                                     token.fileRange);
      } else if (token.value == "128") {
        cacheTy = new ast::FloatType(IR::FloatTypeKind::_128, getVariability(),
                                     token.fileRange);
      } else if (token.value == "128ppc") {
        cacheTy = new ast::FloatType(IR::FloatTypeKind::_128PPC,
                                     getVariability(), token.fileRange);
      } else {
        cacheTy = new ast::FloatType(IR::FloatTypeKind::_32, getVariability(),
                                     token.fileRange);
      }
      break;
    }
    case TokenType::identifier: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      auto symRes = parseSymbol(ctx, i);
      i = symRes.second;
      if (isNext(TokenType::templateTypeStart, i)) {
        // TODO - Implement template type parsing
      }
      cacheTy = new ast::NamedType(symRes.first.name, getVariability(),
                                   token.fileRange);
      break;
    }
    case TokenType::referenceType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      auto subRes = parseType(ctx, i, to);
      i = subRes.second;
      cacheTy = new ast::ReferenceType(
          subRes.first, getVariability(),
          utils::FileRange(token.fileRange, tokens.at(i).fileRange));
      break;
    }
    case TokenType::pointerType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      if (isNext(TokenType::bracketOpen, i)) {
        auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                    TokenType::bracketClose, i + 1, false);
        if (bCloseRes.hasValue() &&
            (to.has_value() ? (bCloseRes.getValue() < to.value()) : true)) {
          auto bClose = bCloseRes.getValue();
          auto subTypeRes = parseType(ctx, i + 1, bClose);
          i = bClose;
          cacheTy = new ast::PointerType(
              subTypeRes.first, getVariability(),
              utils::FileRange(token.fileRange, tokens.at(bClose).fileRange));
          break;
        } else {
          throw_error("Invalid end for pointer type", tokens.at(i).fileRange);
        }
      } else {
        throw_error("Type of the pointer not specified", token.fileRange);
      }
      break;
    }
    case TokenType::bracketOpen: {
      if (!cacheTy.has_value()) {
        throw_error("Element type of array not specified", token.fileRange);
      }
      if (isNext(TokenType::unsignedLiteral, i) ||
          isNext(TokenType::integerLiteral, i)) {
        if (isNext(TokenType::bracketClose, i + 1)) {
          auto subType = cacheTy.value();
          auto numstr = tokens.at(i + 1).value;
          if (numstr.find('_') != std::string::npos) {
            numstr = numstr.substr(0, numstr.find('_'));
          }
          cacheTy = new ast::ArrayType(subType, std::stoul(numstr),
                                       getVariability(), token.fileRange);
          i = i + 2;
          break;
        } else {
          throw_error("Expected ] after the array size",
                      tokens.at(i + 1).fileRange);
        }
      } else {
        throw_error("Expected non negative number of elements for the array",
                    token.fileRange);
      }
      break;
    }
    default: {
      if (!cacheTy.has_value()) {
        throw_error("No type found", tokens.at(from).fileRange);
      }
      return std::pair(cacheTy.value(), i - 1);
    }
    }
  }
  if (!cacheTy.has_value()) {
    throw_error("No type found", tokens.at(from).fileRange);
  }
  return std::pair(cacheTy.value(), i - 1);
}

std::vector<ast::Node *> Parser::parse(ParserContext prev_ctx, std::size_t from,
                                       std::size_t to) {
  std::vector<ast::Node *> result;
  using lexer::Token;
  using lexer::TokenType;

  if (to == -1) {
    to = tokens.size();
  }

  ParserContext ctx(prev_ctx);

  std::optional<CacheSymbol> c_sym = std::nullopt;
  auto setCachedSymbol = [&c_sym](CacheSymbol sym) { c_sym = sym; };
  auto getCachedSymbol = [&c_sym]() {
    auto result = c_sym.value();
    c_sym = std::nullopt;
    return result;
  };
  auto hasCachedSymbol = [&c_sym]() { return c_sym.has_value(); };

  std::deque<Token> cacheT;
  std::deque<ast::QatType *> cacheTy;
  std::string context = "global";

  bool isVar = false;
  auto getVar = [&isVar]() {
    auto result = isVar;
    isVar = false;
    return result;
  };
  auto setVar = [&isVar]() { isVar = true; };

  for (std::size_t i = (from + 1); i < to; i++) {
    Token &token = tokens.at(i);
    switch (token.type) {
    case TokenType::endOfFile: {
      return result;
    }
    case TokenType::parenthesisOpen:
    case TokenType::boolType:
    case TokenType::voidType:
    case TokenType::integerType:
    case TokenType::pointerType:
    case TokenType::floatType: {
      auto typeResult = parseType(ctx, i - 1, std::nullopt);
      cacheTy.push_back(typeResult.first);
      i = typeResult.second;
      break;
    }
    case TokenType::Type: {
      // TODO - Consider other possible tokens and template types instead of
      // just identifiers
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::assignment, i + 1)) {
          // TODO: Add support for type definitions, sum types and more
        } else if (isNext(TokenType::curlybraceOpen, i + 1)) {
          auto bClose = getPairEnd(TokenType::curlybraceOpen,
                                   TokenType::curlybraceClose, i + 1, true);
          if (bClose.hasValue()) {
            auto tRes = parseCoreTypeContents(ctx, i + 2, bClose.getValue(),
                                              tokens.at(i + 1).value);
            // FIXME - Handle this scenario
          } else {
            throw_error("Invalid end for declaring a type",
                        tokens.at(i + 2).fileRange);
          }
          i = bClose.getValue();
        }
      } else if (isNext(TokenType::child, i)) {
        if (isNext(TokenType::alias, i + 1)) {
          /* Type alias declaration */
          if (isNext(TokenType::identifier, i + 2)) {
            auto t_ident = tokens.at(i + 3);
            if (ctx.has_type_alias(t_ident.value) ||
                ctx.has_alias(t_ident.value)) {
              throw_error( //
                  std::string(ctx.has_alias(t_ident.value) ? "An" : "A type") +
                      "alias with the name `" + t_ident.value +
                      "` already exists",
                  t_ident.fileRange);
            } else {
              if (isNext(TokenType::assignment, i + 3)) {
                auto end_res = firstPrimaryPosition(TokenType::stop, i + 4);
                if (end_res.hasValue()) {
                  auto type_res = parseType(ctx, i + 4, end_res.getValue());
                  ctx.add_type_alias(t_ident.value, type_res.first);
                  i = end_res.getValue();
                } else {
                  throw_error("Invalid end for the sentence",
                              utils::FileRange(token.fileRange,
                                               tokens.at(i + 4).fileRange));
                }
              } else {
                throw_error("Expected assignment after type alias name",
                            utils::FileRange(token.fileRange,
                                             tokens.at(i + 3).fileRange));
              }
            }
          } else {
            throw_error(
                "Expected name for the type alias",
                utils::FileRange(token.fileRange, tokens.at(i + 2).fileRange));
          }
        }
      } else {
        throw_error("Expected name for the type", token.fileRange);
      }
      break;
    }
    case TokenType::lib: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::curlybraceOpen, i + 1)) {
          auto bClose = getPairEnd(TokenType::curlybraceOpen,
                                   TokenType::curlybraceClose, i + 2, false);
          if (bClose.hasValue()) {
            auto contents = parse(ctx, i + 2, bClose.getValue());
            result.push_back(new ast::Lib(
                tokens.at(i + 1).value, contents, utils::VisibilityInfo::pub(),
                utils::FileRange(token.fileRange,
                                 tokens.at(bClose.getValue()).fileRange)));
            i = bClose.getValue();
          } else {
            throw_error("Expected } to close the lib",
                        tokens.at(i + 2).fileRange);
          }
        } else {
          throw_error("Expected { after name for the lib",
                      tokens.at(i + 1).fileRange);
        }
      } else {
        throw_error("Expected name for the lib", token.fileRange);
      }
      break;
    }
    case TokenType::box: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::curlybraceOpen, i + 1)) {
          auto bClose = getPairEnd(TokenType::curlybraceOpen,
                                   TokenType::curlybraceClose, i + 2, false);
          if (bClose.hasValue()) {
            auto contents = parse(ctx, i + 2, bClose.getValue());
            result.push_back(new ast::Box(
                tokens.at(i + 1).value, contents, utils::VisibilityInfo::pub(),
                utils::FileRange(token.fileRange,
                                 tokens.at(bClose.getValue()).fileRange)));
            i = bClose.getValue();
          } else {
            throw_error("Expected } to close the box",
                        tokens.at(i + 2).fileRange);
          }
        } else {
          throw_error("Expected { after name for the box",
                      tokens.at(i + 1).fileRange);
        }
      } else {
        throw_error("Expected name for the box", token.fileRange);
      }
      break;
    }
    case TokenType::alias: {
      /* Alias declaration */
      if (isNext(TokenType::identifier, i)) {
        auto t_ident = tokens.at(i + 1);
        if (ctx.has_type_alias(t_ident.value) || ctx.has_alias(t_ident.value)) {
          throw_error(
              std::string(ctx.has_alias(t_ident.value) ? "An" : "A type") +
                  "alias with the name `" + t_ident.value + "` already exists",
              t_ident.fileRange);
        } else {
          if (isNext(TokenType::assignment, i + 1)) {
            auto end_res = firstPrimaryPosition(TokenType::stop, i + 2);
            if (end_res.hasValue()) {
              if (isNext(TokenType::identifier, i + 2)) {
                auto sym_res = parseSymbol(ctx, i + 3);
                ctx.add_alias(t_ident.value, sym_res.first.name);
                i = end_res.getValue();
              } else {
                throw_error("Expected an identifier representing an entity for "
                            "the alias",
                            tokens.at(i + 2).fileRange);
              }
            } else {
              throw_error("Invalid end for the sentence",
                          utils::FileRange(token.fileRange,
                                           tokens.at(i + 2).fileRange));
            }
          } else {
            throw_error(
                "Expected assignment after alias name",
                utils::FileRange(token.fileRange, tokens.at(i + 1).fileRange));
          }
        }
      } else {
        throw_error("Expected name for the type alias", token.fileRange);
      }
      break;
    }
    case TokenType::var: {
      setVar();
      break;
    }
    case TokenType::identifier: {
      auto sym_res = parseSymbol(ctx, i);
      i = sym_res.second;
      setCachedSymbol(sym_res.first);
      break;
    }
    case TokenType::givenTypeSeparator: {
      // TODO - Add support for template types for functions
      if (!hasCachedSymbol()) {
        throw_error("Function name not provided", token.fileRange);
      }
      SHOW("Parsing Type")
      auto retTypeRes = parseType(ctx, i, std::nullopt);
      SHOW("Type parsing complete")
      auto retType = retTypeRes.first;
      i = retTypeRes.second;
      if (isNext(TokenType::parenthesisOpen, i)) {
        SHOW("Found (")
        auto pCloseResult =
            getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose,
                       i + 1, false);
        if (!pCloseResult.hasValue()) {
          throw_error("Expected )", tokens.at(i + 1).fileRange);
        }
        SHOW("Found )")
        auto pClose = pCloseResult.getValue();
        SHOW("Parsing function parameters")
        auto argResult =
            parseFunctionParameters(ctx, i + 1, (std::size_t)pClose);
        SHOW("Fn Params complete")
        bool isExternal = false;
        std::string callConv;
        if (isNext(TokenType::stop, pClose)) {
          throw_error("Expected external keyword or a definition",
                      tokens.at(pClose).fileRange);
        } else if (isNext(TokenType::external, pClose)) {
          isExternal = true;
          if (isNext(TokenType::StringLiteral, pClose + 1)) {
            callConv = tokens.at(pClose + 2).value;
            if (!isNext(TokenType::stop, pClose + 2)) {
              // TODO - Sync errors
              throw_error("Expected the function declaration to end here. "
                          "Please add `.` here",
                          tokens.at(pClose + 2).fileRange);
            }
            i = pClose + 3;
          } else {
            throw_error("Expected Calling Convention string", token.fileRange);
          }
        }
        SHOW("Argument count: " << argResult.first.size())
        for (auto arg : argResult.first) {
          SHOW("Arg name " << arg->getName())
        }
        /// TODO: Change implementation of Box
        auto prototype = new ast::FunctionPrototype(
            getCachedSymbol().name, argResult.first, argResult.second,
            // FIXME - Support async
            retType, false,
            isExternal ? llvm::GlobalValue::ExternalLinkage
                       : llvm::GlobalValue::WeakAnyLinkage,
            // FIXME - Support other visibilities
            callConv, utils::VisibilityInfo::pub(), token.fileRange);
        SHOW("Prototype created")
        if (!isExternal) {
          if (isNext(TokenType::bracketOpen, pClose)) {
            auto bCloseResult =
                getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                           pClose + 1, false);
            if (bCloseResult.hasValue()
                    ? (bCloseResult.getValue() >= tokens.size())
                    : true) {
              throw_error("Expected ] at the end of the Function Definition",
                          tokens.at(pClose + 1).fileRange);
            }
            SHOW("HAS BCLOSE")
            auto bClose = bCloseResult.getValue();
            SHOW("Starting sentence parsing")
            auto sentences = parseSentences(ctx, pClose + 1, bClose);
            SHOW("Sentence parsing completed")
            auto definition = new ast::FunctionDefinition(
                prototype, sentences,
                utils::FileRange(tokens.at(pClose + 1).fileRange,
                                 tokens.at(bClose).fileRange));
            SHOW("Function definition created")
            result.push_back(definition);
            i = bClose;
            continue;
          } else {
            throw_error("Expected definition for non-external function",
                        token.fileRange);
          }
        } else {
          // TODO - Implement this for external functions
        }
      } else {
        throw_error("Expected (", token.fileRange);
      }
      break;
    }
    }
  }
  return result;
}

// FIXME - Finish functionality for parsing type contents
std::vector<ast::Node *> Parser::parseCoreTypeContents(ParserContext &prev_ctx,
                                                       const std::size_t from,
                                                       const std::size_t to,
                                                       const std::string name) {
  using lexer::Token;
  using lexer::TokenType;

  std::vector<ast::Node *> result;

  for (std::size_t i = from + 1; i < to; i++) {
    Token &token = tokens.at(i);
    switch (token.type) {
    case TokenType::var: {
      break;
    }
    case TokenType::identifier: {
      break;
    }
    case TokenType::from: {
      break;
    }
    case TokenType::to: {
      break;
    }
    }
  }
}

ast::Expression *
Parser::parseExpression(ParserContext &prev_ctx,
                        const llvm::Optional<CacheSymbol> symbol,
                        const std::size_t from, const std::size_t to) {
  using ast::Expression;
  using lexer::Token;
  using lexer::TokenType;

  bool varCall = false;
  auto setVar = [&varCall]() { varCall = true; };
  auto getVar = [&varCall]() {
    auto result = varCall;
    varCall = false;
    return result;
  };

  llvm::Optional<CacheSymbol> c_sym = symbol;

  std::vector<Expression *> c_exp;
  std::vector<Token> c_binops;
  std::vector<Token> c_unops;
  int pointerCount = 0;
  for (std::size_t i = from + 1; i < to; i++) {
    Token &token = tokens.at(i);
    switch (token.type) {
    case TokenType::unsignedLiteral: {
      if ((token.value.find('_') != std::string::npos) &&
          (token.value.find('u') != (token.value.length() - 1))) {
        unsigned long bits = 32;
        auto split = token.value.find('_');
        std::string number(token.value.substr(0, split));
        if ((split + 2) < token.value.length()) {
          bits = std::stoul(token.value.substr(split + 2));
        }
        c_exp.push_back(
            new ast::CustomIntegerLiteral(number, true, bits, token.fileRange));
      } else {
        c_exp.push_back(new ast::UnsignedLiteral(token.value, token.fileRange));
      }
      break;
    }
    case TokenType::integerLiteral: {
      if (token.value.find('_') != std::string::npos) {
        u64 bits = 32;
        auto split = token.value.find('_');
        std::string number(token.value.substr(0, split));
        if ((split + 2) < token.value.length()) {
          bits = std::stoul(token.value.substr(split + 2));
        }
        c_exp.push_back(new ast::CustomIntegerLiteral(number, false, bits,
                                                      token.fileRange));
      } else {
        c_exp.push_back(new ast::IntegerLiteral(token.value, token.fileRange));
      }
      break;
    }
    case TokenType::floatLiteral: {
      auto number = token.value;
      if (number.find('_') != std::string::npos) {
        c_exp.push_back(new ast::CustomFloatLiteral(
            number.substr(0, number.find('_')),
            number.substr(number.find('_') + 1), token.fileRange));
      } else {
        c_exp.push_back(new ast::FloatLiteral(token.value, token.fileRange));
      }
      break;
    }
    case TokenType::StringLiteral: {
      c_exp.push_back(new ast::StringLiteral(token.value, token.fileRange));
      break;
    }
    case TokenType::null: {
      c_exp.push_back(new ast::NullPointer(token.fileRange));
      break;
    }
    case TokenType::variationMarker: {
      setVar();
      break;
    }
    case TokenType::identifier: {
      bool isStatic = false;
      auto symbol_res = parseSymbol(prev_ctx, i);
      // TODO - Check if this is indeed the only possible scenario
      c_exp.push_back(new ast::Entity(
          symbol_res.first.name,
          utils::FileRange(token.fileRange,
                           tokens.at(symbol_res.second).fileRange)));
      i = symbol_res.second;
      break;
    }
    case TokenType::lesserThan: {
      if (!c_exp.empty()) {
        c_binops.push_back(
            Token::valued(TokenType::binaryOperator, "<", token.fileRange));
      } else {
        throw_error("Expected expression before binary operator <",
                    token.fileRange);
      }
      break;
    }
    case TokenType::parenthesisOpen: {
      if (c_binops.empty() && !c_exp.empty()) {
        // This paranthesis is supposed to indicate a function call
        auto p_close = getPairEnd(TokenType::parenthesisOpen,
                                  TokenType::parenthesisClose, i, false);
        if (p_close.hasValue()) {
          if (p_close.getValue() >= to) {
            throw_error("Invalid position of )", token.fileRange);
          } else {
            std::vector<ast::Expression *> args;
            if (isPrimaryWithin(TokenType::separator, i, p_close.getValue())) {
              args = parseSeparatedExpressions(prev_ctx, i, p_close.getValue());
            } else {
              args.push_back(
                  parseExpression(prev_ctx, llvm::None, i, p_close.getValue()));
            }
            if (c_exp.empty()) {
              if (c_sym.hasValue()) {
                c_exp.push_back(new ast::FunctionCall(
                    c_sym.getValue().name, args,
                    c_sym.getValue().extend_fileRange(
                        tokens.at(p_close.getValue()).fileRange)));
                c_sym = llvm::None;
              } else {
                auto varVal = getVar();
                throw_error(
                    std::string("No expression found to be passed to the ") +
                        (varVal ? "variation " : "") + "function call." +
                        (varVal ? ""
                                : " And no function name found for "
                                  "the static function call"),
                    utils::FileRange(token.fileRange,
                                     tokens.at(p_close.getValue()).fileRange));
              }
            } else if (c_sym.hasValue()) {
              auto instance = *c_exp.end();
              c_exp.pop_back();
              c_exp.push_back(new ast::MemberFunctionCall(
                  instance, c_sym.getValue().name, args, getVar(),
                  c_sym.getValue().fileRange));
              c_sym = llvm::None;
            } else {
              throw_error(
                  std::string("No function name found for the ") +
                      "variation " + "function call",
                  utils::FileRange(token.fileRange,
                                   tokens.at(p_close.getValue()).fileRange));
            }
            i = p_close.getValue();
          }
        } else {
          throw_error("Expected ) to close the scope started by this opening "
                      "paranthesis",
                      token.fileRange);
        }
      } else {
        auto p_close_res = getPairEnd(TokenType::parenthesisOpen,
                                      TokenType::parenthesisClose, i, false);
        if (!p_close_res.hasValue()) {
          throw_error("Expected )", token.fileRange);
        }
        if (p_close_res.getValue() >= to) {
          throw_error("Invalid position of )", token.fileRange);
        }
        auto p_close = p_close_res.getValue();
        if (isPrimaryWithin(TokenType::semiColon, i, p_close)) {
          if (!c_exp.empty()) {
            throw_error("Tuple expression found after another expression",
                        utils::FileRange(token.fileRange,
                                         tokens.at(p_close).fileRange));
          }
          std::vector<ast::Expression *> values;
          auto separations =
              primaryPositionsWithin(TokenType::semiColon, i, p_close);
          values.push_back(
              parseExpression(prev_ctx, c_sym, i, separations.front()));
          bool shouldContinue = true;
          if (separations.size() == 1) {
            SHOW("Only 1 separation")
            shouldContinue = (separations.at(0) != (p_close - 1));
            SHOW("Found condition")
          }
          if (shouldContinue) {
            for (std::size_t j = 0; j < (separations.size() - 1); j += 1) {
              values.push_back(parseExpression(
                  prev_ctx, c_sym, separations.at(j), separations.at(j + 1)));
            }
            values.push_back(
                parseExpression(prev_ctx, c_sym, separations.back(), p_close));
          }
          c_exp.push_back(new ast::TupleValue(
              values,
              utils::FileRange(token.fileRange, tokens.at(p_close).fileRange)));
        } else {
          auto exp = parseExpression(prev_ctx, llvm::None, i, p_close);
          if (c_unops.empty() && c_binops.empty()) {
          } else if (!c_unops.empty()) {
            if (pointerCount != 0) {
              // TODO
            }
          } else if (!c_binops.empty()) {
            if (c_exp.empty()) {
              throw_error("No expression found on the left side of the binary "
                          "operator " +
                              c_binops.end()->value,
                          token.fileRange);
            }
            auto lhs = *c_exp.end();
            auto opr = *c_binops.end();
            auto binExp =
                new ast::BinaryExpression(lhs, opr.value, exp, token.fileRange);
            c_exp.pop_back();
            c_binops.pop_back();
            c_exp.push_back(binExp);
          }
        }
        i = p_close;
      }
      break;
    }
    case TokenType::pointerType: {
      c_unops.push_back(token);
      break;
    }
    case TokenType::binaryOperator: {
      if (c_exp.empty()) {
        throw_error(
            "No expression found on the left side of the binary operator " +
                token.value,
            token.fileRange);
      } else {
        c_binops.push_back(token);
      }
      break;
    }
    case TokenType::child: {
      // FIXME - Handle this
      break;
    }
    case TokenType::stop: {
      throw_error("Found . in an expression - please remove this",
                  token.fileRange);
    }
    default: {
      // TODO - Throw error?
    }
    }
  }
  if (c_exp.empty()) {
    throw_error("No expression found", tokens.at(from).fileRange);
  }
  return c_exp.back();
}

std::vector<ast::Expression *> Parser::parseSeparatedExpressions(
    ParserContext &prev_ctx, const std::size_t from, const std::size_t to) {
  std::vector<ast::Expression *> result;
  for (std::size_t i = from; i < to; i++) {
    if (isPrimaryWithin(lexer::TokenType::separator, i, to)) {
      auto endResult = firstPrimaryPosition(lexer::TokenType::separator, i);
      if (endResult.hasValue() ? (endResult.getValue() >= to) : true) {
        throw_error("Invalid position for separator `,`",
                    tokens.at(i).fileRange);
      }
      auto end = endResult.getValue();
      result.push_back(parseExpression(prev_ctx, llvm::None, i, end));
      i = end;
    } else {
      result.push_back(parseExpression(prev_ctx, llvm::None, i, to));
      i = to;
    }
  }
  return result;
}

std::pair<CacheSymbol, std::size_t>
Parser::parseSymbol(ParserContext &prev_ctx, const std::size_t start) {
  using lexer::TokenType;
  std::string name;
  if (tokens.at(start).type == TokenType::identifier) {
    auto prev = TokenType::identifier;
    auto i = start;
    for (; i < tokens.size(); i++) {
      auto tok = tokens.at(i);
      if (tok.type == TokenType::identifier) {
        if ((i != start) ? prev != TokenType::identifier : true) {
          name += tok.value;
        } else {
          break;
        }
      } else if (tok.type == TokenType::colon) {
        if (prev == TokenType::identifier) {
          name += ":";
        } else {
          throw_error("No identifier found before `:`. Anonymous boxes are "
                      "not allowed",
                      tok.fileRange);
        }
      } else {
        break;
      }
    }
    // If there already is an alias, set the value of the alias as the name of
    // the symbol
    if (prev_ctx.has_alias(name)) {
      name = prev_ctx.get_alias(name);
    }
    return std::pair<CacheSymbol, std::size_t>(
        CacheSymbol(name, start, tokens.at(start).fileRange), i - 1);
  } else {
    // TODO - Change this, after verifying that this situation is never
    // encountered.
    throw_error("No identifier found for parsing the symbol",
                tokens.at(start).fileRange);
  }
}

std::vector<ast::Sentence *> Parser::parseSentences(ParserContext &prev_ctx,
                                                    const std::size_t from,
                                                    const std::size_t to) {
  using ast::FloatType;
  using ast::IntegerType;
  using IR::FloatTypeKind;
  using lexer::Token;
  using lexer::TokenType;

  auto ctx = ParserContext(prev_ctx);
  std::vector<ast::Sentence *> result;

  llvm::Optional<CacheSymbol> c_sym = llvm::None;
  // NOTE - Potentially remove this variable
  std::vector<ast::QatType *> cacheTy;
  std::vector<ast::Expression *> cacheExp;

  std::size_t index = 0;
  std::string context;
  std::string name_val;

  auto var = false;

  for (std::size_t i = from + 1; i < to; i++) {
    index = i;
    Token &token = tokens.at(i);
    switch (token.type) {
    case TokenType::voidType:
    case TokenType::integerType:
    case TokenType::floatType:
    case TokenType::boolType:
    case TokenType::curlybraceOpen:
    case TokenType::var:
    case TokenType::pointerType: {
      if ((token.type == TokenType::var) && isPrev(TokenType::let, i)) {
        // NOTE - This might cause some invalid type parsing
        // This will be continued in the next run where the parser is supposed
        // to encounter an identifier
        break;
      }
      auto typeRes = parseType(ctx, i - 1, std::nullopt);
      i = typeRes.second;
      cacheTy.push_back(typeRes.first);
      break;
    }
    case TokenType::parenthesisOpen: {
      auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                  TokenType::parenthesisClose, i, false);
      if (pCloseRes.hasValue()) {
        auto pClose = pCloseRes.getValue();
        if (isNext(TokenType::singleStatementMarker, pClose)) {
          /* Closure definition */
          if (isNext(TokenType::bracketOpen, pClose + 1)) {
            auto bCloseRes =
                getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                           pClose + 2, false);
            if (bCloseRes.hasValue()) {
              auto endRes =
                  firstPrimaryPosition(TokenType::stop, bCloseRes.getValue());
              if (endRes.hasValue()) {
                auto exp =
                    parseExpression(ctx, c_sym, i - 1, endRes.getValue());
                i = endRes.getValue();
                result.push_back(new ast::ExpressionSentence(
                    exp,
                    utils::FileRange(token.fileRange,
                                     tokens.at(endRes.getValue()).fileRange)));
                break;
              } else {
                throw_error("End for the sentence not found",
                            utils::FileRange(
                                token.fileRange,
                                tokens.at(bCloseRes.getValue()).fileRange));
              }
            } else {
              throw_error("Invalid end for definition of closure",
                          tokens.at(pClose + 2).fileRange);
            }
          } else {
            throw_error("Definition for closure not found",
                        tokens.at(pClose + 1).fileRange);
          }
        } else if (isNext(TokenType::identifier, pClose)) {
          /* Tuple value declaration */
          auto typeRes = parseType(ctx, i - 1, pClose + 1);
          i = typeRes.second;
          cacheTy.push_back(typeRes.first);
          break;
        } else {
          /* Expression */
          auto endRes = firstPrimaryPosition(TokenType::stop, pClose);
          if (endRes.hasValue()) {
            auto exp = parseExpression(ctx, c_sym, i - 1, endRes.getValue());
            i = endRes.getValue();
            result.push_back(new ast::ExpressionSentence(
                exp, utils::FileRange(token.fileRange,
                                      tokens.at(endRes.getValue()).fileRange)));
            break;
          } else {
            throw_error("End of sentence not found", token.fileRange);
          }
        }
      } else {
        throw_error("Invalid end of parenthesis", token.fileRange);
      }
      break;
    }
    case TokenType::child: {
      if (c_sym.hasValue()) {
        auto end = firstPrimaryPosition(TokenType::stop, i);
        if (end.hasValue()) {
          auto exp = parseExpression(ctx, c_sym.getValue(), i, end.getValue());
          i = end.getValue();
        } else {
          throw_error("End of the sentence not found",
                      c_sym.getValue().fileRange);
        }
      } else {
        throw_error("No expression or entity found to access the child of",
                    token.fileRange);
      }

      break;
    }
    case TokenType::identifier: {
      context = "IDENTIFIER";
      SHOW("Identifier encountered")
      if (c_sym.hasValue() || !cacheTy.empty()) {
        /**
         *  Declaration
         *
         * A normal declaration where the type of the entity is provided by the
         * user
         *
         */

        if (isNext(TokenType::assignment, i)) {
          auto end_res = firstPrimaryPosition(TokenType::stop, i);
          if (end_res.hasValue() ? (end_res.getValue() < to) : false) {
            auto exp = parseExpression(ctx, llvm::None, i, end_res.getValue());
            result.push_back(new ast::LocalDeclaration(
                (cacheTy.empty()
                     ? (new ast::NamedType(c_sym.getValue().name, false,
                                           c_sym.getValue().fileRange))
                     : cacheTy.back()),
                token.value, exp, var, token.fileRange));
            var = false;
            cacheTy.clear();
            c_sym = llvm::None;
            i = end_res.getValue();
            break;
          } else {
            throw_error("Invalid end for declaration syntax",
                        tokens.at(i + 1).fileRange);
          }
        } else if (isNext(TokenType::from, i)) {
          // TODO - Handle constructor call
        }
        // FIXME - Handle this case
      } else if (isPrev(TokenType::let, i) ||
                 ((isPrev(TokenType::var, i) &&
                   (i > 0 ? isPrev(TokenType::let, i - 1) : false)))) {

        SHOW("Inferred Declaration")

        /**
         *  Inferred Declaration
         *
         * The type of the declaration is inferred from the expression
         *
         */

        auto isVar = isPrev(TokenType::var, i);
        if (isNext(TokenType::assignment, i)) {
          auto end_res = firstPrimaryPosition(TokenType::stop, i + 1);
          SHOW("Found end of id sentence")
          if (end_res.hasValue() ? (end_res.getValue() >= to) : true) {
            throw_error("Invalid end for the inferred declaration",
                        token.fileRange);
          }
          auto end = end_res.getValue();
          SHOW("Parsing expression")
          auto exp = parseExpression(ctx, llvm::None, i + 1, end);
          SHOW("Expression parsed")
          result.push_back(new ast::LocalDeclaration(nullptr, token.value, exp,
                                                     isVar, token.fileRange));
          var = false;
          cacheTy.clear();
          i = end;
        } else if (isNext(TokenType::from, i)) {
          throw_error(
              std::string("Expected an expression to be assigned to the ") +
                  (isVar ? "variable" : "constant") +
                  " since type inference requires an assignment. You cannot "
                  "use a `from` expression if there is no type specified.",
              tokens.at(i).fileRange);
        } else {
          throw_error(
              std::string("Expected an expression to be assigned to the ") +
                  (isVar ? "variable" : "constant") +
                  " since type inference requires an assignment",
              tokens.at(i).fileRange);
        }
      } else if (isNext(TokenType::child, i)) {
        if (c_sym.hasValue()) {
          auto end_res = firstPrimaryPosition(TokenType::stop, i);
          if (end_res.hasValue() ? (end_res.getValue() <= to) : false) {
            auto end = end_res.getValue();
            auto exp = parseExpression(ctx, c_sym, i, end);
            result.push_back(new ast::ExpressionSentence(
                exp, utils::FileRange(c_sym.getValue().fileRange,
                                      tokens.at(end).fileRange)));
            i = end;
          } else {
            // TODO - Sync errors
            throw_error("Invalid end of sentence", token.fileRange);
          }
        } else {
          // TODO - There is another error call with the same error explained
          // differently. Make sure that these remain the same
          throw_error("Expected an expression before ' for correct access",
                      token.fileRange);
        }
      } else {
        auto sym_res = parseSymbol(ctx, i);
        c_sym = sym_res.first;
        i = sym_res.second;
      }
      break;
    }
    case TokenType::assignment: {
      SHOW("Assignment found")
      /* Assignment */
      if (c_sym.hasValue()) {
        auto end_res = firstPrimaryPosition(TokenType::stop, i);
        if (end_res.hasValue() ? (end_res.getValue() >= to) : true) {
          throw_error("Invalid end for the sentence", token.fileRange);
        }
        auto end = end_res.getValue();
        auto exp = parseExpression(ctx, llvm::None, i, end);
        auto lhs =
            new ast::Entity(c_sym.getValue().name, c_sym.getValue().fileRange);
        result.push_back(
            new ast::Assignment(lhs, exp,
                                utils::FileRange(c_sym.getValue().fileRange,
                                                 tokens.at(end).fileRange)));
        var = false;
        cacheTy.clear();
        i = end;
      } else if (!cacheExp.empty()) {

      } else {
        throw_error("Expected an expression to assign the expression to",
                    token.fileRange);
      }
      break;
    }
    case TokenType::say: {
      context = "SAY";
      auto end_res = firstPrimaryPosition(TokenType::stop, i);
      if (end_res.hasValue() ? (end_res.getValue() >= to) : true) {
        throw_error("Say sentence has invalid end", token.fileRange);
      }
      auto end = end_res.getValue();
      auto exps = parseSeparatedExpressions(ctx, i, end);
      result.push_back(new ast::Say(exps, token.fileRange));
      i = end;
      break;
    }
    case TokenType::stop: {
      context = "STOP";
      break;
    }
    case TokenType::let: {
      context = "LET";
      SHOW("Found let")
      break;
    }
    case TokenType::give: {
      SHOW("give sentence found")
      context = "GIVE";
      if (isNext(TokenType::stop, i)) {
        i++;
        result.push_back(new ast::GiveSentence(
            std::nullopt,
            utils::FileRange(token.fileRange, tokens.at(i + 1).fileRange)));
      } else {
        auto end = firstPrimaryPosition(TokenType::stop, i);
        if (!end.hasValue()) {
          throw_error("Expected give sentence to end. Please add `.` wherever "
                      "appropriate to mark the end of the statement",
                      token.fileRange);
        }
        auto exp = parseExpression(ctx, c_sym, i, end.getValue());
        i = end.getValue();
        result.push_back(new ast::GiveSentence(
            exp, utils::FileRange(token.fileRange,
                                  tokens.at(end.getValue()).fileRange)));
      }
      break;
    }
    default: {
      throw_error("Unexpected token found for parsing sentence",
                  token.fileRange);
    }
    }
  }
  return result;
}

std::pair<std::vector<ast::Argument *>, bool>
Parser::parseFunctionParameters(ParserContext &prev_ctx, const std::size_t from,
                                const std::size_t to) {
  using ast::FloatType;
  using ast::IntegerType;
  using IR::FloatTypeKind;
  using lexer::TokenType;

  std::vector<ast::Argument *> args;

  std::optional<ast::QatType *> typ;
  auto hasType = [&typ]() { return typ.has_value(); };
  auto useType = [&typ]() {
    if (typ.has_value()) {
      auto result = typ.value();
      typ = std::nullopt;
      return result;
    }
  };

  std::optional<std::string> name;
  auto hasName = [&name]() { return name.has_value(); };
  auto useName = [&name]() {
    if (name.has_value()) {
      auto result = name.value();
      name = std::nullopt;
      return result;
    }
  };

  for (std::size_t i = from + 1; ((i < to) && (i < tokens.size())); i++) {
    auto &token = tokens.at(i);
    switch (token.type) {
    case TokenType::voidType: {
      throw_error("Arguments cannot have void type", token.fileRange);
      break;
    }
    case TokenType::identifier: {
      if (hasType() && (!hasName())) {
        name = token.value;
        if (isNext(TokenType::separator, i) ||
            isNext(TokenType::parenthesisClose, i)) {
          args.push_back(
              ast::Argument::Normal(useName(), token.fileRange, useType()));
          i++;
          break;
        } else {
          throw_error("Expected , or ) after argument name", token.fileRange);
        }
      } else if (hasName()) {
        throw_error("Additional name provided after the previous one. Please "
                    "remove this name",
                    token.fileRange);
      } else {
        auto typeRes = parseType(prev_ctx, i - 1, std::nullopt);
        typ = typeRes.first;
        i = typeRes.second;
      }
      break;
    }
    case TokenType::var:
    case TokenType::pointerType:
    case TokenType::referenceType:
    case TokenType::stringType:
    case TokenType::boolType:
    case TokenType::floatType:
    case TokenType::integerType: {
      if (hasType()) {
        throw_error("A type is already provided before. Please change this "
                    "to the name of the argument, or remove the previous type",
                    token.fileRange);
      } else {
        auto typeRes = parseType(prev_ctx, i - 1, std::nullopt);
        typ = typeRes.first;
        i = typeRes.second;
        break;
      }
    }
    case TokenType::variadic: {
      if (isNext(TokenType::identifier, i)) {
        args.push_back(ast::Argument::Normal(
            tokens.at(i + 1).value,
            utils::FileRange(token.fileRange, tokens.at(i + 1).fileRange),
            nullptr));
        if (isNext(TokenType::parenthesisClose, i + 1) ||
            (isNext(TokenType::separator, i + 1) &&
             isNext(TokenType::parenthesisClose, i + 2))) {
          return std::pair(args, true);
        } else {
          throw_error(
              "Variadic argument should be the last argument of the function",
              token.fileRange);
        }
      } else {
        throw_error(
            "Expected name for the variadic argument. Please provide a name",
            token.fileRange);
      }
    }
    case TokenType::self: {
      if (isNext(TokenType::identifier, i)) {
        args.push_back(ast::Argument::ForConstructor(
            tokens.at(i + 1).value,
            utils::FileRange(token.fileRange, tokens.at(i + 1).fileRange),
            nullptr, true));
        i++;
      } else {
        throw_error("Expected name of the member to be initialised",
                    token.fileRange);
      }
    }
    }
  }
  // FIXME - Support variadic function parameters
  return std::pair(args, false);
}

llvm::Optional<std::size_t> Parser::getPairEnd(const lexer::TokenType startType,
                                               const lexer::TokenType endType,
                                               const std::size_t current,
                                               const bool respectScope) {
  std::size_t collisions = 0;
  for (std::size_t i = current + 1; i < tokens.size(); i++) {
    if (respectScope) {
      for (auto scopeTk : TokenFamily::scopeLimiters) {
        if (tokens.at(i).type == scopeTk) {
          return llvm::None;
        }
      }
    }
    if (tokens.at(i).type == startType) {
      collisions++;
    } else if (tokens.at(i).type == endType) {
      if (collisions == 0) {
        return i;
      } else {
        collisions--;
      }
    }
  }
  return llvm::None;
}

bool Parser::isPrev(const lexer::TokenType type, const std::size_t from) {
  if ((from - 1) >= 0) {
    return tokens.at(from - 1).type == type;
  } else {
    return false;
  }
}

bool Parser::isNext(const lexer::TokenType type, const std::size_t current) {
  if ((current + 1) < tokens.size()) {
    return tokens.at(current + 1).type == type;
  } else {
    return false;
  }
}

bool Parser::areOnlyPresentWithin(const std::vector<lexer::TokenType> kinds,
                                  const std::size_t from,
                                  const std::size_t to) {
  for (std::size_t i = from + 1; i < to; i++) {
    for (auto kind : kinds) {
      if (kind != tokens.at(i).type) {
        return false;
      }
    }
  }
  return true;
}

bool Parser::isPrimaryWithin(const lexer::TokenType candidate,
                             const std::size_t from, const std::size_t to) {
  auto pos_res = firstPrimaryPosition(candidate, from);
  return pos_res.hasValue() ? (pos_res.getValue() < to) : false;
}

llvm::Optional<std::size_t>
Parser::firstPrimaryPosition(const lexer::TokenType candidate,
                             const std::size_t from) {
  using lexer::TokenType;
  for (std::size_t i = from + 1; i < tokens.size(); i++) {
    auto tok = tokens.at(i);
    if (tok.type == TokenType::parenthesisOpen) {
      auto end_res = getPairEnd(TokenType::parenthesisOpen,
                                TokenType::parenthesisClose, i, false);
      if (end_res.hasValue() ? (end_res.getValue() < tokens.size()) : false) {
        i = end_res.getValue();
      } else {
        return llvm::None;
      }
    } else if (tok.type == TokenType::bracketOpen) {
      auto end_res =
          getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
      if (end_res.hasValue() ? (end_res.getValue() < tokens.size()) : false) {
        i = end_res.getValue();
      } else {
        return llvm::None;
      }
    } else if (tok.type == TokenType::curlybraceOpen) {
      auto end_res = getPairEnd(TokenType::curlybraceOpen,
                                TokenType::curlybraceClose, i, false);
      if (end_res.hasValue() ? (end_res.getValue() < tokens.size()) : false) {
        i = end_res.getValue();
      } else {
        return llvm::None;
      }
    } else if (tok.type == TokenType::templateTypeStart) {
      auto end_res = getPairEnd(TokenType::templateTypeStart,
                                TokenType::templateTypeEnd, i, false);
      if (end_res.hasValue() ? (end_res.getValue() < tokens.size()) : false) {
        i = end_res.getValue();
      } else {
        return llvm::None;
      }
    } else if (tok.type == candidate) {
      return i;
    }
  }
  return llvm::None;
}

std::vector<std::size_t>
Parser::primaryPositionsWithin(const lexer::TokenType candidate,
                               const std::size_t from, const std::size_t to) {
  std::vector<std::size_t> result;
  using lexer::TokenType;
  for (std::size_t i = from + 1; (i < to && i < tokens.size()); i++) {
    auto tok = tokens.at(i);
    if (tok.type == TokenType::parenthesisOpen) {
      auto end_res = getPairEnd(TokenType::parenthesisOpen,
                                TokenType::parenthesisClose, i, false);
      if (end_res.hasValue() ? (end_res.getValue() < to) : false) {
        i = end_res.getValue();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::bracketOpen) {
      auto end_res =
          getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
      if (end_res.hasValue() ? (end_res.getValue() < to) : false) {
        i = end_res.getValue();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::curlybraceOpen) {
      auto end_res = getPairEnd(TokenType::curlybraceOpen,
                                TokenType::curlybraceClose, i, false);
      if (end_res.hasValue() ? (end_res.getValue() < to) : false) {
        i = end_res.getValue();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::templateTypeStart) {
      auto end_res = getPairEnd(TokenType::templateTypeStart,
                                TokenType::templateTypeEnd, i, false);
      if (end_res.hasValue() ? (end_res.getValue() < to) : false) {
        i = end_res.getValue();
      } else {
        return result;
      }
    } else if (tok.type == candidate) {
      result.push_back(i);
    }
  }
  return result;
}

void Parser::throw_error(const std::string message,
                         const utils::FileRange fileRange) {
  std::cout << colors::red << "[ PARSER ERROR ] " << colors::bold::green
            << fs::absolute(fileRange.file).string() << ":"
            << fileRange.start.line << ":" << fileRange.start.character << " - "
            << fileRange.end.line << ":" << fileRange.end.character
            << colors::reset << "\n"
            << "   " << message << "\n";
  tokens.clear();
  exit(0);
}

} // namespace qat::parser