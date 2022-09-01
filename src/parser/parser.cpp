#include "./parser.hpp"
#include "../ast/convertor_definition.hpp"
#include "../ast/convertor_prototype.hpp"
#include "../ast/expressions/array_literal.hpp"
#include "../ast/expressions/binary_expression.hpp"
#include "../ast/expressions/custom_float_literal.hpp"
#include "../ast/expressions/custom_integer_literal.hpp"
#include "../ast/expressions/entity.hpp"
#include "../ast/expressions/float_literal.hpp"
#include "../ast/expressions/function_call.hpp"
#include "../ast/expressions/index_access.hpp"
#include "../ast/expressions/integer_literal.hpp"
#include "../ast/expressions/loop_index.hpp"
#include "../ast/expressions/member_access.hpp"
#include "../ast/expressions/member_function_call.hpp"
#include "../ast/expressions/null_pointer.hpp"
#include "../ast/expressions/plain_initialiser.hpp"
#include "../ast/expressions/self.hpp"
#include "../ast/expressions/self_member.hpp"
#include "../ast/expressions/ternary.hpp"
#include "../ast/expressions/to_conversion.hpp"
#include "../ast/expressions/tuple_value.hpp"
#include "../ast/expressions/unsigned_literal.hpp"
#include "../ast/function_definition.hpp"
#include "../ast/lib.hpp"
#include "../ast/member_definition.hpp"
#include "../ast/sentences/assignment.hpp"
#include "../ast/sentences/break.hpp"
#include "../ast/sentences/continue.hpp"
#include "../ast/sentences/expression_sentence.hpp"
#include "../ast/sentences/give_sentence.hpp"
#include "../ast/sentences/if_else.hpp"
#include "../ast/sentences/local_declaration.hpp"
#include "../ast/sentences/loop_infinite.hpp"
#include "../ast/sentences/loop_n_times.hpp"
#include "../ast/sentences/loop_while.hpp"
#include "../ast/sentences/say_sentence.hpp"
#include "../ast/type_definition.hpp"
#include "../ast/types/array.hpp"
#include "../ast/types/cstring.hpp"
#include "../ast/types/float.hpp"
#include "../ast/types/integer.hpp"
#include "../ast/types/named.hpp"
#include "../ast/types/pointer.hpp"
#include "../ast/types/reference.hpp"
#include "../ast/types/string_slice.hpp"
#include "../ast/types/tuple.hpp"
#include "../ast/types/unsigned.hpp"
#include "../show.hpp"
#include "cache_symbol.hpp"

#define RangeAt(ind) tokens.at(ind).fileRange
#define RangeSpan(ind1, ind2)                                                  \
  { tokens.at(ind1).fileRange, tokens.at(ind2).fileRange }

// NOTE - Check if file fileRange values are making use of the new merge
// functionality
namespace qat::parser {

Parser::Parser() = default;

void Parser::setTokens(const Vec<lexer::Token> &allTokens) {
  g_ctx  = ParserContext();
  tokens = allTokens;
}

void Parser::filterComments() {}

ast::BringEntities *Parser::parseBroughtEntities(ParserContext &ctx, usize from,
                                                 usize upto) {
  using lexer::TokenType;
  Vec<String>        result;
  Maybe<CacheSymbol> csym = None;

  for (usize i = from + 1; i < upto; i++) {
    auto token = tokens.at(i);
    switch (token.type) { // NOLINT(clang-diagnostic-switch)
    case TokenType::identifier: {
      auto sym_res = parseSymbol(ctx, i);
      csym         = sym_res.first;
      i            = sym_res.second;
      if (isNext(TokenType::curlybraceOpen, i)) {
        auto bCloseRes = getPairEnd(TokenType::curlybraceOpen,
                                    TokenType::curlybraceClose, i, false);
        if (bCloseRes.has_value()) {
          auto        bClose = bCloseRes.value();
          Vec<String> entities;
          if (isPrimaryWithin(TokenType::separator, i, bClose)) {
            // TODO - Implement
          } else {
            Warning("Expected multiple entities to be brought. Remove the "
                    "curly braces since only one entity is being brought.",
                    utils::FileRange(RangeAt(i + 1), RangeAt(bClose)));
            if (isNext(TokenType::identifier, i + 1)) {
              entities.push_back(tokens.at(i + 2).value);
            } else {
              Error("Expected the name of an entity in the bring sentence",
                    RangeAt(i + 2));
            }
          }

        } else {
          Error("Expected } to close this curly brace", RangeAt(i + 1));
        }
      } else if (isNext(TokenType::identifier, i)) {
      }
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
  // FIXME - Return valid value
}

Vec<String> Parser::parseBroughtFilesOrFolders(usize from, usize upto) {
  using lexer::TokenType;
  Vec<String> result;
  for (usize i = from + 1; (i < upto) && (i < tokens.size()); i++) {
    auto token = tokens.at(i);
    switch (token.type) {
    case TokenType::StringLiteral: {
      if (isNext(TokenType::StringLiteral, i)) {
        Error("Implicit concatenation of adjacent string literals are not "
              "supported in bring sentences, to avoid ambiguity and confusion. "
              "If this is supposed to be another file or folder to bring, then "
              "add a separator between these. Or else fix the sentence.",
              RangeAt(i + 1));
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
          Error("Multiple adjacent separators found. This is not supported to "
                "discourage code clutter. Please remove this",
                RangeAt(i + 1));
        } else {
          Error("Unexpected token in bring sentence", RangeAt(i));
        }
      }
      break;
    }
    default: {
      Error("Unexpected token in bring sentence", RangeAt(i));
    }
    }
  }
  return result;
}

Pair<ast::QatType *, usize> Parser::parseType(ParserContext &prev_ctx,
                                              usize from, Maybe<usize> upto) {
  using lexer::Token;
  using lexer::TokenType;

  bool variable       = false;
  auto getVariability = [&variable]() {
    auto value = variable;
    variable   = false;
    return value;
  };

  auto                  ctx = ParserContext(prev_ctx);
  usize                 i   = 0; // NOLINT(readability-identifier-length)
  Maybe<ast::QatType *> cacheTy;

  for (i = from + 1; i < (upto.has_value() ? upto.value() : tokens.size());
       i++) {
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
        if (pCloseRes.has_value()) {
          auto hasPrimary =
              isPrimaryWithin(TokenType::semiColon, i, pCloseRes.value());
          if (hasPrimary) {
            Error(
                "Tuple type found after another type",
                utils::FileRange(token.fileRange, RangeAt(pCloseRes.value())));
          } else {
            return {cacheTy.value(), i - 1};
          }
        } else {
          Error("Expected )", token.fileRange);
        }
      }
      auto pCloseResult = getPairEnd(TokenType::parenthesisOpen,
                                     TokenType::parenthesisClose, i, false);
      if (!pCloseResult.has_value()) {
        Error("Expected )", token.fileRange);
      }
      if (upto.has_value() && (pCloseResult.value() > upto.value())) {
        Error("Invalid position for )", RangeAt(pCloseResult.value()));
      }
      auto                pClose = pCloseResult.value();
      Vec<ast::QatType *> subTypes;
      for (usize j = i; j < pClose; j++) {
        if (isPrimaryWithin(TokenType::semiColon, j, pClose)) {
          auto semiPosResult = firstPrimaryPosition(TokenType::semiColon, j);
          if (!semiPosResult.has_value()) {
            Error("Invalid position of ; separator", token.fileRange);
          }
          auto semiPos = semiPosResult.value();
          subTypes.push_back(parseType(ctx, j, semiPos).first);
          j = semiPos - 1;
        } else if (j != (pClose - 1)) {
          subTypes.push_back(parseType(ctx, j, pClose).first);
          j = pClose;
        }
      }
      i       = pClose;
      cacheTy = new ast::TupleType(subTypes, false, getVariability(),
                                   token.fileRange);
      break;
    }
    case TokenType::voidType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      cacheTy = new ast::VoidType(getVariability(), token.fileRange);
      break;
    }
    case TokenType::boolType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      cacheTy = new ast::UnsignedType(1, getVariability(), token.fileRange);
      break;
    }
    case TokenType::stringSliceType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      cacheTy = new ast::StringSliceType(getVariability(), token.fileRange);
      break;
    }
    case TokenType::cstringType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      cacheTy = new ast::CStringType(getVariability(), token.fileRange);
      break;
    }
    case TokenType::unsignedIntegerType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      cacheTy = new ast::UnsignedType(std::stoul(token.value), getVariability(),
                                      token.fileRange);
      break;
    }
    case TokenType::integerType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      cacheTy = new ast::IntegerType(std::stoul(token.value), getVariability(),
                                     token.fileRange);
      break;
    }
    case TokenType::floatType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
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
    case TokenType::super:
    case TokenType::identifier: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      auto symRes = parseSymbol(ctx, i);
      i           = symRes.second;
      if (isNext(TokenType::templateTypeStart, i)) {
        // TODO - Implement template type parsing
      }
      cacheTy = new ast::NamedType(symRes.first.relative, symRes.first.name,
                                   getVariability(), token.fileRange);
      break;
    }
    case TokenType::referenceType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      auto subRes = parseType(ctx, i, upto);
      i           = subRes.second;
      cacheTy =
          new ast::ReferenceType(subRes.first, getVariability(),
                                 utils::FileRange(token.fileRange, RangeAt(i)));
      break;
    }
    case TokenType::pointerType: {
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      if (isNext(TokenType::bracketOpen, i)) {
        auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                    TokenType::bracketClose, i + 1, false);
        if (bCloseRes.has_value() &&
            (!upto.has_value() || (bCloseRes.value() < upto.value()))) {
          auto bClose     = bCloseRes.value();
          auto subTypeRes = parseType(ctx, i + 1, bClose);
          i               = bClose;
          cacheTy         = new ast::PointerType(
                      subTypeRes.first, getVariability(),
                      utils::FileRange(token.fileRange, RangeAt(bClose)));
          break;
        } else {
          Error("Invalid end for pointer type", RangeAt(i));
        }
      } else {
        Error("Type of the pointer not specified", token.fileRange);
      }
      break;
    }
    case TokenType::bracketOpen: {
      if (!cacheTy.has_value()) {
        Error("Element type of array not specified", token.fileRange);
      }
      if (isNext(TokenType::unsignedLiteral, i) ||
          isNext(TokenType::integerLiteral, i)) {
        if (isNext(TokenType::bracketClose, i + 1)) {
          auto *subType      = cacheTy.value();
          auto  numberString = tokens.at(i + 1).value;
          if (numberString.find('_') != String::npos) {
            numberString = numberString.substr(0, numberString.find('_'));
          }
          cacheTy = new ast::ArrayType(subType, std::stoul(numberString),
                                       getVariability(), token.fileRange);
          i       = i + 2;
          break;
        } else {
          Error("Expected ] after the array size", RangeAt(i + 1));
        }
      } else {
        Error("Expected non negative number of elements for the array",
              token.fileRange);
      }
      break;
    }
    default: {
      if (!cacheTy.has_value()) {
        Error("No type found", RangeAt(from));
      }
      return {cacheTy.value(), i - 1};
    }
    }
  }
  if (!cacheTy.has_value()) {
    Error("No type found", RangeAt(from));
  }
  return {cacheTy.value(), i - 1};
}

