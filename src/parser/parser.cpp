#include "./parser.hpp"
#include "../ast/constructor.hpp"
#include "../ast/define_mix_type.hpp"
#include "../ast/destructor.hpp"
#include "../ast/expressions/array_literal.hpp"
#include "../ast/expressions/binary_expression.hpp"
#include "../ast/expressions/boolean_literal.hpp"
#include "../ast/expressions/constructor_call.hpp"
#include "../ast/expressions/custom_float_literal.hpp"
#include "../ast/expressions/custom_integer_literal.hpp"
#include "../ast/expressions/default.hpp"
#include "../ast/expressions/dereference.hpp"
#include "../ast/expressions/entity.hpp"
#include "../ast/expressions/float_literal.hpp"
#include "../ast/expressions/function_call.hpp"
#include "../ast/expressions/heap.hpp"
#include "../ast/expressions/index_access.hpp"
#include "../ast/expressions/integer_literal.hpp"
#include "../ast/expressions/loop_index.hpp"
#include "../ast/expressions/member_access.hpp"
#include "../ast/expressions/member_function_call.hpp"
#include "../ast/expressions/mix_type_initialiser.hpp"
#include "../ast/expressions/move.hpp"
#include "../ast/expressions/null_pointer.hpp"
#include "../ast/expressions/plain_initialiser.hpp"
#include "../ast/expressions/self.hpp"
#include "../ast/expressions/self_member.hpp"
#include "../ast/expressions/size_of_type.hpp"
#include "../ast/expressions/template_entity.hpp"
#include "../ast/expressions/ternary.hpp"
#include "../ast/expressions/to_conversion.hpp"
#include "../ast/expressions/tuple_value.hpp"
#include "../ast/expressions/unsigned_literal.hpp"
#include "../ast/function.hpp"
#include "../ast/global_declaration.hpp"
#include "../ast/lib.hpp"
#include "../ast/member_function.hpp"
#include "../ast/operator_function.hpp"
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
#include "../ast/sentences/new_alias.hpp"
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
#include "../ast/types/template_named_type.hpp"
#include "../ast/types/tuple.hpp"
#include "../ast/types/unsigned.hpp"
#include "../show.hpp"
#include "cache_symbol.hpp"
#include "parser_context.hpp"
#include <string>
#include <utility>

#define ValueAt(ind) tokens->at(ind).value
#define RangeAt(ind) tokens->at(ind).fileRange
#define RangeSpan(ind1, ind2)                                                  \
  { tokens->at(ind1).fileRange, tokens->at(ind2).fileRange }

