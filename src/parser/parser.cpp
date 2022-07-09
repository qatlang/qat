#include "./parser.hpp"
#include "../AST/lib.hpp"
#include "../show.hpp"
#include <string>

// NOTE - Check if file placement values are making use of the new merge
// functionality
namespace qat {
namespace parser {

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
            tokens.at(i + 1).filePlacement);
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
              tokens.at(i + 1).filePlacement);
        } else {
          throw_error("Unexpected token in bring sentence",
                      tokens.at(i).filePlacement);
        }
      }
      break;
    }
    default: {
      throw_error("Unexpected token in bring sentence",
                  tokens.at(i).filePlacement);
    }
    }
  }
  return result;
}

std::pair<AST::QatType *, std::size_t>
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
  std::optional<AST::QatType *> cacheTy;

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
            throw_error("Tuple type found after another type",
                        utils::FilePlacement(
                            token.filePlacement,
                            tokens.at(pCloseRes.getValue()).filePlacement));
          } else {
            return std::pair(cacheTy.value(), i - 1);
          }
        } else {
          throw_error("Expected )", token.filePlacement);
        }
      }
      auto pCloseResult = getPairEnd(TokenType::parenthesisOpen,
                                     TokenType::parenthesisClose, i, false);
      if (!pCloseResult.hasValue()) {
        throw_error("Expected )", token.filePlacement);
      }
      if (to.has_value() ? (pCloseResult.getValue() > to.value()) : false) {
        throw_error("Invalid position for )",
                    tokens.at(pCloseResult.getValue()).filePlacement);
      }
      auto pClose = pCloseResult.getValue();
      std::vector<AST::QatType *> subTypes;
      for (std::size_t j = i; j < pClose; j++) {
        if (isPrimaryWithin(TokenType::semiColon, j, pClose)) {
          auto semiPosResult = firstPrimaryPosition(TokenType::semiColon, j);
          if (!semiPosResult.hasValue()) {
            throw_error("Invalid position of ; separator", token.filePlacement);
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
      cacheTy = new AST::TupleType(subTypes, false, getVariability(),
                                   token.filePlacement);
      break;
    }
    case TokenType::voidType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      cacheTy = new AST::VoidType(getVariability(), token.filePlacement);
      break;
    }
    case TokenType::boolType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      cacheTy = new AST::UnsignedType(1, getVariability(), token.filePlacement);
      break;
    }
    case TokenType::unsignedIntegerType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      cacheTy = new AST::UnsignedType(std::stoi(token.value), getVariability(),
                                      token.filePlacement);
      break;
    }
    case TokenType::integerType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      cacheTy = new AST::IntegerType(std::stoi(token.value), getVariability(),
                                     token.filePlacement);
      break;
    }
    case TokenType::floatType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      if (token.value == "brain") {
        cacheTy = new AST::FloatType(IR::FloatTypeKind::_brain,
                                     getVariability(), token.filePlacement);
      } else if (token.value == "half") {
        cacheTy = new AST::FloatType(IR::FloatTypeKind::_half, getVariability(),
                                     token.filePlacement);
      } else if (token.value == "32") {
        cacheTy = new AST::FloatType(IR::FloatTypeKind::_32, getVariability(),
                                     token.filePlacement);
      } else if (token.value == "64") {
        cacheTy = new AST::FloatType(IR::FloatTypeKind::_64, getVariability(),
                                     token.filePlacement);
      } else if (token.value == "80") {
        cacheTy = new AST::FloatType(IR::FloatTypeKind::_80, getVariability(),
                                     token.filePlacement);
      } else if (token.value == "128") {
        cacheTy = new AST::FloatType(IR::FloatTypeKind::_128, getVariability(),
                                     token.filePlacement);
      } else if (token.value == "128ppc") {
        cacheTy = new AST::FloatType(IR::FloatTypeKind::_128PPC,
                                     getVariability(), token.filePlacement);
      } else {
        cacheTy = new AST::FloatType(IR::FloatTypeKind::_32, getVariability(),
                                     token.filePlacement);
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
      cacheTy = new AST::NamedType(symRes.first.name, getVariability(),
                                   token.filePlacement);
      break;
    }
    case TokenType::referenceType: {
      if (cacheTy.has_value()) {
        return std::pair(cacheTy.value(), i - 1);
      }
      auto subRes = parseType(ctx, i, to);
      i = subRes.second;
      cacheTy = new AST::ReferenceType(
          subRes.first, getVariability(),
          utils::FilePlacement(token.filePlacement,
                               tokens.at(i).filePlacement));
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
          cacheTy = new AST::PointerType(
              subTypeRes.first, getVariability(),
              utils::FilePlacement(token.filePlacement,
                                   tokens.at(bClose).filePlacement));
          break;
        } else {
          throw_error("Invalid end for pointer type",
                      tokens.at(i).filePlacement);
        }
      } else {
        throw_error("Type of the pointer not specified", token.filePlacement);
      }
      break;
    }
    case TokenType::bracketOpen: {
      if (!cacheTy.has_value()) {
        throw_error("Element type of array not specified", token.filePlacement);
      }
      if (isNext(TokenType::unsignedLiteral, i) ||
          isNext(TokenType::integerLiteral, i)) {
        if (isNext(TokenType::bracketClose, i + 1)) {
          auto subType = cacheTy.value();
          auto numstr = tokens.at(i + 1).value;
          if (numstr.find('_') != std::string::npos) {
            numstr = numstr.substr(0, numstr.find('_'));
          }
          cacheTy = new AST::ArrayType(subType, std::stoul(numstr),
                                       getVariability(), token.filePlacement);
          i = i + 2;
          break;
        } else {
          throw_error("Expected ] after the array size",
                      tokens.at(i + 1).filePlacement);
        }
      } else {
        throw_error("Expected non negative number of elements for the array",
                    token.filePlacement);
      }
      break;
    }
    default: {
      if (!cacheTy.has_value()) {
        throw_error("No type found", tokens.at(from).filePlacement);
      }
      return std::pair(cacheTy.value(), i - 1);
    }
    }
  }
  if (!cacheTy.has_value()) {
    throw_error("No type found", tokens.at(from).filePlacement);
  }
  return std::pair(cacheTy.value(), i - 1);
}