Vec<ast::Node *>
Parser::parse(ParserContext prev_ctx, // NOLINT(misc-no-recursion)
              usize from, usize upto) {
  Vec<ast::Node *> result;
  using lexer::Token;
  using lexer::TokenType;

  if (upto == -1) {
    upto = tokens.size();
  }

  ParserContext ctx(prev_ctx);

  Maybe<CacheSymbol> c_sym = None;
  auto setCachedSymbol     = [&c_sym](const CacheSymbol &sym) { c_sym = sym; };
  auto getCachedSymbol     = [&c_sym]() {
    auto result = c_sym.value();
    c_sym       = None;
    return result;
  };
  auto hasCachedSymbol = [&c_sym]() { return c_sym.has_value(); };

  std::deque<Token>          cacheT;
  std::deque<ast::QatType *> cacheTy;
  String                     context = "global";

  bool isVar  = false;
  auto getVar = [&isVar]() {
    auto result = isVar;
    isVar       = false;
    return result;
  };
  auto setVar = [&isVar]() { isVar = true; };

  Maybe<utils::VisibilityKind> visibility;
  auto setVisibility = [&](utils::VisibilityKind kind) { visibility = kind; };
  auto getVisibility = [&]() {
    auto res   = visibility.value_or(utils::VisibilityKind::parent);
    visibility = None;
    return res;
  };

  for (usize i = (from + 1); i < upto; i++) {
    Token &token = tokens.at(i);
    switch (token.type) { // NOLINT(clang-diagnostic-switch)
    case TokenType::endOfFile: {
      return result;
    }
    case TokenType::Public: {
      auto kindRes = parseVisibilityKind(i);
      setVisibility(kindRes.first);
      i = kindRes.second;
      break;
    }
    case TokenType::parenthesisOpen:
    case TokenType::boolType:
    case TokenType::voidType:
    case TokenType::cstringType:
    case TokenType::stringSliceType:
    case TokenType::unsignedIntegerType:
    case TokenType::referenceType:
    case TokenType::integerType:
    case TokenType::pointerType:
    case TokenType::floatType: {
      auto start      = i;
      auto typeResult = parseType(ctx, i - 1, None);
      i               = typeResult.second;
      if (isNext(TokenType::identifier, i)) {
        ast::Expression *exp = nullptr;
        if (isNext(TokenType::assignment, i + 1)) {
          auto endRes = firstPrimaryPosition(TokenType::stop, i + 2);
          if (endRes.has_value()) {
            auto end = endRes.value();

          } else {
            Error("Expected . to end the declaration of the global entity",
                  RangeSpan(start, i + 2));
          }
        }
      }
      break;
    }
    case TokenType::bring: {
      if (isNext(TokenType::identifier, i)) {
        auto endRes = firstPrimaryPosition(TokenType::stop, i);
        if (endRes.has_value()) {
          auto *broughtEntities = parseBroughtEntities(ctx, i, endRes.value());
        }
      } else if (isNext(TokenType::StringLiteral, i)) {
        auto endRes = firstPrimaryPosition(TokenType::stop, i);
        if (endRes.has_value()) {
          auto broughtFiles = parseBroughtFilesOrFolders(i, endRes.value());
        } else {
          Error("Expected end of bring sentence", token.fileRange);
        }
      }
      break;
    }
    case TokenType::Type: {
      // TODO - Consider other possible tokens and template types instead of
      // just identifiers
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::assignment, i + 1)) {
          SHOW("Parsing type definition")
          auto endRes = firstPrimaryPosition(TokenType::stop, i + 2);
          if (endRes.has_value()) {
            auto *typ = parseType(prev_ctx, i + 2, endRes.value()).first;
            result.push_back(new ast::TypeDefinition(
                tokens.at(i + 1).value, typ, RangeSpan(i, endRes.value()),
                getVisibility()));
            i = endRes.value();
            break;
          } else {
            Error("Invalid end of type definition", RangeSpan(i, i + 2));
          }
        } else if (isNext(TokenType::curlybraceOpen, i + 1)) {
          auto bClose = getPairEnd(TokenType::curlybraceOpen,
                                   TokenType::curlybraceClose, i + 2, false);
          if (bClose.has_value()) {
            auto *tRes =
                parseCoreType(ctx, i + 2, bClose.value(),
                              new ast::DefineCoreType(
                                  tokens.at(i + 1).value, getVisibility(),
                                  utils::FileRange(token.fileRange,
                                                   RangeAt(bClose.value()))));
            result.push_back(tRes);
            i = bClose.value();
            break;
          } else {
            Error("Invalid end for declaring a type", RangeAt(i + 2));
          }
        }
      } else if (isNext(TokenType::child, i)) {
        if (isNext(TokenType::alias, i + 1)) {
          /* Type alias declaration */
          if (isNext(TokenType::identifier, i + 2)) {
            auto t_ident = tokens.at(i + 3);
            if (ctx.has_type_alias(t_ident.value) ||
                ctx.has_alias(t_ident.value)) {
              Error( //
                  String(ctx.has_alias(t_ident.value) ? "An" : "A type") +
                      "alias with the name `" + t_ident.value +
                      "` already exists",
                  t_ident.fileRange);
            } else {
              if (isNext(TokenType::assignment, i + 3)) {
                auto end_res = firstPrimaryPosition(TokenType::stop, i + 4);
                if (end_res.has_value()) {
                  auto type_res = parseType(ctx, i + 4, end_res.value());
                  ctx.add_type_alias(t_ident.value, type_res.first);
                  i = end_res.value();
                } else {
                  Error("Invalid end for the sentence",
                        utils::FileRange(token.fileRange, RangeAt(i + 4)));
                }
              } else {
                Error("Expected assignment after type alias name",
                      utils::FileRange(token.fileRange, RangeAt(i + 3)));
              }
            }
          } else {
            Error("Expected name for the type alias",
                  utils::FileRange(token.fileRange, RangeAt(i + 2)));
          }
        }
      } else {
        Error("Expected name for the type", token.fileRange);
      }
      break;
    }
    case TokenType::lib: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::curlybraceOpen, i + 1)) {
          auto bClose = getPairEnd(TokenType::curlybraceOpen,
                                   TokenType::curlybraceClose, i + 2, false);
          if (bClose.has_value()) {
            auto contents = parse(ctx, i + 2, bClose.value());
            result.push_back(new ast::Lib(
                tokens.at(i + 1).value, contents, getVisibility(),
                utils::FileRange(token.fileRange, RangeAt(bClose.value()))));
            i = bClose.value();
          } else {
            Error("Expected } to close the lib", RangeAt(i + 2));
          }
        } else {
          Error("Expected { after name for the lib", RangeAt(i + 1));
        }
      } else {
        Error("Expected name for the lib", token.fileRange);
      }
      break;
    }
    case TokenType::box: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::curlybraceOpen, i + 1)) {
          auto bClose = getPairEnd(TokenType::curlybraceOpen,
                                   TokenType::curlybraceClose, i + 2, false);
          if (bClose.has_value()) {
            auto contents = parse(ctx, i + 2, bClose.value());
            result.push_back(new ast::Box(
                tokens.at(i + 1).value, contents, getVisibility(),
                utils::FileRange(token.fileRange, RangeAt(bClose.value()))));
            i = bClose.value();
          } else {
            Error("Expected } to close the box", RangeAt(i + 2));
          }
        } else {
          Error("Expected { after name for the box", RangeAt(i + 1));
        }
      } else {
        Error("Expected name for the box", token.fileRange);
      }
      break;
    }
    case TokenType::alias: {
      /* Alias declaration */
      if (isNext(TokenType::identifier, i)) {
        auto t_ident = tokens.at(i + 1);
        if (ctx.has_type_alias(t_ident.value) || ctx.has_alias(t_ident.value)) {
          Error(String(ctx.has_alias(t_ident.value) ? "An" : "A type") +
                    "alias with the name `" + t_ident.value +
                    "` already exists",
                t_ident.fileRange);
        } else {
          if (isNext(TokenType::assignment, i + 1)) {
            auto end_res = firstPrimaryPosition(TokenType::stop, i + 2);
            if (end_res.has_value()) {
              if (isNext(TokenType::identifier, i + 2)) {
                auto sym_res = parseSymbol(ctx, i + 3);
                ctx.add_alias(t_ident.value, sym_res.first.name);
                i = end_res.value();
              } else {
                Error("Expected an identifier representing an entity for "
                      "the alias",
                      RangeAt(i + 2));
              }
            } else {
              Error("Invalid end for the sentence",
                    utils::FileRange(token.fileRange, RangeAt(i + 2)));
            }
          } else {
            Error("Expected assignment after alias name",
                  utils::FileRange(token.fileRange, RangeAt(i + 1)));
          }
        }
      } else {
        Error("Expected name for the type alias", token.fileRange);
      }
      break;
    }
    case TokenType::var: {
      auto typeRes = parseType(prev_ctx, i, None);
      i            = typeRes.second;
      if (isNext(TokenType::identifier, i)) {

      } else {
        Error("Expected identifier for the name of the global variable after "
              "the type",
              typeRes.first->fileRange);
      }
      break;
    }
    case TokenType::identifier: {
      auto start   = i;
      auto sym_res = parseSymbol(ctx, i);
      i            = sym_res.second;
      if (isNext(TokenType::parenthesisOpen, i)) {
        SHOW("Function with void return type")
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i + 1, false);
        if (pCloseRes.has_value()) {
          auto pClose    = pCloseRes.value();
          auto argResult = parseFunctionParameters(prev_ctx, i + 1, pClose);
          SHOW("Parsed arguments")
          i = pClose;
          String callConv;
          if (isNext(TokenType::external, i)) {
            SHOW("Function is extern")
            if (isNext(TokenType::StringLiteral, i + 1)) {
              SHOW("Calling convention found")
              callConv = tokens.at(i + 2).value;
              i += 2;
            } else {
              SHOW("No calling convention")
              Error("Expected calling convention string after extern",
                    RangeAt(i + 1));
            }
          }
          SHOW("Creating prototype")
          auto *prototype = new ast::FunctionPrototype(
              sym_res.first.name, argResult.first, argResult.second,
              new ast::VoidType(false, RangeAt(start)), false,
              llvm::GlobalValue::WeakAnyLinkage, callConv, getVisibility(),
              RangeSpan(start, pClose));
          SHOW("Prototype created")
          if (isNext(TokenType::bracketOpen, i)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                        TokenType::bracketClose, i + 1, false);
            if (bCloseRes.has_value()) {
              auto  bClose     = bCloseRes.value();
              auto  sentences  = parseSentences(prev_ctx, i + 1, bClose);
              auto *definition = new ast::FunctionDefinition(
                  prototype, sentences, RangeSpan(i + 1, bClose));
              SHOW("Function definition created")
              result.push_back((ast::Node *)definition);
              SHOW("Parsing function complete")
              i = bClose;
              break;
            } else {
              // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
              Error("Expected end for [", RangeAt(i + 1));
            }
          } else if (isNext(TokenType::stop, i)) {
            result.push_back(prototype);
            i++;
            break;
          } else {
            // FIXME - Implement
          }
        } else {
          Error("Expected end for (", RangeAt(i + 1));
        }
      } else {
        setCachedSymbol(sym_res.first);
      }
      break;
    }
    case TokenType::givenTypeSeparator: {
      // TODO - Add support for template types for functions
      if (!hasCachedSymbol()) {
        Error("Function name not provided", token.fileRange);
      }
      SHOW("Parsing Type")
      auto retTypeRes = parseType(ctx, i, None);
      SHOW("Type parsing complete")
      auto *retType = retTypeRes.first;
      i             = retTypeRes.second;
      if (isNext(TokenType::parenthesisOpen, i)) {
        SHOW("Found (")
        auto pCloseResult =
            getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose,
                       i + 1, false);
        if (!pCloseResult.has_value()) {
          Error("Expected )", RangeAt(i + 1));
        }
        SHOW("Found )")
        auto pClose = pCloseResult.value();
        SHOW("Parsing function parameters")
        auto argResult = parseFunctionParameters(ctx, i + 1, pClose);
        SHOW("Fn Params complete")
        bool   isExternal = false;
        String callConv;
        if (isNext(TokenType::stop, pClose)) {
          Error("Expected external keyword or a definition", RangeAt(pClose));
        } else if (isNext(TokenType::external, pClose)) {
          isExternal = true;
          if (isNext(TokenType::StringLiteral, pClose + 1)) {
            callConv = tokens.at(pClose + 2).value;
            if (!isNext(TokenType::stop, pClose + 2)) {
              // TODO - Sync errors
              Error("Expected the function declaration to end here. "
                    "Please add `.` here",
                    RangeAt(pClose + 2));
            }
            i = pClose + 3;
          } else {
            Error("Expected Calling Convention string", token.fileRange);
          }
        }
        SHOW("Argument count: " << argResult.first.size())
        for (auto *arg : argResult.first) {
          SHOW("Arg name " << arg->getName())
        }
        /// TODO: Change implementation of Box
        auto *prototype = new ast::FunctionPrototype(
            getCachedSymbol().name, argResult.first, argResult.second,
            // FIXME - Support async
            retType, false,
            isExternal ? llvm::GlobalValue::ExternalLinkage
                       : llvm::GlobalValue::WeakAnyLinkage,
            // FIXME - Support other visibilities
            callConv, getVisibility(), token.fileRange);
        SHOW("Prototype created")
        if (!isExternal) {
          if (isNext(TokenType::bracketOpen, pClose)) {
            auto bCloseResult =
                getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                           pClose + 1, false);
            if (!bCloseResult.has_value() ||
                (bCloseResult.value() >= tokens.size())) {
              Error("Expected ] at the end of the Function Definition",
                    RangeAt(pClose + 1));
            }
            SHOW("HAS BCLOSE")
            auto bClose = bCloseResult.value();
            SHOW("Starting sentence parsing")
            auto sentences = parseSentences(ctx, pClose + 1, bClose);
            SHOW("Sentence parsing completed")
            auto *definition = new ast::FunctionDefinition(
                prototype, sentences,
                utils::FileRange(RangeAt(pClose + 1), RangeAt(bClose)));
            SHOW("Function definition created")
            result.push_back(definition);
            i = bClose;
            continue;
          } else {
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
            Error("Expected definition for non-external function",
                  token.fileRange);
          }
        } else {
          // TODO - Implement this for external functions
        }
      } else {
        Error("Expected (", token.fileRange);
      }
      break;
    }
    }
  }
  return result;
}