// NOTE - Check if file fileRange values are making use of the new merge
// functionality
namespace qat::parser {

Parser::Parser() = default;

void Parser::setTokens(Vec<lexer::Token> *allTokens) {
  g_ctx = ParserContext();
  delete tokens;
  tokens = allTokens;
}

void Parser::filterComments() {}

ast::BringEntities *Parser::parseBroughtEntities(ParserContext &ctx, usize from,
                                                 usize upto) {
  using lexer::TokenType;
  Vec<String>        result;
  Maybe<CacheSymbol> csym = None;

  for (usize i = from + 1; i < upto; i++) {
    auto token = tokens->at(i);
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
              entities.push_back(ValueAt(i + 2));
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
  for (usize i = from + 1; (i < upto) && (i < tokens->size()); i++) {
    auto token = tokens->at(i);
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

  for (i = from + 1; i < (upto.has_value() ? upto.value() : tokens->size());
       i++) {
    Token &token = tokens->at(i);
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
      cacheTy =
          new ast::UnsignedType(token.value == "usize"
                                    // NOLINTNEXTLINE(readability-magic-numbers)
                                    ? (sizeof(usize) * 8u)
                                    : std::stoul(token.value),
                                getVariability(), token.fileRange);
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
      auto start = i;
      if (cacheTy.has_value()) {
        return {cacheTy.value(), i - 1};
      }
      auto symRes = parseSymbol(ctx, i);
      auto name   = symRes.first.name;
      i           = symRes.second;
      SHOW("Checking templates for: " << name)
      if (prev_ctx.hasTemplate(name)) {
        cacheTy = new ast::TemplatedType(
            prev_ctx.getTemplate(name)->getTemplateID(), name, getVariability(),
            symRes.first.fileRange);
        break;
      } else {
        if (isNext(TokenType::templateTypeStart, i)) {
          auto endRes = getPairEnd(TokenType::templateTypeStart,
                                   TokenType::templateTypeEnd, i + 1, false);
          if (endRes.has_value()) {
            auto                end = endRes.value();
            Vec<ast::QatType *> types =
                parseSeparatedTypes(prev_ctx, i + 1, end);
            cacheTy = new ast::TemplateNamedType(
                symRes.first.relative, symRes.first.name, types,
                getVariability(), RangeSpan(start, end));
            i = end;
            break;
          } else {
            Error("Expected end for template type specification",
                  RangeAt(i + 1));
          }
        } else {
          cacheTy =
              new ast::NamedType(symRes.first.relative, name, getVariability(),
                                 symRes.first.fileRange);
          i = symRes.second;
          break;
        }
      }

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
          auto &numberString = ValueAt(i + 1);
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

Vec<ast::TemplatedType *> Parser::parseTemplateTypes(usize from, usize upto) {
  using lexer::TokenType;

  Vec<ast::TemplatedType *> result;
  for (usize i = from + 1; i < upto; i++) {
    auto token = tokens->at(i);
    if (token.type == TokenType::identifier) {
      result.push_back(new ast::TemplatedType(utils::unique_id(), token.value,
                                              false, token.fileRange));
      if (isNext(TokenType::separator, i)) {
        i++;
        continue;
      } else if (isNext(TokenType::templateTypeEnd, i)) {
        break;
      } else {
        Error(
            "Unexpected token after identifier in template type specification",
            token.fileRange);
      }
    }
  }
  return result;
}

Vec<ast::Node *>
Parser::parse(ParserContext prev_ctx, // NOLINT(misc-no-recursion)
              usize from, usize upto) {
  Vec<ast::Node *> result;
  using lexer::Token;
  using lexer::TokenType;

  if (upto == -1) {
    upto = tokens->size();
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

  Maybe<utils::VisibilityKind> visibility;
  auto setVisibility = [&](utils::VisibilityKind kind) { visibility = kind; };
  auto getVisibility = [&]() {
    auto res   = visibility.value_or(utils::VisibilityKind::parent);
    visibility = None;
    return res;
  };

  for (usize i = (from + 1); i < upto; i++) {
    Token &token = tokens->at(i);
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
            exp      = parseExpression(prev_ctx, None, i + 2, end).first;
            result.push_back(new ast::GlobalDeclaration(
                ValueAt(i + 1), typeResult.first, exp,
                typeResult.first->isVariable(), getVisibility(),
                RangeSpan(start, end)));
            i = end;
          } else {
            Error("Expected . to end the declaration of the global entity",
                  RangeSpan(start, i + 2));
          }
        }
      } else {
        cacheTy.push_back(typeResult.first);
      }
      break;
    }
    case TokenType::bring: {
      // FIXME - Support bring sentences
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
    case TokenType::mix: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::curlybraceOpen, i + 1)) {
          auto bCloseRes = getPairEnd(TokenType::curlybraceOpen,
                                      TokenType::curlybraceClose, i + 2, false);
          if (bCloseRes.has_value()) {
            auto                                     bClose = bCloseRes.value();
            Vec<Pair<String, Maybe<ast::QatType *>>> subTypes;
            Vec<utils::FileRange>                    fileRanges;
            parseMixType(prev_ctx, i + 2, bClose, subTypes, fileRanges);
            // FIXME - Support packing
            result.push_back(new ast::DefineMixType(
                ValueAt(i + 1), std::move(subTypes), std::move(fileRanges),
                false, getVisibility(), RangeSpan(i, bClose)));
            i = bClose;
          } else {
            Error("Expected end for {", RangeAt(i + 2));
          }
        } else {
          Error("Expected { to start the definition of the mix type",
                RangeSpan(i, i + 1));
        }
      } else {
        Error("Expected an identifier for the name of the mix type",
              RangeAt(i));
      }
      break;
    }
    case TokenType::choice: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::curlybraceOpen, i + 1)) {
          auto bCloseRes = getPairEnd(TokenType::curlybraceOpen,
                                      TokenType::curlybraceClose, i + 2, false);
          if (bCloseRes.has_value()) {
            auto bClose = bCloseRes.value();
            Vec<Pair<ast::DefineChoiceType::Field,
                     Maybe<ast::DefineChoiceType::Value>>>
                fields;
            parseChoiceType(i + 2, bClose, fields);
            result.push_back(new ast::DefineChoiceType(
                ValueAt(i + 1), std::move(fields), getVisibility(),
                RangeSpan(i, bClose)));
            i = bClose;
          } else {
            Error("Expected end for {", RangeAt(i + 2));
          }
        } else {
          Error("Expected { to start the definition of the choice type",
                RangeSpan(i, i + 1));
        }
      } else {
        Error("Expected an identifier for the name of the choice type",
              RangeAt(i));
      }
      break;
    }
    case TokenType::Type: {
      // TODO - Consider other possible tokens and template types instead of
      // just identifiers
      if (isNext(TokenType::identifier, i)) {
        auto                     &name = ValueAt(i + 1);
        Vec<ast::TemplatedType *> templates;
        auto                      ctx = ParserContext();
        i++;
        if (isNext(TokenType::templateTypeStart, i)) {
          auto endRes = getPairEnd(TokenType::templateTypeStart,
                                   TokenType::templateTypeEnd, i + 1, false);
          if (endRes.has_value()) {
            auto end  = endRes.value();
            templates = parseTemplateTypes(i + 1, end);
            for (auto *temp : templates) {
              SHOW("Template type parsed for core type " << temp->getName());
              ctx.addTemplate(temp);
            }
            i = end;
          } else {
            Error("Expected end for template specification", RangeAt(i + 1));
          }
        }
        if (isNext(TokenType::assignment, i)) {
          SHOW("Parsing type definition")
          auto endRes = firstPrimaryPosition(TokenType::stop, i + 1);
          if (endRes.has_value()) {
            auto *typ = parseType(ctx, i + 1, endRes.value()).first;
            result.push_back(new ast::TypeDefinition(
                name, typ, RangeSpan(i, endRes.value()), getVisibility()));
            i = endRes.value();
            break;
          } else {
            Error("Invalid end of type definition", RangeSpan(i, i + 2));
          }
        } else if (isNext(TokenType::curlybraceOpen, i)) {
          auto bClose = getPairEnd(TokenType::curlybraceOpen,
                                   TokenType::curlybraceClose, i + 1, false);
          if (bClose.has_value()) {
            auto *tRes = new ast::DefineCoreType(
                name, getVisibility(),
                {token.fileRange, RangeAt(bClose.value())}, templates);
            parseCoreType(ctx, i + 1, bClose.value(), tRes);
            result.push_back(tRes);
            i = bClose.value();
            break;
          } else {
            Error("Invalid end for declaring a type", RangeAt(i + 2));
          }
        }
        // FIXME - Support mix types
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
                ValueAt(i + 1), contents, getVisibility(),
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
                ValueAt(i + 1), contents, getVisibility(),
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
        auto t_ident = tokens->at(i + 1);
        if (ctx.has_alias(t_ident.value)) {
          Error("An alias with the name " + t_ident.value + " already exists",
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
      Vec<ast::TemplatedType *> templates;
      auto                      ctx = ParserContext();
      if (isNext(TokenType::templateTypeStart, i)) {
        auto endRes = getPairEnd(TokenType::templateTypeStart,
                                 TokenType::templateTypeEnd, i + 1, false);
        if (endRes.has_value()) {
          auto end  = endRes.value();
          templates = parseTemplateTypes(i + 1, end);
          for (auto *temp : templates) {
            ctx.addTemplate(temp);
          }
          if (isNext(TokenType::givenTypeSeparator, end) ||
              isNext(TokenType::parenthesisOpen, end)) {
            i = end;
          } else {
            // FIXME - Template types in global declaration
            if (isNext(TokenType::colon, end)) {
              if (isNext(TokenType::identifier, end + 1)) {
              }
            }
          }
        } else {
          Error("Expected end for template specification", RangeAt(i + 1));
        }
      }
      if (isNext(TokenType::parenthesisOpen, i) ||
          isNext(TokenType::givenTypeSeparator, i)) {
        ast::QatType *retType = nullptr;
        if (isNext(TokenType::givenTypeSeparator, i)) {
          auto typRes = parseType(ctx, i + 1, None);
          retType     = typRes.first;
          i           = typRes.second;
        } else {
          retType = new ast::VoidType(false, RangeAt(start));
        }
        if (!isNext(TokenType::parenthesisOpen, i)) {
          Error("Expected ( for arguments in function declaration",
                RangeSpan(start, i));
        }
        SHOW("Function with void return type")
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i + 1, false);
        if (pCloseRes.has_value()) {
          auto pClose    = pCloseRes.value();
          auto argResult = parseFunctionParameters(ctx, i + 1, pClose);
          SHOW("Parsed arguments")
          i = pClose;
          String callConv;
          if (isNext(TokenType::external, i)) {
            SHOW("Function is extern")
            if (isNext(TokenType::StringLiteral, i + 1)) {
              SHOW("Calling convention found")
              callConv = ValueAt(i + 2);
              i += 2;
            } else {
              SHOW("No calling convention")
              Error("Expected calling convention string after extern",
                    RangeAt(i + 1));
            }
          }
          SHOW("Creating prototype")
          auto *prototype = new ast::FunctionPrototype(
              ValueAt(start), argResult.first, argResult.second, retType, false,
              llvm::GlobalValue::WeakAnyLinkage, callConv, getVisibility(),
              RangeSpan(start, pClose), templates);
          SHOW("Prototype created")
          if (isNext(TokenType::bracketOpen, i)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                        TokenType::bracketClose, i + 1, false);
            if (bCloseRes.has_value()) {
              auto bClose    = bCloseRes.value();
              auto sentences = parseSentences(ctx, i + 1, bClose);
              SHOW("Function definition created")
              result.push_back(new ast::FunctionDefinition(
                  prototype, sentences, RangeSpan(i + 1, bClose)));
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
            callConv = ValueAt(pClose + 2);
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
                (bCloseResult.value() >= tokens->size())) {
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
      auto val = ValueAt(from + 2);
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
void Parser::parseCoreType(ParserContext &prev_ctx, usize from, usize upto,
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
    Token &token = tokens->at(i);
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
      if (!isNext(TokenType::identifier, i) && !isNext(TokenType::Async, i) &&
          !isNext(TokenType::Operator, i)) {
        Error("Expected `operator`, `async` or the identifier for the name of "
              "the member function after <~",
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
      SHOW("Identifier inside core type: " << token.value)
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
                              ValueAt(start), argsRes.first, argsRes.second,
                              retTy, getAsync(), getVisibility(),
                              RangeSpan(start, pClose + 1))
                        : ast::MemberPrototype::Normal(
                              getVariation(), ValueAt(start), argsRes.first,
                              argsRes.second, retTy, getAsync(),
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
      } else {
        auto typeRes = parseType(prev_ctx, i - 1, None);
        cacheTy      = typeRes.first;
        SHOW("Type parsed inside core type is " << typeRes.first->toString())
        SHOW("Type parsed inside core type kind is "
             << (int)typeRes.first->typeKind())
        i = typeRes.second;
      }
      break;
    }
    case TokenType::Default: {
      if (coreTy->hasDefaultConstructor()) {
        Error("A default destructor is already defined for the core type",
              RangeAt(i));
      }
      if (isNext(TokenType::bracketOpen, i)) {
        auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                    TokenType::bracketClose, i + 1, false);
        if (bCloseRes.has_value()) {
          auto bClose = bCloseRes.value();
          auto snts   = parseSentences(prev_ctx, i + 1, bClose);
          coreTy->setDefaultConstructor(new ast::ConstructorDefinition(
              ast::ConstructorPrototype::Default(getVisibility(), RangeAt(i)),
              std::move(snts), RangeSpan(i, bClose)));
          i = bClose;
        } else {
          Error("Expected end for [", RangeAt(i + 1));
        }
      } else {
        Error("Expected [ to start the definition of the default constructor",
              RangeAt(i));
      }
      break;
    }
    case TokenType::copy: {
      if (coreTy->hasCopyConstructor()) {
        Error("A copy constructor is already defined for the core type",
              RangeAt(i));
      }
      if (isNext(TokenType::identifier, i)) {
        auto argName = ValueAt(i + 1);
        if (isNext(TokenType::bracketOpen, i + 1)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                      TokenType::bracketClose, i + 2, false);
          if (bCloseRes.has_value()) {
            auto bClose = bCloseRes.value();
            auto snts   = parseSentences(prev_ctx, i + 2, bClose);
            coreTy->setCopyConstructor(new ast::ConstructorDefinition(
                ast::ConstructorPrototype::Copy(RangeSpan(i, i + 1), argName),
                std::move(snts), RangeSpan(i, bClose)));
            i = bClose;
          } else {
            Error("Expected end for [", RangeAt(i + 2));
          }
        } else {
          Error("Expected [ to start the definition of the copy constructor",
                RangeSpan(i, i + 1));
        }
      } else {
        Error("Expected name for the argument, which represents the existing "
              "instance of the type, after copy keyword",
              RangeAt(i));
      }
      break;
    }
    case TokenType::move: {
      if (coreTy->hasMoveConstructor()) {
        Error("A move constructor is already defined for the core type",
              RangeAt(i));
      }
      if (isNext(TokenType::identifier, i)) {
        auto argName = ValueAt(i + 1);
        if (isNext(TokenType::bracketOpen, i + 1)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                      TokenType::bracketClose, i + 2, false);
          if (bCloseRes.has_value()) {
            auto bClose = bCloseRes.value();
            auto snts   = parseSentences(prev_ctx, i + 2, bClose);
            coreTy->setMoveConstructor(new ast::ConstructorDefinition(
                ast::ConstructorPrototype::Move(RangeSpan(i, i + 1), argName),
                std::move(snts), RangeSpan(i, bClose)));
            i = bClose;
          } else {
            Error("Expected end for [", RangeAt(i + 2));
          }
        } else {
          Error("Expected [ to start the definition of the move constructor",
                RangeSpan(i, i + 1));
        }
      } else {
        Error("Expected name for the argument, which represents the existing "
              "instance of the type, after move keyword",
              RangeAt(i));
      }
      break;
    }
    case TokenType::from: {
      auto start = i;
      if (isNext(TokenType::parenthesisOpen, i)) {
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
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
              if (argsRes.first.size() == 1) {
                // NOTE - Convertor
                coreTy->addConvertorDefinition(new ast::ConvertorDefinition(
                    ast::ConvertorPrototype::From(
                        argsRes.first.front()->getName(),
                        argsRes.first.front()->getType(),
                        argsRes.first.front()->isTypeMember(), getVisibility(),
                        RangeSpan(start, pClose)),
                    snts, RangeSpan(start, bClose)));
              } else {
                // NOTE = Constructor
                coreTy->addConstructorDefinition(new ast::ConstructorDefinition(
                    ast::ConstructorPrototype::Normal(std::move(argsRes.first),
                                                      getVisibility(),
                                                      RangeSpan(start, pClose)),
                    snts, RangeSpan(start, bClose)));
              }
              i = bClose;
            } else {
              Error("Expected end for [", RangeAt(pClose + 1));
            }
          } else {
            Error("Expected definition for constructor",
                  RangeSpan(start, pClose));
          }
        } else {
          Error("Expected end for (", RangeAt(i + 1));
        }
      }
      break;
    }
    case TokenType::Operator: {
      if (isNext(TokenType::copy, i)) {
        if (coreTy->hasCopyAssignment()) {
          Error("The copy assignment operator is already defined for this core "
                "type",
                RangeSpan(i, i + 1));
        }
        if (isNext(TokenType::assignment, i + 1)) {
          if (isNext(TokenType::identifier, i + 2)) {
            if (isNext(TokenType::bracketOpen, i + 3)) {
              auto bCloseRes =
                  getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                             i + 4, false);
              if (bCloseRes) {
                auto bClose = bCloseRes.value();
                auto snts   = parseSentences(prev_ctx, i + 4, bClose);
                coreTy->setCopyAssignment(new ast::OperatorDefinition(
                    new ast::OperatorPrototype(
                        true, ast::Op::copyAssignment, {}, nullptr,
                        utils::VisibilityKind::pub, RangeSpan(i, i + 3),
                        ValueAt(i + 3)),
                    std::move(snts), RangeSpan(i, bClose)));
                i = bClose;
              } else {
                Error("Expected end for [", RangeAt(i + 4));
              }
            } else {
              Error("Expected definition for the copy assignment operator",
                    RangeSpan(i, i + 3));
            }
          } else {
            Error("Expected identifier for the name of the value that is being "
                  "copied from",
                  RangeSpan(i, i + 2));
          }
        } else {
          Error("Expected = after the copy keyword to indicate the copy "
                "assignment operator",
                RangeSpan(i, i + 1));
        }
        break;
      } else if (isNext(TokenType::move, i)) {
        if (coreTy->hasMoveAssignment()) {
          Error("The move assignment operator is already defined for this core "
                "type",
                RangeSpan(i, i + 1));
        }
        if (isNext(TokenType::assignment, i + 1)) {
          if (isNext(TokenType::identifier, i + 2)) {
            if (isNext(TokenType::bracketOpen, i + 3)) {
              auto bCloseRes =
                  getPairEnd(TokenType::bracketOpen, TokenType::bracketClose,
                             i + 4, false);
              if (bCloseRes) {
                auto bClose = bCloseRes.value();
                auto snts   = parseSentences(prev_ctx, i + 4, bClose);
                coreTy->setMoveAssignment(new ast::OperatorDefinition(
                    new ast::OperatorPrototype(
                        true, ast::Op::moveAssignment, {}, nullptr,
                        utils::VisibilityKind::pub, RangeSpan(i, i + 3),
                        ValueAt(i + 3)),
                    std::move(snts), RangeSpan(i, bClose)));
                i = bClose;
              } else {
                Error("Expected end for [", RangeAt(i + 4));
              }
            } else {
              Error("Expected definition for the move assignment operator",
                    RangeSpan(i, i + 3));
            }
          } else {
            Error("Expected identifier for the name of the value that is being "
                  "moved from",
                  RangeSpan(i, i + 2));
          }
        } else {
          Error("Expected = after the move keyword to indicate the move "
                "assignment operator",
                RangeSpan(i, i + 1));
        }
        break;
      }
      auto          start = i;
      String        opr;
      bool          isUnary = false;
      ast::QatType *returnTy =
          new ast::VoidType(false, utils::FileRange{"", {0u, 0u}, {0u, 0u}});
      Vec<ast::Argument *> args;
      if (isNext(TokenType::binaryOperator, i)) {
        SHOW("Binary operator for core type: " << ValueAt(i + 1))
        if (ValueAt(i + 1) == "-") {
          if (isNext(TokenType::parenthesisOpen, i) &&
              isNext(TokenType::parenthesisClose, i + 1)) {
            isUnary = true;
          } else if (isNext(TokenType::givenTypeSeparator, i)) {
            auto typRes = parseType(prev_ctx, i + 1, None);
            returnTy    = typRes.first;
            i           = typRes.second;
            if (isNext(TokenType::parenthesisOpen, i) &&
                isNext(TokenType::parenthesisClose, i + 1)) {
              isUnary = true;
            }
          }
        }
        opr = ValueAt(i + 1);
        i++;
      } else if (isNext(TokenType::unaryOperator, i)) {
        isUnary = true;
        opr     = ValueAt(i + 1);
        i++;
      } else if (isNext(TokenType::assignment, i)) {
        opr = "=";
        i++;
      } else if (isNext(TokenType::bracketOpen, i)) {
        if (isNext(TokenType::bracketClose, i + 1)) {
          opr = "[]";
          i += 2;
        } else {
          Error("[ is an invalid operator. Did you mean []",
                RangeSpan(start, start + 1));
        }
      } else {
        Error("Invalid operator", RangeAt(i));
      }
      if (isNext(TokenType::givenTypeSeparator, i)) {
        auto typRes = parseType(prev_ctx, i + 1, None);
        returnTy    = typRes.first;
        i           = typRes.second;
      }
      if (!isUnary) {
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes =
              getPairEnd(TokenType::parenthesisOpen,
                         TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto pClose    = pCloseRes.value();
            auto fnArgsRes = parseFunctionParameters(prev_ctx, i + 1, pClose);
            if (fnArgsRes.second) {
              Error("Operator functions cannot have variadic arguments",
                    RangeSpan(start, pClose));
            }
            args = fnArgsRes.first;
            i    = pClose;
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        }
      }
      auto *prototype = new ast::OperatorPrototype(
          getVariation(),
          isUnary ? (opr == "-" ? ast::Op::minus : ast::OpFromString(opr))
                  : ast::OpFromString(opr),
          args, returnTy, getVisibility(), RangeSpan(start, i));
      if (isNext(TokenType::bracketOpen, i)) {
        auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                    TokenType::bracketClose, i + 1, false);
        if (bCloseRes.has_value()) {
          auto bClose = bCloseRes.value();
          auto snts   = parseSentences(prev_ctx, i + 1, bClose);
          coreTy->addOperatorDefinition(new ast::OperatorDefinition(
              prototype, snts, RangeSpan(start, bClose)));
          i = bClose;
        } else {
          Error("Expected end for [", RangeAt(i + 1));
        }
      } else {
        Error("Expected definition for the operator", RangeSpan(start, i));
      }
      break;
    }
    case TokenType::to: {
      break;
    }
    case TokenType::end: {
      if (coreTy->hasDestructor()) {
        Error("A destructor is already defined for the core type", RangeAt(i));
      }
      if (isNext(TokenType::bracketOpen, i)) {
        auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                    TokenType::bracketClose, i + 1, false);
        if (bCloseRes) {
          auto snts = parseSentences(prev_ctx, i + 1, bCloseRes.value());
          coreTy->setDestructorDefinition(new ast::DestructorDefinition(
              snts, RangeSpan(i, bCloseRes.value())));
          i = bCloseRes.value();
        } else {
          Error("Expected end for [", RangeAt(i + 1));
        }
      } else {
        Error("Expected definition of destructor", RangeAt(i));
      }
      break;
    }
    default: {
    }
    }
  }
}