std::vector<AST::Node *> Parser::parse(ParserContext prev_ctx, std::size_t from,
                                       std::size_t to) {
  std::vector<AST::Node *> result;
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
  std::deque<AST::QatType *> cacheTy;
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
    };
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
                        tokens.at(i + 2).filePlacement);
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
                  t_ident.filePlacement);
            } else {
              if (isNext(TokenType::assignment, i + 3)) {
                auto end_res = firstPrimaryPosition(TokenType::stop, i + 4);
                if (end_res.hasValue()) {
                  auto type_res = parseType(ctx, i + 4, end_res.getValue());
                  ctx.add_type_alias(t_ident.value, type_res.first);
                  i = end_res.getValue();
                } else {
                  throw_error(
                      "Invalid end for the sentence",
                      utils::FilePlacement(token.filePlacement,
                                           tokens.at(i + 4).filePlacement));
                }
              } else {
                throw_error(
                    "Expected assignment after type alias name",
                    utils::FilePlacement(token.filePlacement,
                                         tokens.at(i + 3).filePlacement));
              }
            }
          } else {
            throw_error("Expected name for the type alias",
                        utils::FilePlacement(token.filePlacement,
                                             tokens.at(i + 2).filePlacement));
          }
        }
      } else {
        throw_error("Expected name for the type", token.filePlacement);
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
            result.push_back(new AST::Lib(
                tokens.at(i + 1).value, contents, utils::VisibilityInfo::pub(),
                utils::FilePlacement(
                    token.filePlacement,
                    tokens.at(bClose.getValue()).filePlacement)));
            i = bClose.getValue();
          } else {
            throw_error("Expected } to close the lib",
                        tokens.at(i + 2).filePlacement);
          }
        } else {
          throw_error("Expected { after name for the lib",
                      tokens.at(i + 1).filePlacement);
        }
      } else {
        throw_error("Expected name for the lib", token.filePlacement);
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
            result.push_back(new AST::Box(
                tokens.at(i + 1).value, contents, utils::VisibilityInfo::pub(),
                utils::FilePlacement(
                    token.filePlacement,
                    tokens.at(bClose.getValue()).filePlacement)));
            i = bClose.getValue();
          } else {
            throw_error("Expected } to close the box",
                        tokens.at(i + 2).filePlacement);
          }
        } else {
          throw_error("Expected { after name for the box",
                      tokens.at(i + 1).filePlacement);
        }
      } else {
        throw_error("Expected name for the box", token.filePlacement);
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
              t_ident.filePlacement);
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
                            tokens.at(i + 2).filePlacement);
              }
            } else {
              throw_error("Invalid end for the sentence",
                          utils::FilePlacement(token.filePlacement,
                                               tokens.at(i + 2).filePlacement));
            }
          } else {
            throw_error("Expected assignment after alias name",
                        utils::FilePlacement(token.filePlacement,
                                             tokens.at(i + 1).filePlacement));
          }
        }
      } else {
        throw_error("Expected name for the type alias",
                    utils::FilePlacement(token.filePlacement,
                                         tokens.at(i + 2).filePlacement));
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
        throw_error("Function name not provided", token.filePlacement);
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
          throw_error("Expected )", tokens.at(i + 1).filePlacement);
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
                      tokens.at(pClose).filePlacement);
        } else if (isNext(TokenType::external, pClose)) {
          isExternal = true;
          if (isNext(TokenType::StringLiteral, pClose + 1)) {
            callConv = tokens.at(pClose + 2).value;
            if (!isNext(TokenType::stop, pClose + 2)) {
              // TODO - Sync errors
              throw_error("Expected the function declaration to end here. "
                          "Please add `.` here",
                          tokens.at(pClose + 2).filePlacement);
            }
            i = pClose + 3;
          } else {
            throw_error("Expected Calling Convention string",
                        token.filePlacement);
          }
        }
        SHOW("Argument count: " << argResult.first.size())
        for (auto arg : argResult.first) {
          SHOW("Arg name " << arg->getName())
        }
        /// TODO: Change implementation of Box
        auto prototype = new AST::FunctionPrototype(
            getCachedSymbol().name, argResult.first, argResult.second,
            // FIXME - Support async
            retType, false,
            isExternal ? llvm::GlobalValue::ExternalLinkage
                       : llvm::GlobalValue::WeakAnyLinkage,
            // FIXME - Support other visibilities
            callConv, utils::VisibilityInfo::pub(), token.filePlacement);
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
                          tokens.at(pClose + 1).filePlacement);
            }
            SHOW("HAS BCLOSE")
            auto bClose = bCloseResult.getValue();
            SHOW("Starting sentence parsing")
            auto sentences = parseSentences(ctx, pClose + 1, bClose);
            SHOW("Sentence parsing completed")
            auto definition = new AST::FunctionDefinition(
                prototype, sentences,
                utils::FilePlacement(tokens.at(pClose + 1).filePlacement,
                                     tokens.at(bClose).filePlacement));
            SHOW("Function definition created")
            result.push_back(definition);
            i = bClose;
            continue;
          } else {
            throw_error("Expected definition for non-external function",
                        token.filePlacement);
          }
        } else {
          // TODO - Implement this for external functions
        }
      } else {
        throw_error("Expected (", token.filePlacement);
      }
      break;
    }
    }
  }
  return result;
}