Pair<utils::VisibilityKind, usize> Parser::parseVisibilityKind(usize from) {
  using lexer::TokenType;
  if (isNext(TokenType::child, from)) {
    if (isNext(TokenType::Type, from + 1)) {
      return {utils::VisibilityKind::type, from + 2};
    } else if (isNext(TokenType::lib, from + 1)) {
      return {utils::VisibilityKind::lib, from + 2};
    } else if (isNext(TokenType::box, from + 1)) {
      return {utils::VisibilityKind::box, from + 2};
    } else if (isNext(TokenType::identifier, from + 1)) {
      auto val = tokens.at(from + 2).value;
      if (val == "file") {
        return {utils::VisibilityKind::file, from + 2};
      } else if (val == "folder") {
        return {utils::VisibilityKind::folder, from + 2};
      } else {
        Error("Invalid identifier found after pub' for visibility",
              RangeAt(from + 2));
      }
    } else {
      Error("Invalid token found after pub' for visibility",
            RangeSpan(from, from + 1));
    }
  } else {
    return {utils::VisibilityKind::pub, from};
  }
} // NOLINT(clang-diagnostic-return-type)

// FIXME - Finish functionality for parsing type contents
ast::DefineCoreType *Parser::parseCoreType(ParserContext &prev_ctx, usize from,
                                           usize                upto,
                                           ast::DefineCoreType *coreTy) {
  using lexer::Token;
  using lexer::TokenType;

  bool isConst  = false;
  auto setConst = [&]() { isConst = true; };
  auto getConst = [&]() {
    bool res = isConst;
    isConst  = false;
    return res;
  };

  bool isVariation  = false;
  auto setVariation = [&]() { isVariation = true; };
  auto getVariation = [&]() {
    bool res    = isVariation;
    isVariation = false;
    return res;
  };

  bool isStatic  = false;
  auto setStatic = [&]() { isStatic = true; };
  auto getStatic = [&]() {
    bool res = isStatic;
    isStatic = false;
    return res;
  };

  bool isAsync  = false;
  auto setAsync = [&]() { isAsync = true; };
  auto getAsync = [&]() {
    bool res = isAsync;
    isAsync  = false;
    return res;
  };

  Maybe<utils::VisibilityKind> broadVisib;
  Maybe<utils::VisibilityKind> visibility;
  auto setVisibility = [&](utils::VisibilityKind kind) { visibility = kind; };
  auto getVisibility = [&]() {
    auto res =
        visibility.value_or(broadVisib.value_or(utils::VisibilityKind::type));
    visibility = None;
    return res;
  };

  Maybe<ast::QatType *> cacheTy;

  for (usize i = from + 1; i < upto; i++) {
    Token &token = tokens.at(i);
    switch (token.type) {
    case TokenType::Public: {
      auto kindRes = parseVisibilityKind(i);
      if (isNext(TokenType::colon, kindRes.second)) {
        broadVisib = kindRes.first;
        i          = kindRes.second + 1;
      } else {
        setVisibility(kindRes.first);
        i = kindRes.second;
      }
      break;
    }
    case TokenType::constant: {
      setConst();
      break;
    }
    case TokenType::Static: {
      if (!isNext(TokenType::Async, i) && !isNext(TokenType::identifier, i)) {
        Error("Unexpected token after static", RangeAt(i));
      }
      setStatic();
      break;
    }
    case TokenType::variationMarker: {
      if (!isNext(TokenType::identifier, i) && !isNext(TokenType::Async, i)) {
        Error(
            "Expected identifier for the name of the member function after <~",
            RangeAt(i));
      }
      setVariation();
      break;
    }
    case TokenType::Async: {
      if (!isNext(TokenType::identifier, i)) {
        Error("Expected identifier after async. Unexpected token found",
              RangeAt(i));
      }
      setAsync();
      break;
    }
    case TokenType::boolType:
    case TokenType::voidType:
    case TokenType::pointerType:
    case TokenType::referenceType:
    case TokenType::unsignedIntegerType:
    case TokenType::integerType:
    case TokenType::floatType:
    case TokenType::stringSliceType:
    case TokenType::parenthesisOpen:
    case TokenType::cstringType: {
      auto typeRes = parseType(prev_ctx, i - 1, None);
      cacheTy      = typeRes.first;
      i            = typeRes.second;
      break;
    }
    case TokenType::identifier: {
      auto start = i;
      if (isNext(TokenType::givenTypeSeparator, i) ||
          isNext(TokenType::parenthesisOpen, i)) {
        SHOW("Member function start")
        ast::QatType *retTy = isNext(TokenType::parenthesisOpen, i)
                                  ? new ast::VoidType(false, RangeAt(i))
                                  : nullptr;
        if (!retTy) {
          auto typeRes = parseType(prev_ctx, i + 1, None);
          retTy        = typeRes.first;
          i            = typeRes.second;
        }
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes =
              getPairEnd(TokenType::parenthesisOpen,
                         TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto pClose  = pCloseRes.value();
            auto argsRes = parseFunctionParameters(prev_ctx, i + 1, pClose);
            if (isNext(TokenType::bracketOpen, pClose)) {
              auto bCloseRes =
                  getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                             pClose + 1, false);
              if (bCloseRes.has_value()) {
                auto bClose = bCloseRes.value();
                auto snts   = parseSentences(prev_ctx, pClose + 1, bClose);
                SHOW("Creating member function prototype")
                coreTy->addMemberDefinition(new ast::MemberDefinition(
                    getStatic()
                        ? ast::MemberPrototype::Static(
                              tokens.at(start).value, argsRes.first,
                              argsRes.second, retTy, getAsync(),
                              getVisibility(), RangeSpan(start, pClose + 1))
                        : ast::MemberPrototype::Normal(
                              getVariation(), tokens.at(start).value,
                              argsRes.first, argsRes.second, retTy, getAsync(),
                              getVisibility(), RangeSpan(start, pClose + 1)),
                    snts, RangeSpan(start, bClose)));
                i = bClose;
              }
            }
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        } else {
          Error("Expected ( for the arguments of the member function",
                RangeSpan(start, i));
        }
      } else if (cacheTy.has_value()) {
        if (isNext(TokenType::stop, i)) {
          coreTy->addMember(new ast::DefineCoreType::Member(
              cacheTy.value(), token.value, !getConst(), getVisibility(),
              utils::FileRange(cacheTy.value()->fileRange, token.fileRange)));
          cacheTy = None;
          i++;
        } else {
          Error("Expected . after member declaration", token.fileRange);
        }
      } else if (isNext(TokenType::identifier, i)) {
        auto typeRes = parseType(prev_ctx, i, None);
        cacheTy      = typeRes.first;
        i            = typeRes.second;
      }
      break;
    }
    case TokenType::from: {
      auto start = i;
      if (isNext(TokenType::parenthesisOpen, i)) {
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i + 1, false);
        if (pCloseRes.has_value()) {
          auto pClose = pCloseRes.value();
          if (!isPrimaryWithin(TokenType::semiColon, i + 1, pClose)) {
            // Constructor
            auto args = parseFunctionParameters(prev_ctx, i + 1, pClose);
            // FIXME - Implement constructor support
          }
        } else {
          Error("Expected end for (", RangeAt(i + 1));
        }
      }
      auto typeRes = parseType(prev_ctx, i, upto);
      i            = typeRes.second;
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::altArrow, i + 1)) {
          if (isNext(TokenType::bracketOpen, i + 2)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                        TokenType::bracketClose, i + 3, false);
            if (bCloseRes.has_value()) {
              auto bClose = bCloseRes.value();
              auto snts   = parseSentences(prev_ctx, i + 3, bClose);
              coreTy->addConvertorDefinition(new ast::ConvertorDefinition(
                  ast::ConvertorPrototype::From(tokens.at(i + 1).value,
                                                typeRes.first, getVisibility(),
                                                RangeSpan(start, i + 2)),
                  snts, RangeSpan(start, bClose)));
              i = bClose;
            } else {
              Error("Expected end for [", RangeAt(i + 3));
            }
          } else {
            Error("Expected [ to start the body of the convertor",
                  RangeSpan(start, i + 2));
          }
        } else {
          Error("Expected => to start the definition of the convertor",
                RangeSpan(start, i + 1));
        }
      } else {
        Error("Expected identifier for the argument name of the convertor",
              typeRes.first->fileRange);
      }
      break;
    }
    case TokenType::to: {
      break;
    }
    default: {
    }
    }
  }
  return coreTy;
}