void Parser::parseMixType(ParserContext &prev_ctx, usize from, usize upto,
                          Vec<Pair<String, Maybe<ast::QatType *>>> &uRef,
                          Vec<utils::FileRange> &fileRanges) {
  using lexer::TokenType;

  for (auto i = from + 1; i < upto; i++) {
    auto &token = tokens->at(i);
    switch (token.type) {
    case TokenType::identifier: {
      auto start = i;
      if (isNext(TokenType::separator, i) ||
          (isNext(TokenType::curlybraceClose, i) && (i + 1 == upto))) {
        uRef.push_back(
            Pair<String, Maybe<ast::QatType *>>(ValueAt(start), None));
        fileRanges.push_back(RangeAt(start));
        i++;
      } else if (isNext(TokenType::mixSeparator, i)) {
        ast::QatType *typ;
        if (isPrimaryWithin(TokenType::separator, i + 1, upto)) {
          auto sepPos =
              firstPrimaryPosition(TokenType::separator, i + 1).value();
          typ = parseType(prev_ctx, i + 1, sepPos).first;
          i   = sepPos;
        } else {
          typ = parseType(prev_ctx, i + 1, upto).first;
          i   = upto;
        }
        uRef.push_back(
            Pair<String, Maybe<ast::QatType *>>(ValueAt(start), typ));
        fileRanges.push_back(RangeAt(start));
      } else {
        Error("Invalid token found after identifier in mix type definition",
              RangeAt(i));
      }
      break;
    }
    default: {
      Error("Invalid token found after identifier in mix type definition",
            RangeAt(i));
    }
    }
  }
}