// FIXME - Finish functionality for parsing type contents
std::vector<AST::Node *> Parser::parseCoreTypeContents(ParserContext &prev_ctx,
                                                       const std::size_t from,
                                                       const std::size_t to,
                                                       const std::string name) {
  using lexer::Token;
  using lexer::TokenType;

  std::vector<AST::Node *> result;

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

AST::Expression *
Parser::parseExpression(ParserContext &prev_ctx,
                        const llvm::Optional<CacheSymbol> symbol,
                        const std::size_t from, const std::size_t to) {
  using AST::Expression;
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
        c_exp.push_back(new AST::CustomIntegerLiteral(number, true, bits,
                                                      token.filePlacement));
      } else {
        c_exp.push_back(
            new AST::UnsignedLiteral(token.value, token.filePlacement));
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
        c_exp.push_back(new AST::CustomIntegerLiteral(number, false, bits,
                                                      token.filePlacement));
      } else {
        c_exp.push_back(
            new AST::IntegerLiteral(token.value, token.filePlacement));
      }
      break;
    }
    case TokenType::floatLiteral: {
      auto number = token.value;
      if (number.find('_') != std::string::npos) {
        c_exp.push_back(new AST::CustomFloatLiteral(
            number.substr(0, number.find('_')),
            number.substr(number.find('_') + 1), token.filePlacement));
      } else {
        c_exp.push_back(
            new AST::FloatLiteral(token.value, token.filePlacement));
      }
      break;
    }
    case TokenType::StringLiteral: {
      c_exp.push_back(new AST::StringLiteral(token.value, token.filePlacement));
      break;
    }
    case TokenType::null: {
      c_exp.push_back(new AST::NullPointer(token.filePlacement));
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
      c_exp.push_back(new AST::Entity(
          symbol_res.first.name,
          utils::FilePlacement(token.filePlacement,
                               tokens.at(symbol_res.second).filePlacement)));
      i = symbol_res.second;
      break;
    }
    case TokenType::lesserThan: {
      if (!c_exp.empty()) {
        c_binops.push_back(
            Token::valued(TokenType::binaryOperator, "<", token.filePlacement));
      } else {
        throw_error("Expected expression before binary operator <",
                    token.filePlacement);
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
            throw_error("Invalid position of )", token.filePlacement);
          } else {
            std::vector<AST::Expression *> args;
            if (isPrimaryWithin(TokenType::separator, i, p_close.getValue())) {
              args = parseSeparatedExpressions(prev_ctx, i, p_close.getValue());
            } else {
              args.push_back(
                  parseExpression(prev_ctx, llvm::None, i, p_close.getValue()));
            }
            if (c_exp.empty()) {
              if (c_sym.hasValue()) {
                c_exp.push_back(new AST::FunctionCall(
                    c_sym.getValue().name, args,
                    c_sym.getValue().extend_placement(
                        tokens.at(p_close.getValue()).filePlacement)));
                c_sym = llvm::None;
              } else {
                auto varVal = getVar();
                throw_error(
                    std::string("No expression found to be passed to the ") +
                        (varVal ? "variation " : "") + "function call." +
                        (varVal ? ""
                                : " And no function name found for "
                                  "the static function call"),
                    utils::FilePlacement(
                        token.filePlacement,
                        tokens.at(p_close.getValue()).filePlacement));
              }
            } else if (c_sym.hasValue()) {
              auto instance = *c_exp.end();
              c_exp.pop_back();
              c_exp.push_back(new AST::MemberFunctionCall(
                  instance, c_sym.getValue().name, args, getVar(),
                  c_sym.getValue().filePlacement));
              c_sym = llvm::None;
            } else {
              throw_error(std::string("No function name found for the ") +
                              "variation " + "function call",
                          utils::FilePlacement(
                              token.filePlacement,
                              tokens.at(p_close.getValue()).filePlacement));
            }
            i = p_close.getValue();
          }
        } else {
          throw_error("Expected ) to close the scope started by this opening "
                      "paranthesis",
                      token.filePlacement);
        }
      } else {
        auto p_close_res = getPairEnd(TokenType::parenthesisOpen,
                                      TokenType::parenthesisClose, i, false);
        if (!p_close_res.hasValue()) {
          throw_error("Expected )", token.filePlacement);
        }
        if (p_close_res.getValue() >= to) {
          throw_error("Invalid position of )", token.filePlacement);
        }
        auto p_close = p_close_res.getValue();
        if (isPrimaryWithin(TokenType::semiColon, i, p_close)) {
          if (!c_exp.empty()) {
            throw_error("Tuple expression found after another expression",
                        utils::FilePlacement(token.filePlacement,
                                             tokens.at(p_close).filePlacement));
          }
          std::vector<AST::Expression *> values;
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
          c_exp.push_back(new AST::TupleValue(
              values, utils::FilePlacement(token.filePlacement,
                                           tokens.at(p_close).filePlacement)));
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
                          token.filePlacement);
            }
            auto lhs = *c_exp.end();
            auto opr = *c_binops.end();
            auto binExp = new AST::BinaryExpression(lhs, opr.value, exp,
                                                    token.filePlacement);
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
            token.filePlacement);
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
                  token.filePlacement);
    }
    default: {
      // TODO - Throw error?
    }
    }
  }
  if (c_exp.empty()) {
    throw_error("No expression found", tokens.at(from).filePlacement);
  }
  return c_exp.back();
}