Pair<ast::Expression *, usize>
Parser::parseExpression(ParserContext &prev_ctx, // NOLINT(misc-no-recursion)
                        const Maybe<CacheSymbol> &symbol, usize from,
                        Maybe<usize> upto, bool isMemberFn) {
  using ast::Expression;
  using lexer::Token;
  using lexer::TokenType;

  bool varCall = false;
  auto setVar  = [&varCall]() { varCall = true; };
  auto getVar  = [&varCall]() {
    auto result = varCall;
    varCall     = false;
    return result;
  };

  Maybe<CacheSymbol> cachedSymbol = symbol;
  Vec<Expression *>  cachedExpressions;
  Vec<Token>         cachedBinaryOps;
  Vec<Token>         cachedUnaryOps;
  int                pointerCount = 0;

  usize i = from + 1; // NOLINT(readability-identifier-length)
  for (; upto.has_value() ? (i < upto.value()) : (i < tokens.size()); i++) {
    Token &token = tokens.at(i);
    switch (token.type) {
    case TokenType::unsignedLiteral: {
      if ((token.value.find('_') != String::npos) &&
          (token.value.find('u') != (token.value.length() - 1))) {
        u64    bits  = 32; // NOLINT(readability-magic-numbers)
        auto   split = token.value.find('_');
        String number(token.value.substr(0, split));
        if ((split + 2) < token.value.length()) {
          bits = std::stoul(token.value.substr(split + 2));
        }
        cachedExpressions.push_back(
            new ast::CustomIntegerLiteral(number, true, bits, token.fileRange));
      } else {
        cachedExpressions.push_back(
            new ast::UnsignedLiteral(token.value, token.fileRange));
      }
      break;
    }
    case TokenType::integerLiteral: {
      if (token.value.find('_') != String::npos) {
        u64    bits  = 32; // NOLINT(readability-magic-numbers)
        auto   split = token.value.find('_');
        String number(token.value.substr(0, split));
        if ((split + 2) < token.value.length()) {
          bits = std::stoul(token.value.substr(split + 2));
        }
        cachedExpressions.push_back(new ast::CustomIntegerLiteral(
            number, false, bits, token.fileRange));
      } else {
        cachedExpressions.push_back(
            new ast::IntegerLiteral(token.value, token.fileRange));
      }
      break;
    }
    case TokenType::floatLiteral: {
      auto number = token.value;
      if (number.find('_') != String::npos) {
        SHOW("Found custom float literal: "
             << number.substr(number.find('_') + 1))
        cachedExpressions.push_back(new ast::CustomFloatLiteral(
            number.substr(0, number.find('_')),
            number.substr(number.find('_') + 1), token.fileRange));
      } else {
        SHOW("Found float literal")
        cachedExpressions.push_back(
            new ast::FloatLiteral(token.value, token.fileRange));
      }
      break;
    }
    case TokenType::StringLiteral: {
      if (!cachedExpressions.empty()) {
        if (cachedExpressions.back()->nodeType() ==
            ast::NodeType::stringLiteral) {
          ((ast::StringLiteral *)cachedExpressions.back())
              ->addValue(token.value, token.fileRange);
        }
      } else {
        cachedExpressions.push_back(
            new ast::StringLiteral(token.value, token.fileRange));
      }
      break;
    }
    case TokenType::null: {
      cachedExpressions.push_back(new ast::NullPointer(token.fileRange));
      break;
    }
    case TokenType::super:
    case TokenType::identifier: {
      auto symbol_res = parseSymbol(prev_ctx, i);
      cachedSymbol    = symbol_res.first;
      i               = symbol_res.second;
      break;
    }
    case TokenType::self: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::parenthesisOpen, i + 1)) {
          // FIXME - Self member function call
        } else {
          cachedExpressions.push_back(
              new ast::SelfMember(tokens.at(i + 1).value, RangeSpan(i, i + 1)));
          i++;
        }
      } else {
        cachedExpressions.push_back(new ast::Self(RangeAt(i)));
      }
      break;
    }
    case TokenType::curlybraceOpen: {
      if (cachedSymbol.has_value()) {
        auto cCloseRes = getPairEnd(TokenType::curlybraceOpen,
                                    TokenType::curlybraceClose, i, false);
        if (cCloseRes.has_value()) {
          auto cClose = cCloseRes.value();
          // FIXME - Maybe change associated assignment to assignment?
          if (isPrimaryWithin(TokenType::associatedAssignment, i, cClose)) {
            Vec<Pair<String, utils::FileRange>> fields;
            Vec<ast::Expression *>              fieldValues;
            for (usize j = i + 1; j < cClose; j++) {
              if (isNext(TokenType::identifier, j - 1)) {
                fields.push_back(Pair<String, utils::FileRange>(
                    tokens.at(j).value, tokens.at(j).fileRange));
                if (isNext(TokenType::associatedAssignment, j)) {
                  if (isPrimaryWithin(TokenType::separator, j + 1, cClose)) {
                    auto sepRes =
                        firstPrimaryPosition(TokenType::separator, j + 1);
                    if (sepRes) {
                      auto sep = sepRes.value();
                      if (sep == j + 2) {
                        Error("No expression for the member found",
                              RangeSpan(j + 1, j + 2));
                      }
                      fieldValues.push_back(
                          parseExpression(prev_ctx, None, j + 1, sep).first);
                      j = sep;
                      continue;
                    } else {
                      Error("Expected ,", RangeSpan(j + 1, cClose));
                    }
                  } else {
                    if (cClose == j + 2) {
                      Error("No expression for the member found",
                            RangeSpan(j + 1, j + 2));
                    }
                    fieldValues.push_back(
                        parseExpression(prev_ctx, None, j + 1, cClose).first);
                    j = cClose;
                  }
                } else {
                  Error("Expected := after the name of the member", RangeAt(j));
                }
              } else {
                Error("Expected an identifier for the name of the member "
                      "of the core type",
                      RangeAt(j));
              }
            }
            cachedExpressions.push_back(new ast::PlainInitialiser(
                new ast::NamedType(cachedSymbol->relative, cachedSymbol->name,
                                   false, cachedSymbol->fileRange),
                fields, fieldValues, RangeSpan(i, cClose)));
          } else {
            auto exps = parseSeparatedExpressions(prev_ctx, i, cClose);
            cachedExpressions.push_back(new ast::PlainInitialiser(
                new ast::NamedType(cachedSymbol->relative, cachedSymbol->name,
                                   false, cachedSymbol->fileRange),
                {}, exps, RangeSpan(i + 1, cClose)));
          }
          cachedSymbol = None;
          i            = cClose;
        } else {
          Error("Expected end for {", RangeAt(i + 1));
        }
      } else {
        // FIXME - Support maps
        Error("No type specified for plain initialisation", RangeAt(i));
      }
      break;
    }
    case TokenType::bracketOpen: {
      SHOW("Found [")
      auto bCloseRes =
          getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
      if (bCloseRes) {
        if (cachedSymbol.has_value()) {
          cachedExpressions.push_back(new ast::Entity(cachedSymbol->relative,
                                                      cachedSymbol->name,
                                                      cachedSymbol->fileRange));
          cachedSymbol = None;
        }
        if (cachedExpressions.empty() || !cachedBinaryOps.empty()) {
          auto bClose = bCloseRes.value();
          if (bClose == i + 1) {
            // Empty array literal
            cachedExpressions.push_back(
                new ast::ArrayLiteral({}, RangeSpan(i, bClose)));
            i = bClose;
          } else {
            auto vals = parseSeparatedExpressions(prev_ctx, i, bClose);
            cachedExpressions.push_back(
                new ast::ArrayLiteral(vals, RangeSpan(i, bClose)));
            i = bClose;
          }
          break;
        } else if (!cachedExpressions.empty()) {
          auto *exp =
              parseExpression(prev_ctx, cachedSymbol, i, bCloseRes.value())
                  .first;
          auto *ent = cachedExpressions.back();
          cachedExpressions.pop_back();
          cachedExpressions.push_back(
              new ast::IndexAccess(ent, exp, {ent->fileRange, exp->fileRange}));
          i = bCloseRes.value();
        }
      } else {
        Error("Invalid end for [", token.fileRange);
      }
      break;
    }
    case TokenType::parenthesisOpen: {
      SHOW("Found paranthesis")
      if (cachedBinaryOps.empty() &&
          (!cachedExpressions.empty() || cachedSymbol.has_value())) {
        // This parenthesis is supposed to indicate a function call
        auto p_close = getPairEnd(TokenType::parenthesisOpen,
                                  TokenType::parenthesisClose, i, false);
        if (p_close.has_value()) {
          SHOW("Found end of paranthesis")
          if (p_close.value() >= upto) {
            Error("Invalid position of )", token.fileRange);
          } else {
            SHOW("About to parse arguments")
            Vec<ast::Expression *> args;
            if (isPrimaryWithin(TokenType::separator, i, p_close.value())) {
              args = parseSeparatedExpressions(prev_ctx, i, p_close.value());
            } else if (i < (p_close.value() - 1)) {
              args.push_back(
                  parseExpression(prev_ctx, None, i, p_close.value()).first);
            }
            if (cachedExpressions.empty()) {
              SHOW("Expressions cache is empty")
              if (cachedSymbol.has_value()) {
                SHOW("Normal function call")
                auto *ent =
                    new ast::Entity(cachedSymbol->relative, cachedSymbol->name,
                                    cachedSymbol->fileRange);
                cachedExpressions.push_back(
                    new ast::FunctionCall(ent, args,
                                          cachedSymbol.value().extend_fileRange(
                                              RangeAt(p_close.value()))));
                cachedSymbol = None;
              } else {
                auto varVal = getVar();
                Error(String("No expression found to be passed to the ") +
                          (varVal ? "variation " : "") + "function call." +
                          (varVal ? ""
                                  : " And no function name found for "
                                    "the static function call"),
                      utils::FileRange(token.fileRange,
                                       RangeAt(p_close.value())));
              }
            } else {
              auto *funCall = new ast::FunctionCall(
                  cachedExpressions.back(), args,
                  utils::FileRange(cachedExpressions.back()->fileRange,
                                   RangeAt(p_close.value())));
              cachedExpressions.pop_back();
              cachedExpressions.push_back(funCall);
              cachedSymbol = None;
            }
            i = p_close.value();
          }
        } else {
          Error("Expected ) to close the scope started by this opening "
                "paranthesis",
                token.fileRange);
        }
      } else {
        auto p_close_res = getPairEnd(TokenType::parenthesisOpen,
                                      TokenType::parenthesisClose, i, false);
        if (!p_close_res.has_value()) {
          Error("Expected )", token.fileRange);
        }
        if (p_close_res.value() >= upto) {
          Error("Invalid position of )", token.fileRange);
        }
        auto p_close = p_close_res.value();
        if (isPrimaryWithin(TokenType::semiColon, i, p_close)) {
          Vec<ast::Expression *> values;
          auto                   separations =
              primaryPositionsWithin(TokenType::semiColon, i, p_close);
          values.push_back(
              parseExpression(prev_ctx, cachedSymbol, i, separations.front())
                  .first);
          bool shouldContinue = true;
          if (separations.size() == 1) {
            SHOW("Only 1 separation")
            shouldContinue = (separations.at(0) != (p_close - 1));
            SHOW("Found condition")
          }
          if (shouldContinue) {
            for (usize j = 0; j < (separations.size() - 1); j += 1) {
              values.push_back(parseExpression(prev_ctx, cachedSymbol,
                                               separations.at(j),
                                               separations.at(j + 1))
                                   .first);
            }
            values.push_back(parseExpression(prev_ctx, cachedSymbol,
                                             separations.back(), p_close)
                                 .first);
          }
          cachedExpressions.push_back(new ast::TupleValue(
              values, utils::FileRange(token.fileRange, RangeAt(p_close))));
          i = p_close;
        } else {
          auto *exp = parseExpression(prev_ctx, None, i, p_close).first;
          cachedExpressions.push_back(exp);
          i = p_close;
        }
      }
      break;
    }
    case TokenType::loop: {
      if (isNext(TokenType::child, i)) {
        if (isNext(TokenType::identifier, i + 1)) {
          cachedExpressions.push_back(new ast::LoopIndex(
              (tokens.at(i + 2).value == "index") ? "" : tokens.at(i + 2).value,
              {token.fileRange, RangeAt(i + 2)}));
          i = i + 2;
        } else {
          Error("Unexpected token after loop'",
                {token.fileRange, RangeAt(i + 1)});
        }
      } else {
        Error("Invalid token after loop", token.fileRange);
      }
      break;
    }
    case TokenType::pointerType: {
      // FIXME - Implement after reconsidering
      break;
    }
    case TokenType::binaryOperator: {
      SHOW("Binary operator found: " << token.value)
      if (cachedExpressions.empty() && !cachedSymbol.has_value()) {
        Error("No expression found on the left side of the binary operator " +
                  token.value,
              token.fileRange);
      } else if (cachedSymbol.has_value()) {
        cachedExpressions.push_back(new ast::Entity(cachedSymbol->relative,
                                                    cachedSymbol->name,
                                                    cachedSymbol->fileRange));
        cachedBinaryOps.push_back(token);
        cachedSymbol = None;
      } else {
        cachedBinaryOps.push_back(token);
      }
      break;
    }
    case TokenType::to: {
      SHOW("Found to expression")
      ast::Expression *source = nullptr;
      if (cachedExpressions.empty()) {
        if (cachedSymbol.has_value()) {
          source = new ast::Entity(cachedSymbol->relative, cachedSymbol->name,
                                   cachedSymbol->fileRange);
          cachedSymbol = None;
        }
      } else {
        source = cachedExpressions.back();
        cachedExpressions.pop_back();
      }
      if (source) {
        auto destTyRes = parseType(prev_ctx, i, upto);
        cachedExpressions.push_back(new ast::ToConversion(
            source, destTyRes.first,
            utils::FileRange(source->fileRange, RangeAt(destTyRes.second))));
        i = destTyRes.second;
      } else {
        Error("No expression found for conversion", token.fileRange);
      }
      break;
    }
    case TokenType::ternary: {
      ast::Expression *cond = nullptr;
      if (cachedExpressions.empty()) {
        if (cachedSymbol.has_value()) {
          cond = new ast::Entity(cachedSymbol->relative, cachedSymbol->name,
                                 cachedSymbol->fileRange);
          cachedSymbol = None;
        } else {
          Error("No expression found before ternary operator", RangeAt(i));
        }
      } else {
        cond = cachedExpressions.back();
        cachedExpressions.pop_back();
      }
      if (isNext(TokenType::parenthesisOpen, i)) {
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i + 1, false);
        if (pCloseRes.has_value()) {
          auto pClose = pCloseRes.value();
          if (isPrimaryWithin(TokenType::separator, i + 1, pClose)) {
            auto splitRes = firstPrimaryPosition(TokenType::separator, i + 1);
            if (splitRes.has_value()) {
              auto  split = splitRes.value();
              auto *trueExpr =
                  parseExpression(prev_ctx, None, i + 1, split).first;
              auto *falseExpr =
                  parseExpression(prev_ctx, None, split, pClose).first;
              cachedExpressions.push_back(new ast::TernaryExpression(
                  cond, trueExpr, falseExpr,
                  {cond->fileRange, tokens.at(pClose).fileRange}));
              i = pClose;
            } else {
              Error("Expected 2 expressions here for true and false cases "
                    "of the ternary expression",
                    RangeSpan(i + 1, pClose));
            }
          } else {
            Error("Expected 2 expressions here for true and false cases "
                  "of the ternary expression",
                  RangeSpan(i + 1, pClose));
          }
        } else {
          Error("Expected end for (", RangeAt(i + 1));
        }
      } else {
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
        Error("Expected ( after ternary operator", RangeAt(i));
      }
      break;
    }
    case TokenType::variationMarker: {
      if (!cachedExpressions.empty() || cachedSymbol.has_value()) {
        if (cachedExpressions.empty() && cachedSymbol.has_value()) {
          SHOW("Expression empty, using symbol")
          cachedExpressions.push_back(new ast::Entity(cachedSymbol->relative,
                                                      cachedSymbol->name,
                                                      cachedSymbol->fileRange));
          cachedSymbol = None;
        }
        auto *exp = cachedExpressions.back();
        cachedExpressions.pop_back();
        if (isNext(TokenType::identifier, i)) {
          if (isNext(TokenType::parenthesisOpen, i + 1)) {
            auto pCloseRes =
                getPairEnd(TokenType::parenthesisOpen,
                           TokenType::parenthesisClose, i + 2, false);
            if (pCloseRes.has_value()) {
              auto pClose = pCloseRes.value();
              auto args   = parseSeparatedExpressions(prev_ctx, i + 2, pClose);
              cachedExpressions.push_back(new ast::MemberFunctionCall(
                  exp, false, tokens.at(i + 1).value, args, true,
                  {exp->fileRange, RangeAt(pClose)}));
              i = pClose;
              break;
            } else {
              Error("Expected end for (", RangeAt(i + 2));
            }
          } else {
            Error("Expected ( to start variation function call",
                  RangeSpan(i, i + 1));
          }
        }
      }
    }
    case TokenType::child: {
      SHOW("Expression parsing : Member access")
      if (!cachedExpressions.empty() || cachedSymbol.has_value()) {
        if (cachedExpressions.empty() && cachedSymbol.has_value()) {
          SHOW("Expression empty, using symbol")
          cachedExpressions.push_back(new ast::Entity(cachedSymbol->relative,
                                                      cachedSymbol->name,
                                                      cachedSymbol->fileRange));
          cachedSymbol = None;
        }
        auto *exp = cachedExpressions.back();
        cachedExpressions.pop_back();
        if (isNext(TokenType::identifier, i)) {
          // TODO - Support template function calls
          if (isNext(TokenType::parenthesisOpen, i + 1)) {
            auto pCloseRes =
                getPairEnd(TokenType::parenthesisOpen,
                           TokenType::parenthesisClose, i + 2, false);
            if (pCloseRes.has_value()) {
              auto pClose = pCloseRes.value();
              auto args   = parseSeparatedExpressions(prev_ctx, i + 2, pClose);
              cachedExpressions.push_back(new ast::MemberFunctionCall(
                  exp, false, tokens.at(i + 1).value, args, false,
                  {exp->fileRange, RangeAt(pClose)}));
              i = pClose;
              break;
            } else {
              Error("Expected end for (", RangeAt(i + 2));
            }
          } else {
            cachedExpressions.push_back(
                new ast::MemberAccess(exp, false, tokens.at(i + 1).value,
                                      {exp->fileRange, RangeAt(i + 1)}));
            i++;
          }
        } else {
          Error("Expected an identifier for member access", RangeAt(i));
        }
      } else {
        Error("No expression found for member access", RangeAt(i));
      }
      break;
    }
    default: {
      if (upto.has_value()) {
        if ((!cachedExpressions.empty()) && (upto.value() == i)) {
          return {cachedExpressions.back(), i - 1};
        } else {
          Error("Encountered an invalid token in the expression", RangeAt(i));
        }
      } else {
        if (!cachedExpressions.empty()) {
          return {cachedExpressions.back(), i - 1};
        } else {
          Error("No expression found and encountered invalid token",
                RangeAt(i));
        }
      }
    }
    }
    if (!cachedBinaryOps.empty()) {
      SHOW("Binary ops are not empty")
      if (cachedExpressions.size() >= 2) {
        SHOW("Found lhs and rhs of binary exp")
        auto *rhs = cachedExpressions.back();
        cachedExpressions.pop_back();
        auto *lhs = cachedExpressions.back();
        cachedExpressions.clear();
        cachedExpressions.push_back(new ast::BinaryExpression(
            lhs, cachedBinaryOps.back().value, rhs,
            utils::FileRange(lhs->fileRange, rhs->fileRange)));
        cachedBinaryOps.clear();
      } else if (!cachedExpressions.empty() && cachedSymbol.has_value()) {
        SHOW("Found lhs exp and cached symbol")
        auto *lhs = cachedExpressions.back();
        cachedExpressions.clear();
        auto *rhs = new ast::Entity(cachedSymbol->relative, cachedSymbol->name,
                                    cachedSymbol->fileRange);
        cachedSymbol = None;
        cachedExpressions.push_back(new ast::BinaryExpression(
            lhs, cachedBinaryOps.back().value, rhs,
            utils::FileRange(lhs->fileRange, rhs->fileRange)));
        cachedBinaryOps.clear();
      }
    }
  }
  if (cachedExpressions.empty()) {
    if (cachedSymbol.has_value()) {
      return {new ast::Entity(cachedSymbol->relative, cachedSymbol->name,
                              cachedSymbol->fileRange),
              i};
    } else {
      Error("No expression found", RangeAt(from));
    }
  } else if (!cachedBinaryOps.empty()) {
    if (cachedExpressions.size() >= 2) {
      auto *rhs = cachedExpressions.back();
      cachedExpressions.pop_back();
      auto *lhs = cachedExpressions.back();
      cachedExpressions.pop_back();
      return {new ast::BinaryExpression(lhs, cachedBinaryOps.back().value, rhs,
                                        {lhs->fileRange, rhs->fileRange}),
              i};
    } else {
      Error("2 expressions required for binary expression not found",
            cachedBinaryOps.back().fileRange);
    }
  } else {
    return {cachedExpressions.back(), i};
  }
} // NOLINT(clang-diagnostic-return-type)