void Parser::parseChoiceType(
    usize from, usize upto,
    Vec<Pair<ast::DefineChoiceType::Field, Maybe<ast::DefineChoiceType::Value>>>
        &fields) {
  using lexer::TokenType;

  for (usize i = from + 1; i < upto; i++) {
    auto &token = tokens->at(i);
    switch (token.type) {
    case TokenType::identifier: {
      auto start = i;
      if (isNext(TokenType::separator, i)) {
        fields.push_back(Pair<ast::DefineChoiceType::Field,
                              Maybe<ast::DefineChoiceType::Value>>(
            {ValueAt(i), RangeAt(i)}, None));
        i++;
      } else if (isNext(TokenType::assignment, i)) {
        auto fieldName  = ValueAt(i);
        bool isNegative = false;
        if (isNext(TokenType::binaryOperator, i + 1) && ValueAt(i + 2) == "-") {
          isNegative = true;
          i += 2;
        } else if (isNext(TokenType::binaryOperator, i + 1)) {
          Error("Invalid token found inside choice definition", RangeAt(i + 2));
        } else {
          i += 1;
        }
        i64              val = 0;
        utils::FileRange valRange{"", {0u, 0u}, {0u, 0u}};
        if (isNext(TokenType::integerLiteral, i) ||
            isNext(TokenType::unsignedLiteral, i)) {
          if (ValueAt(i + 1).find('_') != String::npos) {
            ValueAt(i + 1) = ValueAt(i + 1).substr(0, ValueAt(i + 1).find('_'));
          }
          val = std::stoi(ValueAt(i + 1));
          i++;
        } else {
          Error("Expected an integer or unsigned integer literal as the value "
                "for the variant of the choice type",
                RangeAt(i));
        }
        if (isNegative) {
          val = -val;
        }
        if (isNext(TokenType::separator, i) ||
            isNext(TokenType::curlybraceClose, i)) {
          fields.push_back(Pair<ast::DefineChoiceType::Field,
                                Maybe<ast::DefineChoiceType::Value>>(
              ast::DefineChoiceType::Field{ValueAt(start), RangeAt(start)},
              ast::DefineChoiceType::Value{val, valRange}));
          i++;
        } else {
          Error("Invalid token found after integer value for the choice "
                "variant",
                RangeAt(start));
        }
      }
      break;
    }
    default: {
      Error("Invalid token found inside choice type definition", RangeAt(i));
    }
    }
  }
}

void Parser::parseMatchContents(
    ParserContext &prev_ctx, usize from, usize upto,
    Vec<Pair<ast::MatchValue *, Vec<ast::Sentence *>>> &chain,
    Maybe<Vec<ast::Sentence *>> &elseCase, bool isTypeMatch) {
  using lexer::TokenType;

  for (usize i = from + 1; i < upto; i++) {
    auto &token = tokens->at(i);
    switch (token.type) {
    case TokenType::mixSeparator: {
      auto start = i;
      if (isNext(TokenType::identifier, i)) {
        Pair<String, utils::FileRange>        fieldName = {ValueAt(i + 1),
                                                           RangeAt(i + 1)};
        Maybe<Pair<String, utils::FileRange>> valueName;
        bool                                  isVar = false;
        if (isNext(TokenType::parenthesisOpen, i + 1)) {
          auto pCloseRes =
              getPairEnd(TokenType::parenthesisOpen,
                         TokenType::parenthesisClose, i + 2, false);
          if (pCloseRes) {
            if (isNext(TokenType::var, i + 2)) {
              isVar = true;
              i += 3;
            } else {
              i += 2;
            }
            if (isNext(TokenType::identifier, i)) {
              SHOW("Value name is: " << ValueAt(i + 1))
              valueName = Pair<String, utils::FileRange>(
                  ValueAt(i + 1),
                  isVar ? utils::FileRange(RangeAt(i), RangeAt(i + 1))
                        : RangeAt(i + 1));
              if (isNext(TokenType::parenthesisClose, i + 1)) {
                i = pCloseRes.value();
              } else {
                Error("Unexpected token found after identifier in match case "
                      "value name specification",
                      RangeAt(i + 1));
              }
            } else {
              Error("Expected value name for the mix subfield",
                    RangeSpan(start, i));
            }
          } else {
            Error("Expected end for (", RangeAt(i + 2));
          }
        } else {
          i++;
        }
        if (isNext(TokenType::bracketOpen, i)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                      TokenType::bracketClose, i + 1, false);
          if (bCloseRes) {
            auto snts = parseSentences(prev_ctx, i + 1, bCloseRes.value());
            chain.push_back(Pair<ast::MatchValue *, Vec<ast::Sentence *>>(
                new ast::MixMatchValue(std::move(fieldName),
                                       std::move(valueName), isVar),
                std::move(snts)));
            i = bCloseRes.value();
          } else {
            Error("Expected end of [", RangeAt(i + 1));
          }
        } else {
          Error("Expected [ to start the sentences in this match case block",
                RangeSpan(i, i + 1));
        }
      } else {
        Error("Expected name for the subfield of the mix type to match",
              RangeAt(i));
      }
      break;
    }
    case TokenType::child: {
      if (isNext(TokenType::identifier, i)) {
        auto variantName = ValueAt(i + 1);
        if (isNext(TokenType::bracketOpen, i + 1)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                      TokenType::bracketClose, i + 2, false);
          if (bCloseRes) {
            auto bClose = bCloseRes.value();
            auto snts   = parseSentences(prev_ctx, i + 2, bClose);
            chain.push_back(Pair<ast::MatchValue *, Vec<ast::Sentence *>>(
                new ast::ChoiceMatchValue(ValueAt(i + 1), RangeAt(i + 1)),
                std::move(snts)));
            i = bClose;
          } else {
            Error("Expected end for [", RangeAt(i + 2));
          }
        } else {
          Error("Expected [ to start the sentences in this match case block",
                RangeSpan(i, i + 1));
        }
      } else {
        Error("Expected name of the variant of the choice type to match",
              RangeAt(i));
      }
      break;
    }
    case TokenType::Else: {
      if (elseCase.has_value()) {
        Error("Else case for match sentence is already provided. Please check "
              "logic and make neceassary changes",
              RangeAt(i));
      }
      if (isNext(TokenType::bracketOpen, i)) {
        auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                    TokenType::bracketClose, i + 1, false);
        if (bCloseRes.has_value()) {
          auto snts = parseSentences(prev_ctx, i + 1, bCloseRes.value());
          elseCase  = std::move(snts);
          i         = bCloseRes.value();
          if (i + 1 != upto) {
            Error("Expected match sentence to end after the else case. Make "
                  "sure "
                  "that the else case is the last branch in a match sentence",
                  RangeSpan(i + 1, bCloseRes.value()));
          }
        } else {
          Error("Expected end for [", RangeAt(i + 1));
        }
      } else {
        Error("Expected sentences for the else case in match sentence",
              RangeAt(i));
      }
      break;
    }
    default: {
      if (isPrimaryWithin(TokenType::altArrow, i, upto)) {
        auto  start  = i;
        auto  expEnd = firstPrimaryPosition(TokenType::altArrow, i).value();
        auto *exp    = parseExpression(prev_ctx, None, i - 1, expEnd).first;
        i            = expEnd;
        if (isNext(TokenType::bracketOpen, i)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                      TokenType::bracketClose, i + 1, false);
          if (bCloseRes.has_value()) {
            auto snts = parseSentences(prev_ctx, i + 1, bCloseRes.value());
            chain.push_back(Pair<ast::MatchValue *, Vec<ast::Sentence *>>(
                new ast::ExpressionMatchValue(exp), std::move(snts)));
            i = bCloseRes.value();
          } else {
            Error("Expected end for [", RangeAt(i + 1));
          }
        } else {
          Error("Expected [ to start the sentences in the match case block",
                RangeSpan(start, i));
        }
        break;
      }
      SHOW("Token type: " << (int)token.type)
      Error("Unexpected token found inside match block", token.fileRange);
    }
    }
  }
}