std::vector<AST::Expression *> Parser::parseSeparatedExpressions(
    ParserContext &prev_ctx, const std::size_t from, const std::size_t to) {
  std::vector<AST::Expression *> result;
  for (std::size_t i = from; i < to; i++) {
    if (isPrimaryWithin(lexer::TokenType::separator, i, to)) {
      auto endResult = firstPrimaryPosition(lexer::TokenType::separator, i);
      if (endResult.hasValue() ? (endResult.getValue() >= to) : true) {
        throw_error("Invalid position for separator `,`",
                    tokens.at(i).filePlacement);
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
        if (i != start ? prev != TokenType::identifier : true) {
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
                      tok.filePlacement);
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
        CacheSymbol(name, start, tokens.at(start).filePlacement), i - 1);
  } else {
    // TODO - Change this, after verifying that this situation is never
    // encountered.
    throw_error("No identifier found for parsing the symbol",
                tokens.at(start).filePlacement);
  }
}

std::vector<AST::Sentence *> Parser::parseSentences(ParserContext &prev_ctx,
                                                    const std::size_t from,
                                                    const std::size_t to) {
  using AST::FloatType;
  using AST::IntegerType;
  using IR::FloatTypeKind;
  using lexer::Token;
  using lexer::TokenType;

  auto ctx = ParserContext(prev_ctx);
  std::vector<AST::Sentence *> result;

  llvm::Optional<CacheSymbol> c_sym = llvm::None;
  // NOTE - Potentially remove this variable
  std::vector<AST::QatType *> cacheTy;
  std::vector<AST::Expression *> cacheExp;

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
                result.push_back(new AST::ExpressionSentence(
                    exp, utils::FilePlacement(
                             token.filePlacement,
                             tokens.at(endRes.getValue()).filePlacement)));
                break;
              } else {
                throw_error("End for the sentence not found",
                            utils::FilePlacement(
                                token.filePlacement,
                                tokens.at(bCloseRes.getValue()).filePlacement));
              }
            } else {
              throw_error("Invalid end for definition of closure",
                          tokens.at(pClose + 2).filePlacement);
            }
          } else {
            throw_error("Definition for closure not found",
                        tokens.at(pClose + 1).filePlacement);
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
            result.push_back(new AST::ExpressionSentence(
                exp, utils::FilePlacement(
                         token.filePlacement,
                         tokens.at(endRes.getValue()).filePlacement)));
            break;
          } else {
            throw_error("End of sentence not found",
                        utils::FilePlacement(token.filePlacement,
                                             tokens.at(pClose).filePlacement));
          }
        }
      } else {
        throw_error("Invalid end of parenthesis", token.filePlacement);
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
                      c_sym.getValue().filePlacement);
        }
      } else {
        throw_error("No expression or entity found to access the child of",
                    token.filePlacement);
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
            result.push_back(new AST::LocalDeclaration(
                (cacheTy.empty()
                     ? (new AST::NamedType(c_sym.getValue().name, false,
                                           c_sym.getValue().filePlacement))
                     : cacheTy.back()),
                token.value, exp, var, token.filePlacement));
            var = false;
            cacheTy.clear();
            c_sym = llvm::None;
            i = end_res.getValue();
            break;
          } else {
            throw_error("Invalid end for declaration syntax",
                        tokens.at(i + 1).filePlacement);
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
                        token.filePlacement);
          }
          auto end = end_res.getValue();
          SHOW("Parsing expression")
          auto exp = parseExpression(ctx, llvm::None, i + 1, end);
          SHOW("Expression parsed")
          result.push_back(new AST::LocalDeclaration(
              nullptr, token.value, exp, isVar, token.filePlacement));
          var = false;
          cacheTy.clear();
          i = end;
        } else if (isNext(TokenType::from, i)) {
          throw_error(
              std::string("Expected an expression to be assigned to the ") +
                  (isVar ? "variable" : "constant") +
                  " since type inference requires an assignment. You cannot "
                  "use a `from` expression if there is no type specified.",
              tokens.at(i).filePlacement);
        } else {
          throw_error(
              std::string("Expected an expression to be assigned to the ") +
                  (isVar ? "variable" : "constant") +
                  " since type inference requires an assignment",
              tokens.at(i).filePlacement);
        }
      } else if (isNext(TokenType::child, i)) {
        if (c_sym.hasValue()) {
          auto end_res = firstPrimaryPosition(TokenType::stop, i);
          if (end_res.hasValue() ? (end_res.getValue() <= to) : false) {
            auto end = end_res.getValue();
            auto exp = parseExpression(ctx, c_sym, i, end);
            result.push_back(new AST::ExpressionSentence(
                exp, utils::FilePlacement(c_sym.getValue().filePlacement,
                                          tokens.at(end).filePlacement)));
            i = end;
          } else {
            // TODO - Sync errors
            throw_error("Invalid end of sentence", token.filePlacement);
          }
        } else {
          // TODO - There is another error call with the same error explained
          // differently. Make sure that these remain the same
          throw_error("Expected an expression before ' for correct access",
                      token.filePlacement);
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
          throw_error("Invalid end for the sentence", token.filePlacement);
        }
        auto end = end_res.getValue();
        auto exp = parseExpression(ctx, llvm::None, i, end);
        auto lhs = new AST::Entity(c_sym.getValue().name,
                                   c_sym.getValue().filePlacement);
        result.push_back(new AST::Assignment(
            lhs, exp,
            utils::FilePlacement(c_sym.getValue().filePlacement,
                                 tokens.at(end).filePlacement)));
        var = false;
        cacheTy.clear();
        i = end;
      } else if (!cacheExp.empty()) {

      } else {
        throw_error("Expected an expression to assign the expression to",
                    token.filePlacement);
      }
      break;
    }
    case TokenType::say: {
      context = "SAY";
      auto end_res = firstPrimaryPosition(TokenType::stop, i);
      if (end_res.hasValue() ? (end_res.getValue() >= to) : true) {
        throw_error("Say sentence has invalid end", token.filePlacement);
      }
      auto end = end_res.getValue();
      auto exps = parseSeparatedExpressions(ctx, i, end);
      result.push_back(new AST::Say(exps, token.filePlacement));
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
        result.push_back(new AST::GiveSentence(
            std::nullopt,
            utils::FilePlacement(token.filePlacement,
                                 tokens.at(i + 1).filePlacement)));
      } else {
        auto end = firstPrimaryPosition(TokenType::stop, i);
        if (!end.hasValue()) {
          throw_error("Expected give sentence to end. Please add `.` wherever "
                      "appropriate to mark the end of the statement",
                      token.filePlacement);
        }
        auto exp = parseExpression(ctx, c_sym, i, end.getValue());
        i = end.getValue();
        result.push_back(new AST::GiveSentence(
            exp,
            utils::FilePlacement(token.filePlacement,
                                 tokens.at(end.getValue()).filePlacement)));
      }
      break;
    }
    default: {
      throw_error("Unexpected token found for parsing sentence",
                  token.filePlacement);
    }
    }
  }
  return result;
}