Vec<ast::Expression *> Parser::
    parseSeparatedExpressions( // NOLINT(misc-no-recursion),
                               // NOLINTNEXTLINE(readability-identifier-length)
        ParserContext &prev_ctx, usize from, usize to) {
  Vec<ast::Expression *> result;
  for (usize i = from + 1; i < to; i++) {
    if (isPrimaryWithin(lexer::TokenType::separator, i - 1, to)) {
      auto endResult = firstPrimaryPosition(lexer::TokenType::separator, i - 1);
      if (!endResult.has_value() || (endResult.value() >= to)) {
        Error("Invalid position for separator `,`", RangeAt(i));
      }
      auto end = endResult.value();
      result.push_back(parseExpression(prev_ctx, None, i - 1, end).first);
      i = end;
    } else {
      result.push_back(parseExpression(prev_ctx, None, i - 1, to).first);
      i = to;
    }
  }
  return result;
}

Pair<CacheSymbol, usize> Parser::parseSymbol(ParserContext &prev_ctx,
                                             const usize    start) {
  using lexer::TokenType;
  String name;
  if (isNext(TokenType::identifier, start - 1) ||
      isNext(TokenType::super, start - 1)) {
    u32  relative = isNext(TokenType::super, start - 1);
    auto prev     = tokens.at(start).type;
    auto i        = start; // NOLINT(readability-identifier-length)
    if (isNext(TokenType::super, start - 1)) {
      while (prev == TokenType::super
                 ? isNext(TokenType::colon, i)
                 : (prev == TokenType::colon ? isNext(TokenType::super, i)
                                             : false)) {
        if (isNext(TokenType::super, i)) {
          relative++;
        }
        prev = tokens.at(i + 1).type;
        i++;
      }
    }
    if (prev == TokenType::super) {
      if (!isNext(TokenType::colon, i)) {
        Error("No : found after ..", RangeAt(i));
      }
    } else if (prev == TokenType::colon) {
      if (!isNext(TokenType::identifier, i)) {
        Error("No identifier found after :", RangeAt(i));
      }
    }
    if (prev != TokenType::identifier) {
      i++;
    }
    for (; i < tokens.size(); i++) {
      auto tok = tokens.at(i);
      if (tok.type == TokenType::identifier) {
        if (i == start || prev != TokenType::identifier) {
          name += tok.value;
          prev = TokenType::identifier;
        } else {
          return {CacheSymbol(relative, name, start, RangeAt(start)), i - 1};
        }
      } else if (tok.type == TokenType::colon) {
        if (prev == TokenType::identifier) {
          name += ":";
        } else {
          Error("No identifier found before `:`. Anonymous boxes are "
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
    return {CacheSymbol(relative, name, start, RangeAt(start)), i - 1};
  } else {
    // TODO - Change this, after verifying that this situation is never
    // encountered.
    Error("No identifier found for parsing the symbol", RangeAt(start));
  }
} // NOLINT(clang-diagnostic-return-type)

Vec<ast::Sentence *> Parser::parseSentences(ParserContext &prev_ctx, usize from,
                                            usize upto, bool onlyOne,
                                            bool isMemberFn) {
  using ast::FloatType;
  using ast::IntegerType;
  using IR::FloatTypeKind;
  using lexer::Token;
  using lexer::TokenType;

  auto                 ctx = ParserContext(prev_ctx);
  Vec<ast::Sentence *> result;

  Maybe<CacheSymbol> cacheSymbol = None;
  // NOTE - Potentially remove this variable
  Vec<ast::QatType *>    cacheTy;
  Vec<ast::Expression *> cachedExpressions;

  String context;
  String name_val;

  auto var = false;

  for (usize i = from + 1; i < upto; i++) {
    Token &token = tokens.at(i);
    switch (token.type) {
    case TokenType::voidType:
    case TokenType::integerType:
    case TokenType::floatType:
    case TokenType::boolType:
    case TokenType::curlybraceOpen:
    case TokenType::unsignedIntegerType:
    case TokenType::referenceType:
    case TokenType::var:
    case TokenType::pointerType: {
      if ((token.type == TokenType::var) && isPrev(TokenType::let, i)) {
        // NOTE - This might cause some invalid type parsing
        // This will be continued in the next run where the parser is supposed
        // to encounter an identifier
        break;
      }
      auto typeRes = parseType(ctx, i - 1, None);
      auto old     = i;
      i            = typeRes.second;
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::assignment, i + 1)) {
          auto endRes = firstPrimaryPosition(TokenType::stop, i + 2);
          if (endRes.has_value()) {
            auto *exp =
                parseExpression(prev_ctx, None, i + 2, endRes.value()).first;
            result.push_back(new ast::LocalDeclaration(
                typeRes.first, false, tokens.at(i + 1).value, exp,
                typeRes.first->isVariable(), RangeSpan(old, endRes.value())));
            i = endRes.value();
            break;
          } else {
            Error("Invalid end for sentence", RangeSpan(old, i + 2));
          }
        } else if (isNext(TokenType::from, i + 1)) {
          // TODO - Implement constructor calls
        } else if (isNext(TokenType::stop, i + 1)) {
          result.push_back(new ast::LocalDeclaration(
              typeRes.first, false, tokens.at(i + 1).value, nullptr,
              typeRes.first->isVariable(), RangeSpan(old, i + 1)));
          i += 2;
          break;
        } else {
          Error("Invalid token found in local declaration", RangeAt(i + 1));
        }
      } else {
        cacheTy.push_back(typeRes.first);
      }
      break;
    }
    case TokenType::parenthesisOpen: {
      auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                  TokenType::parenthesisClose, i, false);
      if (pCloseRes.has_value()) {
        auto pClose = pCloseRes.value();
        if (isNext(TokenType::altArrow, pClose)) {
          /* Closure definition */
          if (isNext(TokenType::bracketOpen, pClose + 1)) {
            auto bCloseRes =
                getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                           pClose + 2, false);
            if (bCloseRes.has_value()) {
              auto endRes =
                  firstPrimaryPosition(TokenType::stop, bCloseRes.value());
              if (endRes.has_value()) {
                auto *exp =
                    parseExpression(ctx, cacheSymbol, i - 1, endRes.value())
                        .first;
                cacheSymbol = None;
                i           = endRes.value();
                result.push_back(new ast::ExpressionSentence(
                    exp, utils::FileRange(token.fileRange,
                                          RangeAt(endRes.value()))));
                break;
              } else {
                Error("End for the sentence not found",
                      utils::FileRange(token.fileRange,
                                       RangeAt(bCloseRes.value())));
              }
            } else {
              Error("Invalid end for definition of closure",
                    RangeAt(pClose + 2));
            }
          } else {
            Error("Definition for closure not found", RangeAt(pClose + 1));
          }
        } else if (isNext(TokenType::identifier, pClose)) {
          /* Tuple value declaration */
          auto typeRes = parseType(ctx, i - 1, pClose + 1);
          i            = typeRes.second;
          cacheTy.push_back(typeRes.first);
          break;
        } else {
          /* Expression */
          auto endRes = firstPrimaryPosition(TokenType::stop, pClose);
          if (endRes.has_value()) {
            auto *exp =
                parseExpression(ctx, cacheSymbol, i - 1, endRes.value()).first;
            i           = endRes.value();
            cacheSymbol = None;
            result.push_back(new ast::ExpressionSentence(
                exp,
                utils::FileRange(token.fileRange, RangeAt(endRes.value()))));
            break;
          } else {
            Error("End of sentence not found", token.fileRange);
          }
        }
      } else {
        Error("Invalid end of parenthesis", token.fileRange);
      }
      break;
    }
    case TokenType::child: {
      if (cacheSymbol.has_value()) {
        auto expRes = parseExpression(ctx, cacheSymbol.value(), i, None);
        cachedExpressions.push_back(expRes.first);
        cacheSymbol = None;
        i           = expRes.second;
      } else {
        Error("No expression or entity found to access the child of",
              token.fileRange);
      }
      break;
    }
    case TokenType::super: {
      auto symRes = parseSymbol(prev_ctx, i);
      cacheSymbol = symRes.first;
      i           = symRes.second;
      break;
    }
    case TokenType::identifier: {
      context = "IDENTIFIER";
      SHOW("Identifier encountered")
      if (cacheSymbol.has_value() || !cacheTy.empty()) {
        // Declaration
        // A normal declaration where the type of the entity is provided by the
        // user
        if (isNext(TokenType::assignment, i)) {
          SHOW("Declaration encountered")
          auto end_res = firstPrimaryPosition(TokenType::stop, i);
          if (end_res.has_value() && (end_res.value() < upto)) {
            auto *exp =
                parseExpression(ctx, None, i + 1, end_res.value()).first;
            result.push_back(new ast::LocalDeclaration(
                (cacheTy.empty()
                     ? (new ast::NamedType(cacheSymbol.value().relative,
                                           cacheSymbol.value().name, false,
                                           cacheSymbol.value().fileRange))
                     : cacheTy.back()),
                false, token.value, exp, var, token.fileRange));
            var = false;
            cacheTy.clear();
            cacheSymbol = None;
            i           = end_res.value();
            break;
          } else {
            Error("Invalid end for declaration syntax", RangeAt(i + 1));
          }
        } else if (isNext(TokenType::from, i)) {
          // TODO - Handle constructor call
        }
        // FIXME - Handle this case
      } else if (isNext(TokenType::child, i)) {
        auto expRes = parseExpression(prev_ctx, None, i - 1, None);
        cachedExpressions.push_back(expRes.first);
        i = expRes.second;
      } else {
        auto sym_res = parseSymbol(ctx, i);
        cacheSymbol  = sym_res.first;
        i            = sym_res.second;
      }
      break;
    }
    case TokenType::assignment: {
      SHOW("Assignment found")
      /* Assignment */
      if (cacheSymbol.has_value()) {
        auto end_res = firstPrimaryPosition(TokenType::stop, i);
        if (!end_res.has_value() || (end_res.value() >= upto)) {
          Error("Invalid end for the sentence", token.fileRange);
        }
        auto  end = end_res.value();
        auto *exp = parseExpression(ctx, None, i, end).first;
        auto *lhs = new ast::Entity(cacheSymbol->relative, cacheSymbol->name,
                                    cacheSymbol->fileRange);
        result.push_back(new ast::Assignment(
            lhs, exp,
            utils::FileRange(cacheSymbol.value().fileRange, RangeAt(end))));
        var = false;
        cacheTy.clear();
        cacheSymbol = None;
        i           = end;
      } else if (!cachedExpressions.empty()) {
        auto endRes = firstPrimaryPosition(TokenType::stop, i);
        if (endRes.has_value()) {
          auto  end = endRes.value();
          auto *exp = parseExpression(prev_ctx, None, i, end).first;
          result.push_back(new ast::Assignment(
              cachedExpressions.back(), exp,
              {cachedExpressions.back()->fileRange, RangeAt(end)}));
          cacheSymbol = None;
          cachedExpressions.clear();
          i = end;
        } else {
          Error("Invalid end of sentence",
                {cachedExpressions.back()->fileRange, token.fileRange});
        }
      } else {
        Error("Expected an expression to assign the expression to",
              token.fileRange);
      }
      break;
    }
    case TokenType::say: {
      context      = "SAY";
      auto end_res = firstPrimaryPosition(TokenType::stop, i);
      if (!end_res.has_value() || (end_res.value() >= upto)) {
        Error("Say sentence has invalid end", token.fileRange);
      }
      auto end  = end_res.value();
      auto exps = parseSeparatedExpressions(ctx, i, end);
      result.push_back(new ast::Say(exps, token.fileRange));
      i = end;
      break;
    }
    case TokenType::stop: {
      SHOW("Parsed expression sentence")
      if (!cachedExpressions.empty()) {
        result.push_back(new ast::ExpressionSentence(
            cachedExpressions.back(),
            {cachedExpressions.back()->fileRange, token.fileRange}));
        cachedExpressions.pop_back();
      }
      break;
    }
    case TokenType::let: {
      const auto start = i;
      context          = "LET";
      SHOW("Found let")
      bool isRef     = false;
      bool isVarDecl = false;
      if (isNext(TokenType::var, i)) {
        isVarDecl = true;
        if (isNext(TokenType::referenceType, i + 1)) {
          isRef = true;
          i += 2;
        } else {
          i += 1;
        }
      } else if (isNext(TokenType::referenceType, i)) {
        isRef = true;
        i += 1;
      }
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::assignment, i + 1)) {
          auto endRes = firstPrimaryPosition(TokenType::stop, i + 2);
          if (endRes.has_value()) {
            auto  end = endRes.value();
            auto *exp = parseExpression(prev_ctx, None, i + 2, end).first;
            result.push_back(new ast::LocalDeclaration(
                nullptr, isRef, tokens.at(i + 1).value, exp, isVarDecl,
                RangeSpan(start, end)));
            cacheSymbol = None;
            cachedExpressions.clear();
            i = end;
          } else {
            Error("Invalid end for sentence", RangeSpan(start, i + 2));
          }
        } else if (isNext(TokenType::from, i + 1)) {
          Error("Initialisation via constructor or convertor can be used "
                "only if there is a type provided. This is an inferred "
                "declaration",
                RangeSpan(start, i + 2));
        } else if (isNext(TokenType::stop, i + 1)) {
          result.push_back(new ast::LocalDeclaration(
              nullptr, isRef, tokens.at(i + 1).value, nullptr, isVarDecl,
              RangeSpan(start, i + 2)));
          cacheSymbol = None;
          cachedExpressions.clear();
          i += 2;
        } else {
          Error("Unexpected token found after the beginning of inferred "
                "declaration",
                RangeAt(i + 1));
        }
      } else {
        Error("Expected an identifier for the name of the local value, in "
              "the inferred declaration",
              RangeSpan(start, i));
      }
      break;
    }
    case TokenType::If: {
      Vec<Pair<ast::Expression *, Vec<ast::Sentence *>>> chain;
      Maybe<Vec<ast::Sentence *>>                        elseCase;
      utils::FileRange fileRange = token.fileRange;
      while (true) {
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes =
              getPairEnd(TokenType::parenthesisOpen,
                         TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes) {
            auto *exp =
                parseExpression(prev_ctx, None, i + 1, pCloseRes.value()).first;
            if (isNext(TokenType::bracketOpen, pCloseRes.value())) {
              auto bCloseRes =
                  getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                             pCloseRes.value() + 1, false);
              if (bCloseRes.has_value()) {
                fileRange =
                    utils::FileRange(fileRange, RangeAt(bCloseRes.value()));
                auto snts = parseSentences(prev_ctx, pCloseRes.value() + 1,
                                           bCloseRes.value());
                chain.push_back(
                    Pair<ast::Expression *, Vec<ast::Sentence *>>(exp, snts));
                if (isNext(TokenType::Else, bCloseRes.value())) {
                  SHOW("Found else")
                  if (isNext(TokenType::If, bCloseRes.value() + 1)) {
                    i = bCloseRes.value() + 2;
                    continue;
                  } else if (isNext(TokenType::bracketOpen,
                                    bCloseRes.value() + 1)) {
                    SHOW("Else case begin")
                    auto bOp = bCloseRes.value() + 2;
                    auto bClRes =
                        getPairEnd(TokenType::bracketOpen,
                                   TokenType::bracketClose, bOp, false);
                    if (bClRes) {
                      fileRange =
                          utils::FileRange(fileRange, RangeAt(bClRes.value()));
                      elseCase = parseSentences(prev_ctx, bOp, bClRes.value());
                      i        = bClRes.value();
                      break;
                    } else {
                      Error("Expected end for [", RangeAt(bOp));
                    }
                  }
                } else {
                  i = bCloseRes.value();
                  break;
                }
              } else {
                Error("Expected end for [", RangeAt(bCloseRes.value()));
              }
            }
            // FIXME - Support single sentence
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        }
      }
      result.push_back(new ast::IfElse(chain, elseCase, fileRange));
      break;
    }
    case TokenType::loop: {
      if (isNext(TokenType::child, i)) {
        auto expRes = parseExpression(prev_ctx, None, i > 0 ? i - 1 : 0, None);
        cachedExpressions.push_back(expRes.first);
        i = expRes.second;
        break;
      } else if (isNext(TokenType::colon, i) ||
                 isNext(TokenType::bracketOpen, i)) {
        SHOW("Infinite loop")
        auto          pos = i;
        Maybe<String> tag = None;
        if (isNext(TokenType::colon, i)) {
          if (isNext(TokenType::identifier, i + 1)) {
            tag = tokens.at(i + 2).value;
            pos = i + 2;
          } else {
            Error("Expected identifier after : for the tag for the loop",
                  RangeAt(i + 1));
          }
        }
        if (isNext(TokenType::bracketOpen, pos)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                      TokenType::bracketClose, pos + 1, false);
          if (bCloseRes.has_value()) {
            auto bClose    = bCloseRes.value();
            auto sentences = parseSentences(prev_ctx, pos + 1, bClose);
            result.push_back(
                new ast::LoopInfinite(sentences, tag, RangeSpan(i, bClose)));
            i = bClose;
          } else {
            Error("Expected end for [", RangeAt(pos + 1));
          }
        }
      } else if (isNext(TokenType::parenthesisOpen, i)) {
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i + 1, false);
        if (pCloseRes.has_value()) {
          auto          pClose  = pCloseRes.value();
          Maybe<String> loopTag = None;
          auto *count = parseExpression(prev_ctx, None, i + 1, pClose).first;
          auto  pos   = pClose;
          if (isNext(TokenType::colon, pClose)) {
            if (isNext(TokenType::identifier, pClose + 1)) {
              loopTag = tokens.at(pClose + 2).value;
              pos     = pClose + 2;
            } else {
              Error("Expected an identifier for the tag for the loop",
                    RangeAt(pClose + 1));
            }
          }
          if (isNext(TokenType::bracketOpen, pos)) {
            auto bCloseRes =
                getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                           pos + 1, false);
            if (bCloseRes.has_value()) {
              auto snts = parseSentences(prev_ctx, pos + 1, bCloseRes.value());
              result.push_back(new ast::LoopNTimes(
                  count, parseSentences(prev_ctx, pos + 1, bCloseRes.value()),
                  loopTag, RangeSpan(i, bCloseRes.value())));
              i = bCloseRes.value();
            } else {
              Error("Expected end for [", RangeAt(pos + 1));
            }
          }
        } else {
          Error("Expected end for (", RangeAt(i + 1));
        }
      } else if (isNext(TokenType::While, i)) {
        if (isNext(TokenType::parenthesisOpen, i + 1)) {
          auto pCloseRes =
              getPairEnd(TokenType::parenthesisOpen,
                         TokenType::parenthesisClose, i + 2, false);
          if (pCloseRes.has_value()) {
            auto  pClose = pCloseRes.value();
            auto *cond   = parseExpression(prev_ctx, None, i + 2, pClose).first;
            if (isNext(TokenType::bracketOpen, pCloseRes.value())) {
              auto bCloseRes =
                  getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                             pClose + 1, false);
              if (bCloseRes.has_value()) {
                auto snts =
                    parseSentences(prev_ctx, pClose + 1, bCloseRes.value());
                result.push_back(new ast::LoopWhile(
                    cond, snts, None, RangeSpan(i, bCloseRes.value())));
                i = bCloseRes.value();
                break;
              } else {
                Error("Expected end for [", RangeAt(pCloseRes.value() + 1));
              }
            } else if (isNext(TokenType::colon, pClose)) {
              if (isNext(TokenType::identifier, pClose + 1)) {
                if (isNext(TokenType::bracketOpen, pClose + 2)) {
                  auto bCloseRes =
                      getPairEnd(TokenType::bracketOpen,
                                 TokenType::bracketClose, pClose + 3, false);
                  if (bCloseRes) {
                    auto snts =
                        parseSentences(prev_ctx, pClose + 3, bCloseRes.value());
                    result.push_back(new ast::LoopWhile(
                        cond, snts, tokens.at(pClose + 2).value,
                        RangeSpan(i, bCloseRes.value())));
                    i = bCloseRes.value();
                  } else {
                    Error("Expected end for [", RangeAt(pClose + 3));
                  }
                } else {
                  // FIXME - Implement single statement loop
                }
              } else {
                Error("Expected name for the tag for the loop",
                      RangeAt(pCloseRes.value()));
              }
            } else {
              // FIXME - Implement single statement loop
            }
          } else {
            Error("Expected end for (", RangeAt(i + 2));
          }
        }
      } else if (isNext(TokenType::over, i)) {
        // TODO - Implement
      } else {
        Error("Unexpected token after loop", token.fileRange);
      }
      break;
    }
    case TokenType::give: {
      SHOW("give sentence found")
      context = "GIVE";
      if (isNext(TokenType::stop, i)) {
        i++;
        result.push_back(new ast::GiveSentence(
            None, utils::FileRange(token.fileRange, RangeAt(i + 1))));
      } else {
        auto end = firstPrimaryPosition(TokenType::stop, i);
        if (!end.has_value()) {
          Error("Expected give sentence to end. Please add `.` wherever "
                "appropriate to mark the end of the statement",
                token.fileRange);
        }
        auto *exp   = parseExpression(ctx, cacheSymbol, i, end.value()).first;
        i           = end.value();
        cacheSymbol = None;
        result.push_back(new ast::GiveSentence(
            exp, utils::FileRange(token.fileRange, RangeAt(end.value()))));
      }
      break;
    }
    case TokenType::bracketOpen: {
      auto expRes = parseExpression(prev_ctx, cacheSymbol, i - 1, None);
      cachedExpressions.push_back(expRes.first);
      cacheSymbol = None;
      i           = expRes.second;
      break;
    }
    case TokenType::Break: {
      if (isNext(TokenType::child, i)) {
        if (isNext(TokenType::identifier, i + 1)) {
          result.push_back(
              new ast::Break(tokens.at(i + 2).value, RangeSpan(i, i + 2)));
          i += 2;
        } else {
          Error("Expected an identifier after break'", RangeSpan(i, i + 2));
        }
      } else if (isNext(TokenType::stop, i)) {
        result.push_back(new ast::Break(None, token.fileRange));
        i++;
      } else {
        Error("Unexpected token found after break", token.fileRange);
      }
      break;
    }
    case TokenType::Continue: {
      if (isNext(TokenType::child, i)) {
        if (isNext(TokenType::identifier, i + 1)) {
          result.push_back(
              new ast::Continue(tokens.at(i + 2).value, RangeSpan(i, i + 2)));
          i += 2;
        } else {
          Error("Expected an identifier after continue'", RangeSpan(i, i + 2));
        }
      } else if (isNext(TokenType::stop, i)) {
        result.push_back(new ast::Continue(None, token.fileRange));
        i++;
      } else {
        Error("Unexpected token found after continue", token.fileRange);
      }
      break;
    }
    default: {
      auto expRes = parseExpression(prev_ctx, cacheSymbol, i - 1, None);
      cachedExpressions.push_back(expRes.first);
      cacheSymbol = None;
      i           = expRes.second;
    }
    }
    if (onlyOne) {
      if (!result.empty()) {
        return result;
      }
    }
  }
  return result;
}