ast::PlainInitialiser *Parser::parsePlainInitialiser(ParserContext &prev_ctx,
                                                     ast::QatType  *type,
                                                     usize from, usize upto) {
  using lexer::TokenType;
  if (isPrimaryWithin(TokenType::associatedAssignment, from, upto)) {
    Vec<Pair<String, utils::FileRange>> fields;
    Vec<ast::Expression *>              fieldValues;
    for (usize j = from + 1; j < upto; j++) {
      if (isNext(TokenType::identifier, j - 1)) {
        fields.push_back(
            Pair<String, utils::FileRange>(ValueAt(j), RangeAt(j)));
        if (isNext(TokenType::associatedAssignment, j)) {
          if (isPrimaryWithin(TokenType::separator, j + 1, upto)) {
            auto sepRes = firstPrimaryPosition(TokenType::separator, j + 1);
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
              Error("Expected ,", RangeSpan(j + 1, upto));
            }
          } else {
            if (upto == j + 2) {
              Error("No expression for the member found",
                    RangeSpan(j + 1, j + 2));
            }
            fieldValues.push_back(
                parseExpression(prev_ctx, None, j + 1, upto).first);
            j = upto;
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
    return new ast::PlainInitialiser(type, fields, fieldValues,
                                     {type->fileRange, RangeAt(upto)});
  } else {
    auto exps = parseSeparatedExpressions(prev_ctx, from, upto);
    return new ast::PlainInitialiser(type, {}, exps,
                                     {type->fileRange, RangeAt(upto)});
  }
}

Pair<ast::Expression *, usize>
Parser::parseExpression(ParserContext &prev_ctx, // NOLINT(misc-no-recursion)
                        const Maybe<CacheSymbol> &symbol, usize from,
                        Maybe<usize> upto, bool isMemberFn,
                        Vec<ast::Expression *> cachedExps) {
  using ast::Expression;
  using lexer::Token;
  using lexer::TokenType;

  Maybe<CacheSymbol> cachedSymbol = symbol;
  Vec<Expression *>  cachedExpressions(std::move(cachedExps));
  Vec<Token>         cachedBinaryOps;
  Vec<Token>         cachedUnaryOps;

  usize i = from + 1; // NOLINT(readability-identifier-length)
  for (; upto.has_value() ? (i < upto.value()) : (i < tokens->size()); i++) {
    Token &token = tokens->at(i);
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
      auto val    = token.value;
      auto fRange = token.fileRange;
      if (isNext(TokenType::StringLiteral, i)) {
        auto pos = i;
        while (isNext(TokenType::StringLiteral, pos)) {
          val += ValueAt(pos + 1);
          fRange = utils::FileRange(fRange, RangeAt(pos + 1));
          pos++;
        }
        i = pos;
      }
      cachedExpressions.push_back(new ast::StringLiteral(val, fRange));
      break;
    }
    case TokenType::sizeOf: {
      if (isNext(TokenType::parenthesisOpen, i)) {
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i + 1, false);
        if (pCloseRes.has_value()) {
          auto *type = parseType(prev_ctx, i + 1, pCloseRes.value()).first;
          cachedExpressions.push_back(
              new ast::SizeOfType(type, RangeSpan(i, pCloseRes.value())));
          i = pCloseRes.value();
        } else {
          Error("Expected end for (", RangeAt(i + 1));
        }
      } else {
        Error("Expected ( to start the expression for sizeOf. Make sure that "
              "you "
              "provide a value, so that the size of its type can be calculated",
              RangeAt(i));
      }
      break;
    }
    case TokenType::null: {
      cachedExpressions.push_back(new ast::NullPointer(token.fileRange));
      break;
    }
    case TokenType::FALSE:
    case TokenType::TRUE: {
      cachedExpressions.push_back(
          new ast::BooleanLiteral(token.type == TokenType::TRUE, RangeAt(i)));
      break;
    }
    case TokenType::super:
    case TokenType::identifier: {
      auto symbolRes = parseSymbol(prev_ctx, i);
      cachedSymbol   = symbolRes.first;
      i              = symbolRes.second;
      break;
    }
    case TokenType::Default: {
      cachedExpressions.push_back(new ast::Default(RangeAt(i)));
      break;
    }
    case TokenType::from: {
      if (cachedSymbol.has_value()) {
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes =
              getPairEnd(TokenType::parenthesisOpen,
                         TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto pClose = pCloseRes.value();
            auto exps   = parseSeparatedExpressions(prev_ctx, i + 1, pClose);
            cachedExpressions.push_back(new ast::ConstructorCall(
                new ast::NamedType(cachedSymbol->relative, cachedSymbol->name,
                                   false, cachedSymbol->fileRange),
                exps, false, {cachedSymbol->fileRange, RangeAt(pClose)}));
            cachedSymbol = None;
            i            = pClose;
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        }
      }
      break;
    }
    case TokenType::templateTypeStart: {
      if (cachedSymbol.has_value()) {
        auto endRes = getPairEnd(TokenType::templateTypeStart,
                                 TokenType::templateTypeEnd, i, false);
        if (endRes.has_value()) {
          auto                end = endRes.value();
          Vec<ast::QatType *> types;
          if (isPrimaryWithin(TokenType::separator, i, end)) {
            auto positions =
                primaryPositionsWithin(TokenType::separator, i, end);
            types.push_back(parseType(prev_ctx, i, positions.front()).first);
            for (usize j = 0; j < (positions.size() - 1); j++) {
              types.push_back(
                  parseType(prev_ctx, positions.at(j), positions.at(j + 1))
                      .first);
            }
            types.push_back(parseType(prev_ctx, positions.back(), end).first);
          } else {
            types.push_back(parseType(prev_ctx, i, end).first);
          }
          if (isNext(TokenType::curlybraceOpen, end)) {
            auto cEnd = getPairEnd(TokenType::curlybraceOpen,
                                   TokenType::curlybraceClose, end + 1, false);
            if (cEnd.has_value()) {
              cachedExpressions.push_back(parsePlainInitialiser(
                  prev_ctx,
                  new ast::TemplateNamedType(cachedSymbol->relative,
                                             cachedSymbol->name, types, false,
                                             cachedSymbol->fileRange),
                  end + 1, cEnd.value()));
              cachedSymbol = None;
              i            = cEnd.value();
            } else {
              Error("Expected end for { in plain initialisation",
                    RangeAt(end + 1));
            }
          } else if (isNext(TokenType::from, end)) {
            i = end;
            if (isNext(TokenType::parenthesisOpen, i)) {
              auto pCloseRes =
                  getPairEnd(TokenType::parenthesisOpen,
                             TokenType::parenthesisClose, i + 1, false);
              if (pCloseRes.has_value()) {
                auto pClose = pCloseRes.value();
                auto exps = parseSeparatedExpressions(prev_ctx, i + 1, pClose);
                cachedExpressions.push_back(new ast::ConstructorCall(
                    new ast::TemplateNamedType(cachedSymbol->relative,
                                               cachedSymbol->name, types, false,
                                               cachedSymbol->fileRange),
                    exps, false, {cachedSymbol->fileRange, RangeAt(pClose)}));
                cachedSymbol = None;
                i            = pClose;
              } else {
                Error("Expected end for (", RangeAt(i + 1));
              }
            }
          } else {
            cachedExpressions.push_back(new ast::TemplateEntity(
                cachedSymbol->relative, cachedSymbol->name, types,
                {cachedSymbol->fileRange, RangeAt(end)}));
            i            = end;
            cachedSymbol = None;
          }
        } else {
          Error("Expected end for template type specification", RangeAt(i));
        }
      } else {
        Error("Expected identifier or symbol before template type "
              "specification",
              RangeAt(i));
      }
      break;
    }
    case TokenType::self: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::parenthesisOpen, i + 1)) {
          // FIXME - Self member function call
        } else {
          cachedExpressions.push_back(
              new ast::SelfMember(ValueAt(i + 1), RangeSpan(i, i + 1)));
          i++;
        }
      } else {
        SHOW("Creating self in AST")
        cachedExpressions.push_back(new ast::Self(RangeAt(i)));
      }
      break;
    }
    case TokenType::heap: {
      if (isNext(TokenType::child, i)) {
        if (isNext(TokenType::identifier, i + 1)) {
          if (ValueAt(i + 2) == "get") {
            if (isNext(TokenType::templateTypeStart, i + 2)) {
              auto tEndRes =
                  getPairEnd(TokenType::templateTypeStart,
                             TokenType::templateTypeEnd, i + 3, false);
              if (tEndRes.has_value()) {
                auto  tEnd    = tEndRes.value();
                auto  typeRes = parseType(prev_ctx, i + 3, tEnd);
                auto *type    = typeRes.first;
                if (typeRes.second != tEnd - 1) {
                  Error("Invalid type for heap'get",
                        RangeSpan(i, typeRes.second));
                }
                if (isNext(TokenType::parenthesisOpen, tEnd)) {
                  auto pCloseRes =
                      getPairEnd(TokenType::parenthesisOpen,
                                 TokenType::parenthesisClose, tEnd + 1, false);
                  if (pCloseRes.has_value()) {
                    auto  pClose = pCloseRes.value();
                    auto *exp =
                        parseExpression(prev_ctx, None, tEnd + 1, pClose).first;
                    cachedExpressions.push_back(
                        new ast::HeapGet(type, exp, RangeAt(i)));
                    i = pClose;
                    break;
                  } else {
                    Error("Expected end for (", RangeAt(tEnd + 1));
                  }
                } else {
                  cachedExpressions.push_back(
                      new ast::HeapGet(type, nullptr, RangeAt(i)));
                  i = tEnd;
                  break;
                }
              } else {
                Error("Expected end for template type specification",
                      RangeAt(i + 3));
              }
            } else {
              Error("Expected template type specification for the type to "
                    "allocate the memory for",
                    RangeSpan(i, i + 2));
            }
          } else if (ValueAt(i + 2) == "put") {
            if (isNext(TokenType::parenthesisOpen, i + 2)) {
              auto pCloseRes =
                  getPairEnd(TokenType::parenthesisOpen,
                             TokenType::parenthesisClose, i + 3, false);
              if (pCloseRes.has_value()) {
                auto *exp =
                    parseExpression(prev_ctx, None, i + 3, pCloseRes.value())
                        .first;
                cachedExpressions.push_back(
                    new ast::HeapPut(exp, RangeSpan(i, pCloseRes.value())));
                i = pCloseRes.value();
                break;
              } else {
                Error("Expected end for (", RangeAt(i + 3));
              }
            } else {
              Error("Invalid expression. Expected a pointer expression to use "
                    "for heap'put",
                    RangeSpan(i, i + 2));
            }
          } else if (ValueAt(i + 2) == "grow") {
            // FIXME - Maybe provided type is not necessary
            if (isNext(TokenType::templateTypeStart, i + 2)) {
              auto tEndRes =
                  getPairEnd(TokenType::templateTypeStart,
                             TokenType::templateTypeEnd, i + 3, false);
              if (tEndRes.has_value()) {
                auto  tEnd = tEndRes.value();
                auto *type = parseType(prev_ctx, i + 3, tEnd).first;
                if (isNext(TokenType::parenthesisOpen, tEnd)) {
                  auto pCloseRes =
                      getPairEnd(TokenType::parenthesisOpen,
                                 TokenType::parenthesisClose, tEnd + 1, false);
                  if (pCloseRes.has_value()) {
                    auto pClose = pCloseRes.value();
                    if (isPrimaryWithin(TokenType::separator, tEnd + 1,
                                        pClose)) {
                      auto split =
                          firstPrimaryPosition(TokenType::separator, tEnd + 1)
                              .value();
                      auto *exp =
                          parseExpression(prev_ctx, None, tEnd + 1, split)
                              .first;
                      auto *count =
                          parseExpression(prev_ctx, None, split, pClose).first;
                      cachedExpressions.push_back(
                          new ast::HeapGrow(type, exp, count, RangeAt(i)));
                      i = pClose;
                    } else {
                      Error("Expected 2 argument values for heap'grow",
                            RangeSpan(tEnd + 1, pClose));
                    }
                    break;
                  } else {
                    Error("Expected end for (", RangeAt(tEnd + 1));
                  }
                } else {
                  Error("Expected arguments for heap'grow", RangeSpan(i, tEnd));
                }
              } else {
                Error("Expected end for the template type specification",
                      RangeAt(i + 3));
              }
            }
          } else {
            Error("Invalid identifier found after heap'", RangeAt(i + 2));
          }
        } else {
        }
      } else {
        Error("Invalid expression", RangeAt(i));
      }
      break;
    }
    case TokenType::curlybraceOpen: {
      if (cachedSymbol.has_value()) {
        auto cCloseRes = getPairEnd(TokenType::curlybraceOpen,
                                    TokenType::curlybraceClose, i, false);
        if (cCloseRes.has_value()) {
          cachedExpressions.push_back(parsePlainInitialiser(
              prev_ctx,
              new ast::NamedType(cachedSymbol->relative, cachedSymbol->name,
                                 false, cachedSymbol->fileRange),
              i, cCloseRes.value()));
          cachedSymbol = None;
          i            = cCloseRes.value();
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
          if (upto.has_value() && (p_close.value() >= upto)) {
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
                Error("No expression found to be passed to the function call. "
                      "And no function name found for "
                      "the static function call",
                      {token.fileRange, RangeAt(p_close.value())});
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
        if (upto.has_value() && (p_close_res.value() >= upto)) {
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
              (ValueAt(i + 2) == "index") ? "" : ValueAt(i + 2),
              RangeSpan(i, i + 2)));
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
    case TokenType::move: {
      if (isNext(TokenType::parenthesisOpen, i)) {
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i + 1, false);
        if (pCloseRes) {
          auto  pClose = pCloseRes.value();
          auto *exp    = parseExpression(prev_ctx, None, i + 1, pClose).first;
          cachedExpressions.push_back(new ast::Move(exp, RangeSpan(i, pClose)));
          i = pClose;
        } else {
          Error("Expected end for (", RangeAt(i + 1));
        }
      } else {
        Error("Expected ( to start the expression to move", RangeAt(i));
      }
      break;
    }
    case TokenType::own: {
      auto start = i;
      // FIXME - Add heaped plain initialisation
      auto symRes  = parseSymbol(prev_ctx, i + 1);
      cachedSymbol = symRes.first;
      i            = symRes.second;
      ast::QatType *type;
      if (isNext(TokenType::templateTypeStart, i)) {
        auto tEndRes = getPairEnd(TokenType::templateTypeStart,
                                  TokenType::templateTypeEnd, i + 1, false);
        if (tEndRes.has_value()) {
          auto tEnd  = tEndRes.value();
          auto types = parseSeparatedTypes(prev_ctx, i + 1, tEnd);
          type       = new ast::TemplateNamedType(cachedSymbol->relative,
                                                  cachedSymbol->name, types, false,
                                                  cachedSymbol->fileRange);
          i          = tEnd;
        } else {
          Error("Expected end for the template type specification",
                RangeAt(i + 1));
        }
      } else {
        type = new ast::NamedType(cachedSymbol->relative, cachedSymbol->name,
                                  false, cachedSymbol->fileRange);
      }
      if (isNext(TokenType::from, i)) {
        if (isNext(TokenType::parenthesisOpen, i + 1)) {
          auto pCloseRes =
              getPairEnd(TokenType::parenthesisOpen,
                         TokenType::parenthesisClose, i + 2, false);
          if (pCloseRes) {
            auto pClose = pCloseRes.value();
            auto args   = parseSeparatedExpressions(prev_ctx, i + 2, pClose);
            cachedExpressions.push_back(new ast::ConstructorCall(
                type, args, true, RangeSpan(start, pClose)));
            i = pClose;
          } else {
            Error("Expected end for (", RangeAt(i + 2));
          }
        }
      }
      cachedSymbol = None;
      break;
    }
    case TokenType::pointerType: {
      if (!cachedExpressions.empty() || cachedSymbol.has_value()) {
        if (cachedSymbol) {
          cachedExpressions.push_back(new ast::Entity(cachedSymbol->relative,
                                                      cachedSymbol->name,
                                                      cachedSymbol->fileRange));
          cachedSymbol = None;
        }
        auto *exp = cachedExpressions.back();
        cachedExpressions.pop_back();
        cachedExpressions.push_back(
            new ast::Dereference(exp, {exp->fileRange, RangeAt(i)}));
      } else {
        Error("No expression found before pointer dereference operator",
              RangeAt(i));
      }
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
    case TokenType::stringSliceType: {
      // FIXME - Support heaped creation
      if (isNext(TokenType::from, i)) {
        if (isNext(TokenType::parenthesisOpen, i + 1)) {
          auto pCloseRes =
              getPairEnd(TokenType::parenthesisOpen,
                         TokenType::parenthesisClose, i + 2, false);
          if (pCloseRes.has_value()) {
            auto pClose = pCloseRes.value();
            auto args   = parseSeparatedExpressions(prev_ctx, i + 2, pClose);
            cachedExpressions.push_back(new ast::ConstructorCall(
                new ast::StringSliceType(false, RangeAt(i)), args, false,
                RangeSpan(i, pClose)));
            i = pClose;
          } else {
            Error("Expected end for (", RangeAt(i + 2));
          }
        } else {
          Error("Expected ( to start the arguments for creating a string slice",
                RangeSpan(i, i + 1));
        }
      } else {
        Error("Invalid expression", RangeAt(i));
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
                  {cond->fileRange, RangeAt(pClose)}));
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
                  exp, false, ValueAt(i + 1), args, true,
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
    case TokenType::mixSeparator: {
      if (cachedSymbol.has_value()) {
        auto *typ =
            new ast::NamedType(cachedSymbol->relative, cachedSymbol->name,
                               false, cachedSymbol->fileRange);
        if (isNext(TokenType::identifier, i)) {
          auto &subName = ValueAt(i + 1);
          if (isNext(TokenType::parenthesisOpen, i + 1)) {
            auto pCloseRes =
                getPairEnd(TokenType::parenthesisOpen,
                           TokenType::parenthesisClose, i + 2, false);
            if (pCloseRes) {
              auto  pClose = pCloseRes.value();
              auto *exp = parseExpression(prev_ctx, None, i + 2, pClose).first;
              cachedExpressions.push_back(new ast::MixTypeInitialiser(
                  typ, subName, exp, RangeSpan(i, pClose)));
              i = pClose;
            } else {
              Error("Expected end for (", RangeAt(i + 2));
            }
          } else {
            cachedExpressions.push_back(new ast::MixTypeInitialiser(
                typ, subName, None, {cachedSymbol->fileRange, RangeAt(i + 1)}));
            i++;
          }
        }
      } else {
        Error("No name of the mix type found", RangeAt(i));
      }
      break;
    }
    case TokenType::child: {
      SHOW("Expression parsing : Member access")
      if ((!cachedExpressions.empty()) || cachedSymbol.has_value()) {
        if (cachedExpressions.empty() && cachedSymbol.has_value()) {
          SHOW("Expression empty, using symbol")
          cachedExpressions.push_back(new ast::Entity(cachedSymbol->relative,
                                                      cachedSymbol->name,
                                                      cachedSymbol->fileRange));
          cachedSymbol = None;
        } else if ((!cachedExpressions.empty()) && cachedSymbol.has_value()) {
          Error("Cached expressions are not empty and found symbol at the same "
                "time",
                cachedExpressions.back()->fileRange);
        }
        auto *exp = cachedExpressions.back();
        cachedExpressions.pop_back();
        if (isNext(TokenType::identifier, i)) {
          // TODO - Support template member function calls
          if (isNext(TokenType::parenthesisOpen, i + 1)) {
            auto pCloseRes =
                getPairEnd(TokenType::parenthesisOpen,
                           TokenType::parenthesisClose, i + 2, false);
            if (pCloseRes.has_value()) {
              auto pClose = pCloseRes.value();
              auto args   = parseSeparatedExpressions(prev_ctx, i + 2, pClose);
              cachedExpressions.push_back(new ast::MemberFunctionCall(
                  exp, false, ValueAt(i + 1), args, false,
                  {exp->fileRange, RangeAt(pClose)}));
              i = pClose;
              break;
            } else {
              Error("Expected end for (", RangeAt(i + 2));
            }
          } else {
            cachedExpressions.push_back(new ast::MemberAccess(
                exp, false, ValueAt(i + 1), {exp->fileRange, RangeAt(i + 1)}));
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
        } else if (cachedSymbol.has_value() && (upto.value() == i)) {
          return {new ast::Entity(cachedSymbol->relative, cachedSymbol->name,
                                  cachedSymbol->fileRange),
                  i - 1};
        } else {
          Error("Encountered an invalid token in the expression", RangeAt(i));
        }
      } else {
        if (!cachedExpressions.empty()) {
          return {cachedExpressions.back(), i - 1};
        } else if (cachedSymbol.has_value()) {
          return {new ast::Entity(cachedSymbol->relative, cachedSymbol->name,
                                  cachedSymbol->fileRange),
                  i - 1};
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

Vec<ast::QatType *> Parser::parseSeparatedTypes(ParserContext &prev_ctx,
                                                usize from, usize upto) {
  Vec<ast::QatType *> result;
  for (usize i = from + 1; i < upto; i++) {
    if (isPrimaryWithin(lexer::TokenType::separator, i - 1, upto)) {
      auto endResult = firstPrimaryPosition(lexer::TokenType::separator, i - 1);
      if (!endResult.has_value() || (endResult.value() >= upto)) {
        Error("Invalid position for separator `,`", RangeAt(i));
      }
      auto end = endResult.value();
      result.push_back(parseType(prev_ctx, i - 1, end).first);
      i = end;
    } else {
      result.push_back(parseType(prev_ctx, i - 1, upto).first);
      i = upto;
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
    auto prev     = tokens->at(start).type;
    auto i        = start; // NOLINT(readability-identifier-length)
    if (isNext(TokenType::super, start - 1)) {
      while (prev == TokenType::super
                 ? isNext(TokenType::colon, i)
                 : (prev == TokenType::colon ? isNext(TokenType::super, i)
                                             : false)) {
        if (isNext(TokenType::super, i)) {
          relative++;
        }
        prev = tokens->at(i + 1).type;
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
    for (; i < tokens->size(); i++) {
      auto tok = tokens->at(i);
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
          prev = TokenType::colon;
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
    return {CacheSymbol(relative, name, start, RangeSpan(start, i - 1)), i - 1};
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

  auto                 ctx = prev_ctx;
  Vec<ast::Sentence *> result;

  Maybe<CacheSymbol> cacheSymbol = None;
  // NOTE - Potentially remove this variable
  Vec<ast::QatType *>    cacheTy;
  Vec<ast::Expression *> cachedExpressions;

  String context;
  String name_val;

  auto var = false;

  for (usize i = from + 1; i < upto; i++) {
    Token &token = tokens->at(i);
    switch (token.type) {
    case TokenType::voidType:
    case TokenType::integerType:
    case TokenType::floatType:
    case TokenType::curlybraceOpen:
    case TokenType::unsignedIntegerType:
    case TokenType::referenceType:
    case TokenType::var:
    case TokenType::pointerType: {
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
                typeRes.first, false, ValueAt(i + 1), exp,
                typeRes.first->isVariable(), RangeSpan(old, endRes.value())));
            i = endRes.value();
            cacheTy.clear();
            break;
          } else {
            Error("Invalid end for sentence", RangeSpan(old, i + 2));
          }
        } else if (isNext(TokenType::from, i + 1)) {
          // TODO - Implement constructor calls
        } else if (isNext(TokenType::stop, i + 1)) {
          Error("No value for initialisation in local declaration",
                {typeRes.first->fileRange, RangeAt(i + 2)});
          // FIXME - Support uninitialised declaration
          // result.push_back(new ast::LocalDeclaration(
          //     typeRes.first, false, ValueAt(i + 1), nullptr,
          //     typeRes.first->isVariable(), RangeSpan(old, i + 1)));
          // cacheTy.clear();
          // i += 2;
          // break;
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
                exp, {token.fileRange, RangeAt(endRes.value())}));
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
      if (cacheSymbol.has_value() || !cachedExpressions.empty()) {
        auto expRes = parseExpression(ctx, cacheSymbol, i, None, isMemberFn,
                                      cachedExpressions);
        cachedExpressions.clear();
        cachedExpressions.push_back(expRes.first);
        cacheSymbol = None;
        i           = expRes.second;
      } else {
        Error("No expression or entity found to access the child of",
              token.fileRange);
      }
      break;
    }
    case TokenType::super:
    case TokenType::identifier: {
      context = "IDENTIFIER";
      SHOW("Identifier encountered")
      if (cacheSymbol.has_value() || !cacheTy.empty()) {
        if (token.type == TokenType::super) {
          Error("Invalid token found in declaration", RangeAt(i));
        }
        SHOW("Symbol or type found")
        if (cacheSymbol) {
          SHOW("Symbol: " << cacheSymbol->name)
        } else if (!cacheTy.empty()) {
          SHOW("Type: " << cacheTy.back()->toString())
        }
        if (token.type == TokenType::super) {
          Error("Invalid token after symbol", RangeAt(i));
        }
        // Declaration
        // A normal declaration where the type of the entity is provided by
        // the user
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
          // FIXME - Handle constructor call
        } else if (isNext(TokenType::stop, i)) {
          Error("No value for initialisation in local declaration",
                {(cacheSymbol.has_value() ? cacheSymbol.value().fileRange
                                          : cacheTy.back()->fileRange),
                 RangeAt(i + 1)});
          // FIXME - Support uninitialised declarations?
          // result.push_back(new ast::LocalDeclaration(
          //     (cacheTy.empty()
          //          ? (new ast::NamedType(cacheSymbol.value().relative,
          //                                cacheSymbol.value().name, false,
          //                                cacheSymbol.value().fileRange))
          //          : cacheTy.back()),
          //     false, token.value, nullptr, var, token.fileRange));
          // var = false;
          // cacheTy.clear();
          // cacheSymbol = None;
          // i++;
        }
        // FIXME - Handle this case
      } else {
        auto sym_res = parseSymbol(ctx, i);
        cacheSymbol  = sym_res.first;
        i            = sym_res.second;
        if (isNext(TokenType::templateTypeStart, i)) {
          auto tEndRes = getPairEnd(TokenType::templateTypeStart,
                                    TokenType::templateTypeEnd, i + 1, false);
          if (tEndRes.has_value()) {
            auto                tEnd = tEndRes.value();
            Vec<ast::QatType *> types =
                parseSeparatedTypes(prev_ctx, i + 1, tEnd);
            if (isNext(TokenType::identifier, tEnd)) {
              cacheTy.push_back(new ast::TemplateNamedType(
                  cacheSymbol->relative, cacheSymbol->name, types, false,
                  {cacheSymbol->fileRange, RangeAt(tEnd)}));
              i = tEnd;
              break;
            } else {
              auto expRes = parseExpression(prev_ctx, cacheSymbol, i, None);
              cachedExpressions.push_back(expRes.first);
              cacheSymbol = None;
              i           = expRes.second;
              break;
            }
          } else {
            Error("Expected end for template type specification",
                  RangeAt(i + 1));
          }
        } else {
          if (isNext(TokenType::identifier, i)) {
            cacheTy.push_back(new ast::NamedType(cacheSymbol->relative,
                                                 cacheSymbol->name, false,
                                                 cacheSymbol->fileRange));
          } else {
            auto expRes = parseExpression(prev_ctx, cacheSymbol, i, None);
            cachedExpressions.push_back(expRes.first);
            cacheSymbol = None;
            i           = expRes.second;
          }
        }
      }
      break;
    }
    case TokenType::alias: {
      if (isNext(TokenType::identifier, i)) {
        if (isNext(TokenType::altArrow, i + 1)) {
          auto stop = firstPrimaryPosition(TokenType::stop, i + 2);
          if (stop.has_value()) {
            auto *exp =
                parseExpression(prev_ctx, None, i + 2, stop.value()).first;
            result.push_back(new ast::NewAlias(ValueAt(i + 1), exp,
                                               RangeSpan(i, stop.value())));
            i = stop.value();
          } else {
            Error("Expected end for alias declaration", RangeSpan(i, i + 2));
          }
        } else {
          Error("Expected => after name for the alias", RangeSpan(i, i + 1));
        }
      } else {
        Error("Expected identifier for alias declaration", RangeAt(i));
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
    case TokenType::match: {
      auto start       = i;
      bool isTypeMatch = false;
      if (isNext(TokenType::Type, i)) {
        isTypeMatch = true;
        i++;
      }
      if (isNext(TokenType::parenthesisOpen, i)) {
        auto pCloseRes = getPairEnd(TokenType::parenthesisOpen,
                                    TokenType::parenthesisClose, i + 1, false);
        if (pCloseRes) {
          auto *cand =
              parseExpression(prev_ctx, None, i + 1, pCloseRes.value()).first;
          i = pCloseRes.value();
          if (isNext(TokenType::bracketOpen, i)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen,
                                        TokenType::bracketClose, i + 1, false);
            if (bCloseRes) {
              Vec<Pair<ast::MatchValue *, Vec<ast::Sentence *>>> chain;
              Maybe<Vec<ast::Sentence *>>                        elseCase;
              parseMatchContents(prev_ctx, i + 1, bCloseRes.value(), chain,
                                 elseCase, isTypeMatch);
              result.push_back(new ast::Match(
                  isTypeMatch, cand, std::move(chain), std::move(elseCase),
                  RangeSpan(start, bCloseRes.value())));
              i = bCloseRes.value();
            } else {
              Error("Expected end for [", RangeAt(i + 1));
            }
          } else {
            Error("Expected sentences for this case of the match sentence",
                  RangeSpan(start, i));
          }
        } else {
          Error("Expected end for (", RangeAt(i + 1));
        }
      } else {
        Error("Expected expression to match", RangeAt(start));
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
            result.push_back(
                new ast::LocalDeclaration(nullptr, isRef, ValueAt(i + 1), exp,
                                          isVarDecl, RangeSpan(start, end)));
            cacheSymbol = None;
            cacheTy.clear();
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
          result.push_back(
              new ast::LocalDeclaration(nullptr, isRef, ValueAt(i + 1), nullptr,
                                        isVarDecl, RangeSpan(start, i + 2)));
          cacheSymbol = None;
          cacheTy.clear();
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
            tag = ValueAt(i + 2);
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
          bool          isAlias = false;
          auto *count = parseExpression(prev_ctx, None, i + 1, pClose).first;
          auto  pos   = pClose;
          if (isNext(TokenType::colon, pClose)) {
            if (isNext(TokenType::let, pClose + 1)) {
              if (isNext(TokenType::identifier, pClose + 2)) {
                isAlias = true;
                loopTag = ValueAt(pClose + 3);
                pos     = pClose + 3;
              } else {
                Error("Expected an identifier for the tag for the loop after "
                      "alias",
                      RangeAt(pClose + 2));
              }
            } else if (isNext(TokenType::identifier, pClose + 1)) {
              loopTag = ValueAt(pClose + 2);
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
                  loopTag, isAlias, RangeSpan(i, bCloseRes.value())));
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
                    result.push_back(
                        new ast::LoopWhile(cond, snts, ValueAt(pClose + 2),
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
          result.push_back(new ast::Break(ValueAt(i + 2), RangeSpan(i, i + 2)));
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
              new ast::Continue(ValueAt(i + 2), RangeSpan(i, i + 2)));
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
      auto expRes = parseExpression(prev_ctx, cacheSymbol, i - 1, None,
                                    isMemberFn, cachedExpressions);
      cachedExpressions.clear();
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

  for (usize i = from + 1; ((i < upto) && (i < tokens->size())); i++) {
    auto &token = tokens->at(i);
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
        args.push_back(ast::Argument::Normal(ValueAt(i + 1),
                                             RangeSpan(i, i + 1), nullptr));
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
        SHOW("Creating member argument: " << ValueAt(i + 1))
        args.push_back(ast::Argument::ForConstructor(
            ValueAt(i + 1), RangeSpan(i, i + 1), nullptr, true));
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
  for (usize i = current + 1; i < tokens->size(); i++) {
    if (respectScope) {
      for (auto scopeTk : TokenFamily::scopeLimiters) {
        if (tokens->at(i).type == scopeTk) {
          return None;
        }
      }
    }
    if (tokens->at(i).type == startType) {
      collisions++;
    } else if (tokens->at(i).type == endType) {
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
    return tokens->at(from - 1).type == type;
  } else {
    return false;
  }
}

bool Parser::isNext(const lexer::TokenType type, const usize current) {
  if ((current + 1) < tokens->size()) {
    return tokens->at(current + 1).type == type;
  } else {
    return false;
  }
}

bool Parser::areOnlyPresentWithin(const Vec<lexer::TokenType> &kinds,
                                  usize from, usize upto) {
  for (usize i = from + 1; i < upto; i++) {
    for (auto const &kind : kinds) {
      if (kind != tokens->at(i).type) {
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
  for (usize i = from + 1; i < tokens->size(); i++) {
    auto tok = tokens->at(i);
    if (tok.type == TokenType::parenthesisOpen) {
      auto end_res = getPairEnd(TokenType::parenthesisOpen,
                                TokenType::parenthesisClose, i, false);
      if (end_res.has_value() && (end_res.value() < tokens->size())) {
        i = end_res.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::bracketOpen) {
      auto end_res =
          getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
      if (end_res.has_value() && (end_res.value() < tokens->size())) {
        i = end_res.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::curlybraceOpen) {
      auto end_res = getPairEnd(TokenType::curlybraceOpen,
                                TokenType::curlybraceClose, i, false);
      if (end_res.has_value() && (end_res.value() < tokens->size())) {
        i = end_res.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::templateTypeStart) {
      auto end_res = getPairEnd(TokenType::templateTypeStart,
                                TokenType::templateTypeEnd, i, false);
      if (end_res.has_value() && (end_res.value() < tokens->size())) {
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
  for (usize i = from + 1; (i < upto && i < tokens->size()); i++) {
    auto tok = tokens->at(i);
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
  tokens->clear();
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