std::pair<std::vector<AST::Argument *>, bool>
Parser::parseFunctionParameters(ParserContext &prev_ctx, const std::size_t from,
                                const std::size_t to) {
  using AST::FloatType;
  using AST::IntegerType;
  using IR::FloatTypeKind;
  using lexer::TokenType;

  std::vector<AST::Argument *> args;

  std::optional<AST::QatType *> typ;
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
      throw_error("Arguments cannot have void type", token.filePlacement);
      break;
    }
    case TokenType::identifier: {
      if (hasType() && (!hasName())) {
        name = token.value;
        if (isNext(TokenType::separator, i) ||
            isNext(TokenType::parenthesisClose, i)) {
          args.push_back(
              AST::Argument::Normal(useName(), token.filePlacement, useType()));
          i++;
          break;
        } else {
          throw_error("Expected , or ) after argument name",
                      token.filePlacement);
        }
      } else if (hasName()) {
        throw_error("Additional name provided after the previous one. Please "
                    "remove this name",
                    token.filePlacement);
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
                    token.filePlacement);
      } else {
        auto typeRes = parseType(prev_ctx, i - 1, std::nullopt);
        typ = typeRes.first;
        i = typeRes.second;
        break;
      }
    }
    case TokenType::variadic: {
      if (isNext(TokenType::identifier, i)) {
        args.push_back(AST::Argument::Normal(
            tokens.at(i + 1).value,
            utils::FilePlacement(token.filePlacement,
                                 tokens.at(i + 1).filePlacement),
            nullptr));
        if (isNext(TokenType::parenthesisClose, i + 1) ||
            (isNext(TokenType::separator, i + 1) &&
             isNext(TokenType::parenthesisClose, i + 2))) {
          return std::pair(args, true);
        } else {
          throw_error(
              "Variadic argument should be the last argument of the function",
              token.filePlacement);
        }
      } else {
        throw_error(
            "Expected name for the variadic argument. Please provide a name",
            token.filePlacement);
      }
    }
    case TokenType::self: {
      if (isNext(TokenType::identifier, i)) {
        args.push_back(AST::Argument::ForConstructor(
            tokens.at(i + 1).value,
            utils::FilePlacement(token.filePlacement,
                                 tokens.at(i + 1).filePlacement),
            nullptr, true));
        i++;
      } else {
        throw_error("Expected name of the member to be initialised",
                    token.filePlacement);
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
                         const utils::FilePlacement filePlacement) {
  std::cout << colors::red << "[ PARSER ERROR ] " << colors::bold::green
            << fs::absolute(filePlacement.file).string() << ":"
            << filePlacement.start.line << ":" << filePlacement.start.character
            << " - " << filePlacement.end.line << ":"
            << filePlacement.end.character << colors::reset << "\n"
            << "   " << message << "\n";
  tokens.clear();
  exit(0);
}

} // namespace parser
} // namespace qat