Pair<Vec<ast::Argument *>, bool>
Parser::parseFunctionParameters(ParserContext &prev_ctx, usize from,
                                usize upto) {
  using ast::FloatType;
  using ast::IntegerType;
  using IR::FloatTypeKind;
  using lexer::TokenType;

  Vec<ast::Argument *> args;

  Maybe<ast::QatType *> typ;
  auto                  hasType = [&typ]() { return typ.has_value(); };
  auto                  useType = [&typ]() {
    if (typ.has_value()) {
      auto *result = typ.value();
      typ          = None;
      return result;
    } else {
      return (ast::QatType *)nullptr;
    }
  };

  Maybe<String> name;
  auto          hasName = [&name]() { return name.has_value(); };
  auto          useName = [&name]() {
    if (name.has_value()) {
      auto result = name.value();
      name        = None;
      return result;
    } else {
      return String("");
    }
  };

  for (usize i = from + 1; ((i < upto) && (i < tokens.size())); i++) {
    auto &token = tokens.at(i);
    switch (token.type) { // NOLINT(clang-diagnostic-switch)
    case TokenType::voidType: {
      Error("Arguments cannot have void type", token.fileRange);
      break;
    }
    case TokenType::super: {
      auto typeRes = parseType(prev_ctx, i - 1, None);
      typ          = typeRes.first;
      i            = typeRes.second;
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
          Error("Expected , or ) after argument name", token.fileRange);
        }
      } else if (hasName()) {
        Error("Additional name provided after the previous one. Please "
              "remove this name",
              token.fileRange);
      } else {
        auto typeRes = parseType(prev_ctx, i - 1, None);
        typ          = typeRes.first;
        i            = typeRes.second;
      }
      break;
    }
    case TokenType::var:
    case TokenType::pointerType:
    case TokenType::referenceType:
    case TokenType::stringSliceType:
    case TokenType::boolType:
    case TokenType::floatType:
    case TokenType::unsignedIntegerType:
    case TokenType::integerType: {
      if (hasType()) {
        Error("A type is already provided before. Please change this "
              "to the name of the argument, or remove the previous type",
              token.fileRange);
      } else {
        auto typeRes = parseType(prev_ctx, i - 1, None);
        typ          = typeRes.first;
        i            = typeRes.second;
        break;
      }
    }
    case TokenType::variadic: {
      if (isNext(TokenType::identifier, i)) {
        args.push_back(ast::Argument::Normal(
            tokens.at(i + 1).value,
            utils::FileRange(token.fileRange, RangeAt(i + 1)), nullptr));
        if (isNext(TokenType::parenthesisClose, i + 1) ||
            (isNext(TokenType::separator, i + 1) &&
             isNext(TokenType::parenthesisClose, i + 2))) {
          return {args, true};
        } else {
          Error("Variadic argument should be the last argument of the function",
                token.fileRange);
        }
      } else {
        Error("Expected name for the variadic argument. Please provide a name",
              token.fileRange);
      }
    }
    case TokenType::self: {
      if (isNext(TokenType::identifier, i)) {
        args.push_back(ast::Argument::ForConstructor(
            tokens.at(i + 1).value,
            utils::FileRange(token.fileRange, RangeAt(i + 1)), nullptr, true));
        i++;
      } else {
        Error("Expected name of the member to be initialised", token.fileRange);
      }
    }
    }
  }
  // FIXME - Support variadic function parameters
  return {args, false};
}

Maybe<usize> Parser::getPairEnd(const lexer::TokenType startType,
                                const lexer::TokenType endType,
                                const usize current, const bool respectScope) {
  usize collisions = 0;
  for (usize i = current + 1; i < tokens.size(); i++) {
    if (respectScope) {
      for (auto scopeTk : TokenFamily::scopeLimiters) {
        if (tokens.at(i).type == scopeTk) {
          return None;
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
  return None;
}

bool Parser::isPrev(const lexer::TokenType type, const usize from) {
  if ((from - 1) >= 0) {
    return tokens.at(from - 1).type == type;
  } else {
    return false;
  }
}

bool Parser::isNext(const lexer::TokenType type, const usize current) {
  if ((current + 1) < tokens.size()) {
    return tokens.at(current + 1).type == type;
  } else {
    return false;
  }
}

bool Parser::areOnlyPresentWithin(const Vec<lexer::TokenType> &kinds,
                                  usize from, usize upto) {
  for (usize i = from + 1; i < upto; i++) {
    for (auto const &kind : kinds) {
      if (kind != tokens.at(i).type) {
        return false;
      }
    }
  }
  return true;
}

bool Parser::isPrimaryWithin(lexer::TokenType candidate, usize from,
                             usize upto) {
  auto pos_res = firstPrimaryPosition(candidate, from);
  return pos_res.has_value() && (pos_res.value() < upto);
}

Maybe<usize> Parser::firstPrimaryPosition(const lexer::TokenType candidate,
                                          const usize            from) {
  using lexer::TokenType;
  for (usize i = from + 1; i < tokens.size(); i++) {
    auto tok = tokens.at(i);
    if (tok.type == TokenType::parenthesisOpen) {
      auto end_res = getPairEnd(TokenType::parenthesisOpen,
                                TokenType::parenthesisClose, i, false);
      if (end_res.has_value() && (end_res.value() < tokens.size())) {
        i = end_res.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::bracketOpen) {
      auto end_res =
          getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
      if (end_res.has_value() && (end_res.value() < tokens.size())) {
        i = end_res.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::curlybraceOpen) {
      auto end_res = getPairEnd(TokenType::curlybraceOpen,
                                TokenType::curlybraceClose, i, false);
      if (end_res.has_value() && (end_res.value() < tokens.size())) {
        i = end_res.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::templateTypeStart) {
      auto end_res = getPairEnd(TokenType::templateTypeStart,
                                TokenType::templateTypeEnd, i, false);
      if (end_res.has_value() && (end_res.value() < tokens.size())) {
        i = end_res.value();
      } else {
        return None;
      }
    } else if (tok.type == candidate) {
      return i;
    }
  }
  return None;
}

Vec<usize> Parser::primaryPositionsWithin(lexer::TokenType candidate,
                                          usize from, usize upto) {
  Vec<usize> result;
  using lexer::TokenType;
  for (usize i = from + 1; (i < upto && i < tokens.size()); i++) {
    auto tok = tokens.at(i);
    if (tok.type == TokenType::parenthesisOpen) {
      auto end_res = getPairEnd(TokenType::parenthesisOpen,
                                TokenType::parenthesisClose, i, false);
      if (end_res.has_value() && (end_res.value() < upto)) {
        i = end_res.value();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::bracketOpen) {
      auto end_res =
          getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
      if (end_res.has_value() && (end_res.value() < upto)) {
        i = end_res.value();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::curlybraceOpen) {
      auto end_res = getPairEnd(TokenType::curlybraceOpen,
                                TokenType::curlybraceClose, i, false);
      if (end_res.has_value() && (end_res.value() < upto)) {
        i = end_res.value();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::templateTypeStart) {
      auto end_res = getPairEnd(TokenType::templateTypeStart,
                                TokenType::templateTypeEnd, i, false);
      if (end_res.has_value() && (end_res.value() < upto)) {
        i = end_res.value();
      } else {
        return result;
      }
    } else if (tok.type == candidate) {
      result.push_back(i);
    }
  }
  return result;
}

void Parser::Error(const String &message, const utils::FileRange &fileRange) {
  std::cout << colors::highIntensityBackground::red << " parser error "
            << colors::reset << "▌ " << colors::bold::red << message
            << colors::reset << " | " << colors::underline::green
            << fileRange.file.string() << ":" << fileRange.start.line << ":"
            << fileRange.start.character << colors::reset << " >> "
            << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.end.line << ":" << fileRange.end.character
            << colors::reset << "\n";
  tokens.clear();
  ast::Node::clearAll();
  exit(0);
}

void Parser::Warning(const String &message, const utils::FileRange &fileRange) {
  std::cout << colors::highIntensityBackground::yellow << " parser warning "
            << colors::reset << "▌ " << colors::bold::yellow << message
            << colors::reset << " | " << colors::underline::green
            << fileRange.file.string() << ":" << fileRange.start.line << ":"
            << fileRange.start.character << colors::reset << " >> "
            << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.end.line << ":" << fileRange.end.character
            << colors::reset << "\n";
}

} // namespace qat::parser
