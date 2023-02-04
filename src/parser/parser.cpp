#include "./parser.hpp"
#include "../ast/constants/boolean_literal.hpp"
#include "../ast/constants/custom_float_literal.hpp"
#include "../ast/constants/custom_integer_literal.hpp"
#include "../ast/constants/default.hpp"
#include "../ast/constants/entity.hpp"
#include "../ast/constants/float_literal.hpp"
#include "../ast/constants/integer_literal.hpp"
#include "../ast/constants/null_pointer.hpp"
#include "../ast/constants/size_of_type.hpp"
#include "../ast/constants/type_checker.hpp"
#include "../ast/constants/unsigned_literal.hpp"
#include "../ast/constructor.hpp"
#include "../ast/define_mix_type.hpp"
#include "../ast/define_region.hpp"
#include "../ast/destructor.hpp"
#include "../ast/expressions/array_literal.hpp"
#include "../ast/expressions/await.hpp"
#include "../ast/expressions/binary_expression.hpp"
#include "../ast/expressions/constructor_call.hpp"
#include "../ast/expressions/copy.hpp"
#include "../ast/expressions/default.hpp"
#include "../ast/expressions/dereference.hpp"
#include "../ast/expressions/entity.hpp"
#include "../ast/expressions/function_call.hpp"
#include "../ast/expressions/generic_entity.hpp"
#include "../ast/expressions/heap.hpp"
#include "../ast/expressions/index_access.hpp"
#include "../ast/expressions/loop_index.hpp"
#include "../ast/expressions/member_access.hpp"
#include "../ast/expressions/member_function_call.hpp"
#include "../ast/expressions/mix_type_initialiser.hpp"
#include "../ast/expressions/move.hpp"
#include "../ast/expressions/none.hpp"
#include "../ast/expressions/not.hpp"
#include "../ast/expressions/plain_initialiser.hpp"
#include "../ast/expressions/self.hpp"
#include "../ast/expressions/self_member.hpp"
#include "../ast/expressions/ternary.hpp"
#include "../ast/expressions/to_conversion.hpp"
#include "../ast/expressions/tuple_value.hpp"
#include "../ast/function.hpp"
#include "../ast/global_declaration.hpp"
#include "../ast/lib.hpp"
#include "../ast/member_function.hpp"
#include "../ast/mod_info.hpp"
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
#include "../ast/sentences/say_sentence.hpp"
#include "../ast/type_definition.hpp"
#include "../ast/types/array.hpp"
#include "../ast/types/const_generic.hpp"
#include "../ast/types/cstring.hpp"
#include "../ast/types/float.hpp"
#include "../ast/types/future.hpp"
#include "../ast/types/generic_named_type.hpp"
#include "../ast/types/integer.hpp"
#include "../ast/types/linked_generic.hpp"
#include "../ast/types/maybe.hpp"
#include "../ast/types/named.hpp"
#include "../ast/types/pointer.hpp"
#include "../ast/types/reference.hpp"
#include "../ast/types/string_slice.hpp"
#include "../ast/types/tuple.hpp"
#include "../ast/types/typed_generic.hpp"
#include "../ast/types/unsigned.hpp"
#include "../show.hpp"
#include "cache_symbol.hpp"
#include "parser_context.hpp"
#include <chrono>
#include <string>
#include <utility>

#define IdentifierAt(ind) Identifier(tokens->at(ind).value, tokens->at(ind).fileRange)
#define ValueAt(ind)      tokens->at(ind).value
#define RangeAt(ind)      tokens->at(ind).fileRange
#define RangeSpan(ind1, ind2)                                                                                          \
  { tokens->at(ind1).fileRange, tokens->at(ind2).fileRange }

// NOTE - Check if file fileRange values are making use of the new merge
// functionality
namespace qat::parser {

Parser::Parser() = default;

Parser::~Parser() {
  delete tokens;
  tokens = nullptr;
  broughtPaths.clear();
  memberPaths.clear();
  comments.clear();
}

u64 Parser::timeInMicroSeconds = 0;

void Parser::setTokens(Deque<lexer::Token>* allTokens) {
  g_ctx = ParserContext();
  delete tokens;
  tokens = allTokens;
}

void Parser::filterComments() {}

ast::BringEntities* Parser::parseBroughtEntities(ParserContext& ctx, usize from, usize upto) {
  using lexer::TokenType;
  Vec<Vec<Identifier>> result;
  Maybe<CacheSymbol>   csym = None;

  for (usize i = from + 1; i < upto; i++) {
    auto token = tokens->at(i);
    switch (token.type) { // NOLINT(clang-diagnostic-switch)
      case TokenType::identifier: {
        auto sym_res = parseSymbol(ctx, i);
        csym         = sym_res.first;
        i            = sym_res.second;
        if (isNext(TokenType::curlybraceOpen, i)) {
          auto bCloseRes = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i, false);
          if (bCloseRes.has_value()) {
            auto        bClose = bCloseRes.value();
            Vec<String> entities;
            if (isPrimaryWithin(TokenType::separator, i, bClose)) {
              // TODO - Implement
            } else {
              Warning("Expected multiple entities to be brought. Remove the "
                      "curly braces since only one entity is being brought.",
                      FileRange(RangeAt(i + 1), RangeAt(bClose)));
              if (isNext(TokenType::identifier, i + 1)) {
                entities.push_back(ValueAt(i + 2));
              } else {
                Error("Expected the name of an entity in the bring sentence", RangeAt(i + 2));
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

Vec<fs::path>& Parser::getBroughtPaths() { return broughtPaths; }

void Parser::clearBroughtPaths() { broughtPaths.clear(); }

Vec<fs::path>& Parser::getMemberPaths() { return memberPaths; }

void Parser::clearMemberPaths() { memberPaths.clear(); }

ast::ModInfo* Parser::parseModuleInfo(usize from, usize upto, const FileRange& startRange) {
  Maybe<ast::StringLiteral*>   outputName;
  Maybe<ast::KeyValue<String>> foreignID;
  Vec<ast::StringLiteral*>     nativeLibs;
  using lexer::TokenType;
  for (usize i = from + 1; i < upto; i++) {
    auto token = tokens->at(i);
    switch (token.type) {
      case TokenType::identifier: {
        if (!isNext(TokenType::associatedAssignment, i)) {
          Error("Expected := after the key", RangeAt(i));
        }
        if (token.value == "foreign") {
          if (isNext(TokenType::StringLiteral, i + 1)) {
            if (foreignID) {
              Error("Foreign module information is already provided", RangeSpan(i, i + 2));
            }
            foreignID = ast::KeyValue<String>({"foreign", RangeAt(i)}, ValueAt(i + 2), RangeAt(i + 2));
            i += 2;
          } else {
            Error("Expected a string for the parent identity of the foreign module", RangeAt(i));
          }
        } else if (token.value == "outputName") {
          if (isNext(TokenType::StringLiteral, i + 1)) {
            outputName = new ast::StringLiteral(ValueAt(i + 2), RangeAt(i + 2));
            i += 2;
          } else {
            Error("Expected name for the output file", RangeSpan(i, i + 1));
          }
        } else {
          Error("Unexpected key found: " + token.value, RangeAt(i));
        }
        break;
      }
      case TokenType::separator: {
        if (isPrev(TokenType::separator, i)) {
          Error("Repeating commas are present", RangeSpan(i - 1, i));
        }
        break;
      }
      default: {
        Error("Unexpected token found inside meta'moduleInfo", RangeAt(i));
      }
    }
  }
  return new ast::ModInfo(outputName, std::move(foreignID), std::move(nativeLibs), {startRange, RangeAt(upto)});
}

ast::BringPaths* Parser::parseBroughtPaths(bool isMember, usize from, usize upto, utils::VisibilityKind kind,
                                           const FileRange& startRange) {
  using lexer::TokenType;
  Vec<ast::StringLiteral*>        paths;
  Vec<Maybe<ast::StringLiteral*>> names;
  for (usize i = from + 1; (i < upto) && (i < tokens->size()); i++) {
    const auto& token = tokens->at(i);
    switch (token.type) {
      case TokenType::StringLiteral: {
        if (isNext(TokenType::StringLiteral, i)) {
          Error("Implicit concatenation of adjacent string literals are not "
                "supported in bring sentences, to avoid ambiguity and confusion. "
                "If this is supposed to be another file or folder to bring, then "
                "add a separator between these. Or else fix the sentence.",
                RangeAt(i + 1));
        } else if (isNext(TokenType::as, i)) {
          if (isNext(TokenType::identifier, i + 1)) {
            names.push_back(new ast::StringLiteral(ValueAt(i + 2), RangeAt(i + 2)));
            i += 2;
          } else {
            Error("Expected alias for the file module", RangeSpan(i, i + 1));
          }
        } else {
          names.push_back(None);
          i++;
        }
        SHOW("Pushing brought path: " << token.fileRange.file.parent_path() / token.value)
        broughtPaths.push_back(token.fileRange.file.parent_path() / token.value);
        if (isMember) {
          memberPaths.push_back(token.fileRange.file.parent_path() / token.value);
        }
        paths.push_back(new ast::StringLiteral(token.value, token.fileRange));
        break;
      }
      case TokenType::separator: {
        if (isNext(TokenType::StringLiteral, i) || isNext(TokenType::stop, i)) {
          continue;
        } else {
          if (isNext(TokenType::separator, i)) {
            // NOTE - This might be too much, or not
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
  return new ast::BringPaths(isMember, std::move(paths), std::move(names), kind, {startRange, RangeAt(upto)});
}

Pair<ast::ConstantExpression*, usize> Parser::parseConstantExpression(ParserContext& preCtx, usize from,
                                                                      Maybe<usize> upto) {
  using lexer::TokenType;

  Maybe<ast::ConstantExpression*> cacheExp;

  auto i = 0;
  for (i = from + 1; i < (upto.has_value() ? upto.value() : tokens->size()); i++) {
    auto& token = tokens->at(i);
    switch (token.type) {
      case TokenType::integerLiteral: {
        cacheExp = new ast::IntegerLiteral(token.value, token.fileRange);
        break;
      }
      case TokenType::unsignedLiteral: {
        cacheExp = new ast::UnsignedLiteral(token.value, token.fileRange);
        break;
      }
      case TokenType::StringLiteral: {
        auto start = i;
        auto value = token.value;
        auto range = token.fileRange;
        if (isNext(TokenType::StringLiteral, i)) {
          while (isNext(TokenType::StringLiteral, i)) {
            value += tokens->at(i + 1).value;
            range = FileRange{range, tokens->at(i + 1).fileRange};
            i++;
          }
        }
        cacheExp = new ast::StringLiteral(value, range);
        break;
      }
      case TokenType::Default: {
        auto start = i;
        if (isNext(TokenType::genericTypeStart, i)) {
          auto gEndRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i, false);
          if (gEndRes.has_value()) {
            auto typeres = parseType(preCtx, i, gEndRes);
            if (typeres.second > gEndRes.value()) {
              Error("Type parsing exceeded the recognised end of type specification of the default constant expression",
                    RangeSpan(start, typeres.second));
            }
            cacheExp = new ast::ConstantDefault(typeres.first, RangeSpan(start, gEndRes.value()));
            i        = gEndRes.value();
            break;
          } else {
            Error("Expected > to end the type specification for the default constant expression", RangeAt(i));
          }
        }
        cacheExp = new ast::ConstantDefault(None, RangeAt(i));
        break;
      }
      default: {
        if (cacheExp.has_value()) {
          return {cacheExp.value(), i - 1};
        } else {
          Error("No expression parsed and invalid token found here", RangeAt(from));
        }
      }
    }
  }
  if (cacheExp.has_value()) {
    return {cacheExp.value(), i - 1};
  } else {
    Error("No expression found", RangeAt(from));
  }
}

Vec<ast::FillGeneric*> Parser::parseGenericFill(ParserContext& preCtx, usize from, usize upto) {
  using lexer::TokenType;
  Vec<ast::FillGeneric*> result;
  for (usize i = from + 1; i < upto; i++) {
    auto& token = tokens->at(i);
    switch (token.type) {
      case TokenType::maybe:
      case TokenType::voidType:
      case TokenType::cstringType:
      case TokenType::stringSliceType:
      case TokenType::unsignedIntegerType:
      case TokenType::integerType:
      case TokenType::floatType:
      case TokenType::future: {
        auto subRes = parseType(preCtx, i - 1, upto);
        result.push_back(new ast::FillGeneric(subRes.first));
        i = subRes.second;
        break;
      }
      case TokenType::super:
      case TokenType::identifier: {
        auto start  = i;
        auto symRes = parseSymbol(preCtx, i);
        auto name   = symRes.first.name;
        i           = symRes.second;
        if (isNext(TokenType::genericTypeStart, i)) {
          auto subRes = parseType(preCtx, start, None);
          result.push_back(new ast::FillGeneric(subRes.first));
          i = subRes.second;
        } else {
          result.push_back(
              new ast::FillGeneric(new ast::ConstantEntity(symRes.first.relative, name, RangeSpan(start, i))));
        }
        break;
      }
      case TokenType::parenthesisOpen: {
        auto start        = i;
        auto pCloseResult = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i, false);
        if (!pCloseResult.has_value()) {
          Error("Expected )", token.fileRange);
        }
        if (pCloseResult.value() > upto) {
          Error("Invalid position for )", RangeAt(pCloseResult.value()));
        }
        if (isPrimaryWithin(TokenType::semiColon, i, pCloseResult.value())) {
          auto               pClose = pCloseResult.value();
          Vec<ast::QatType*> subTypes;
          for (usize j = i; j < pClose; j++) {
            if (isPrimaryWithin(TokenType::semiColon, j, pClose)) {
              auto semiPosResult = firstPrimaryPosition(TokenType::semiColon, j);
              if (!semiPosResult.has_value()) {
                Error("Invalid position of ; separator", token.fileRange);
              }
              auto semiPos = semiPosResult.value();
              subTypes.push_back(parseType(preCtx, j, semiPos).first);
              j = semiPos - 1;
            } else if (j != (pClose - 1)) {
              subTypes.push_back(parseType(preCtx, j, pClose).first);
              j = pClose;
            }
          }
          i = pClose;
          result.push_back(new ast::FillGeneric(new ast::TupleType(subTypes, false, false, token.fileRange)));
        } else {
          auto constRes = parseConstantExpression(preCtx, i, pCloseResult.value());
          result.push_back(new ast::FillGeneric(constRes.first));
          if (constRes.second >= pCloseResult.value()) {
            Error("Parsing the constant expression wrapped inside ( and ) exceeded the position of )",
                  RangeSpan(start, constRes.second));
          }
          i = pCloseResult.value();
        }
        break;
      }
      default: {
        auto constRes = parseConstantExpression(preCtx, i - 1, None);
        result.push_back(new ast::FillGeneric(constRes.first));
        i = constRes.second;
      }
    }
    if (isNext(TokenType::separator, i)) {
      i++;
    } else if (!(i == (upto - 1))) {
      Error("Unexpected token found here", RangeAt(i + 1));
    }
  }
  return result;
}

Pair<ast::QatType*, usize> Parser::parseType(ParserContext& preCtx, usize from, Maybe<usize> upto) {
  using lexer::Token;
  using lexer::TokenType;

  bool variable       = false;
  auto getVariability = [&variable]() {
    auto value = variable;
    variable   = false;
    return value;
  };

  auto                 ctx = ParserContext(preCtx);
  usize                i   = 0; // NOLINT(readability-identifier-length)
  Maybe<ast::QatType*> cacheTy;

  for (i = from + 1; i < (upto.has_value() ? upto.value() : tokens->size()); i++) {
    Token& token = tokens->at(i);
    switch (token.type) {
      case TokenType::var: {
        variable = true;
        break;
      }
      case TokenType::future: {
        auto subRes = parseType(preCtx, i, upto);
        cacheTy     = new ast::FutureType(false, subRes.first, RangeSpan(i, subRes.second));
        i           = subRes.second;
        break;
      }
      case TokenType::maybe: {
        auto subRes = parseType(preCtx, i, upto);
        cacheTy     = new ast::MaybeType(getVariability(), subRes.first, RangeSpan(i, subRes.second));
        i           = subRes.second;
        break;
      }
      case TokenType::parenthesisOpen: {
        if (cacheTy.has_value()) {
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i, false);
          if (pCloseRes.has_value()) {
            auto hasPrimary = isPrimaryWithin(TokenType::semiColon, i, pCloseRes.value());
            if (hasPrimary) {
              Error("Tuple type found after another type", FileRange(token.fileRange, RangeAt(pCloseRes.value())));
            } else {
              return {cacheTy.value(), i - 1};
            }
          } else {
            Error("Expected )", token.fileRange);
          }
        }
        auto pCloseResult = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i, false);
        if (!pCloseResult.has_value()) {
          Error("Expected )", token.fileRange);
        }
        if (upto.has_value() && (pCloseResult.value() > upto.value())) {
          Error("Invalid position for )", RangeAt(pCloseResult.value()));
        }
        auto               pClose = pCloseResult.value();
        Vec<ast::QatType*> subTypes;
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
        cacheTy = new ast::TupleType(subTypes, false, getVariability(), token.fileRange);
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
        // FIXME - Change usize to work in cross compilation
        cacheTy = new ast::UnsignedType((token.value == "usize")
                                            // NOLINTNEXTLINE(readability-magic-numbers)
                                            ? (sizeof(usize) * 8u)
                                            : ((token.value == "bool") ? 1u : std::stoul(token.value)),
                                        getVariability(), token.value == "bool", token.fileRange);
        break;
      }
      case TokenType::integerType: {
        if (cacheTy.has_value()) {
          return {cacheTy.value(), i - 1};
        }
        cacheTy = new ast::IntegerType(std::stoul(token.value), getVariability(), token.fileRange);
        break;
      }
      case TokenType::floatType: {
        if (cacheTy.has_value()) {
          return {cacheTy.value(), i - 1};
        }
        if (token.value == "brain") {
          cacheTy = new ast::FloatType(IR::FloatTypeKind::_brain, getVariability(), token.fileRange);
        } else if (token.value == "half") {
          cacheTy = new ast::FloatType(IR::FloatTypeKind::_half, getVariability(), token.fileRange);
        } else if (token.value == "32") {
          cacheTy = new ast::FloatType(IR::FloatTypeKind::_32, getVariability(), token.fileRange);
        } else if (token.value == "64") {
          cacheTy = new ast::FloatType(IR::FloatTypeKind::_64, getVariability(), token.fileRange);
        } else if (token.value == "80") {
          cacheTy = new ast::FloatType(IR::FloatTypeKind::_80, getVariability(), token.fileRange);
        } else if (token.value == "128") {
          cacheTy = new ast::FloatType(IR::FloatTypeKind::_128, getVariability(), token.fileRange);
        } else if (token.value == "128ppc") {
          cacheTy = new ast::FloatType(IR::FloatTypeKind::_128PPC, getVariability(), token.fileRange);
        } else {
          cacheTy = new ast::FloatType(IR::FloatTypeKind::_32, getVariability(), token.fileRange);
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
        if (isNext(TokenType::genericTypeStart, i)) {
          auto endRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1, false);
          if (endRes.has_value()) {
            auto end = endRes.value();
            if (endRes.value() == i + 2) {
              cacheTy = new ast::GenericNamedType(symRes.first.relative, symRes.first.name, {}, getVariability(),
                                                  RangeSpan(start, end));
            } else {
              auto types = parseGenericFill(preCtx, i + 1, end);
              cacheTy    = new ast::GenericNamedType(symRes.first.relative, symRes.first.name, types, getVariability(),
                                                     RangeSpan(start, end));
              i          = end;
            }
            break;
          } else {
            Error("Expected end for generic type specification", RangeAt(i + 1));
          }
        } else if ((name.size() == 1) &&
                   (preCtx.hasTypedGeneric(name.front().value) || preCtx.hasConstGeneric(name.front().value))) {
          ast::GenericAbstractType* genericParameter;
          if (preCtx.hasTypedGeneric(name.front().value)) {
            genericParameter = preCtx.getTypedGeneric(name.front().value);
          } else {
            genericParameter = preCtx.getConstGeneric(name.front().value);
          }
          cacheTy = new ast::LinkedGeneric(getVariability(), genericParameter, name.front().range);
          i       = symRes.second;
          break;
        } else {
          cacheTy = new ast::NamedType(symRes.first.relative, name, getVariability(), symRes.first.fileRange);
          i       = symRes.second;
          break;
        }
        break;
      }
      case TokenType::referenceType: {
        if (cacheTy.has_value()) {
          return {cacheTy.value(), i - 1};
        }
        auto subRes = parseType(ctx, i, upto);
        i           = subRes.second;
        cacheTy     = new ast::ReferenceType(subRes.first, getVariability(), FileRange(token.fileRange, RangeAt(i)));
        break;
      }
      case TokenType::pointerType: {
        if (cacheTy.has_value()) {
          return {cacheTy.value(), i - 1};
        }
        if (isNext(TokenType::bracketOpen, i)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
          if (bCloseRes.has_value() && (!upto.has_value() || (bCloseRes.value() < upto.value()))) {
            auto bClose     = bCloseRes.value();
            bool isMultiPtr = false;
            if (isNext(TokenType::binaryOperator, i + 1) && (ValueAt(i + 2) == "+")) {
              isMultiPtr = true;
              i++;
            }
            if (isPrimaryWithin(TokenType::ternary, i + 1, bClose)) {
              auto questionPos = firstPrimaryPosition(TokenType::ternary, i + 1).value();
              auto subTypeRes  = parseType(ctx, i + 1, questionPos);
              if (questionPos != (bClose - 1)) {
                Error("Unexpected tokens after the anonymous ownership marker", RangeSpan(questionPos, bClose - 1));
              }
              cacheTy = new ast::PointerType(subTypeRes.first, getVariability(), ast::PtrOwnType::anonymous, None,
                                             isMultiPtr, {token.fileRange, RangeAt(bClose)});
            } else if (isPrimaryWithin(TokenType::child, i + 1, bClose)) {
              auto childPos   = firstPrimaryPosition(TokenType::child, i + 1).value();
              auto subTypeRes = parseType(ctx, i + 1, childPos);
              if (isNext(TokenType::heap, childPos)) {
                if (childPos + 2 != bClose) {
                  Error("Invalid ownership 'heap", RangeSpan(childPos, bClose));
                }
                cacheTy = new ast::PointerType(subTypeRes.first, getVariability(), ast::PtrOwnType::heap, None,
                                               isMultiPtr, {token.fileRange, RangeAt(bClose)});
              } else if (isNext(TokenType::Type, childPos)) {
                if (isNext(TokenType::parenthesisOpen, childPos + 1)) {
                  auto pCloseRes =
                      getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, childPos + 2, false);
                  if (pCloseRes.has_value()) {
                    // FIXME - Less assumptions about end of the type
                    cacheTy = new ast::PointerType(subTypeRes.first, getVariability(), ast::PtrOwnType::type,
                                                   parseType(preCtx, childPos + 2, pCloseRes.value()).first, isMultiPtr,
                                                   {token.fileRange, RangeAt(bClose)});
                  } else {
                    Error("Expected end for (", RangeAt(childPos + 2));
                  }
                } else {
                  if (childPos + 2 != bClose) {
                    Error("Invalid ownership variant 'type", RangeSpan(childPos, bClose));
                  }
                  cacheTy = new ast::PointerType(subTypeRes.first, getVariability(), ast::PtrOwnType::typeParent, None,
                                                 isMultiPtr, {token.fileRange, RangeAt(bClose)});
                }
              } else {
                Error("Invalid ownership of the pointer", {token.fileRange, RangeAt(childPos)});
              }
            } else {
              auto subTypeRes = parseType(ctx, i + 1, bClose);
              cacheTy         = new ast::PointerType(subTypeRes.first, getVariability(), ast::PtrOwnType::parent, None,
                                                     isMultiPtr, {token.fileRange, RangeAt(bClose)});
            }
            i = bClose;
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
        auto bClose = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
        if (bClose.has_value()) {
          auto lengthExp = parseConstantExpression(preCtx, i, bClose);
          cacheTy        = new ast::ArrayType(cacheTy.value(), lengthExp.first, getVariability(), token.fileRange);
          if (lengthExp.second > bClose.value()) {
            Error("Invalid end for the constant expression specifying the length of the array",
                  RangeSpan(i, lengthExp.second));
          }
          i = bClose.value();
        } else {
          Error("Expected end for [", RangeAt(i));
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

Vec<ast::GenericAbstractType*> Parser::parseGenericAbstractTypes(ParserContext& preCtx, usize from, usize upto) {
  using lexer::TokenType;

  Vec<ast::GenericAbstractType*> result;
  for (usize i = from + 1; i < upto; i++) {
    auto token = tokens->at(i);
    if (token.type == TokenType::identifier) {
      if (isNext(TokenType::assignment, i)) {
        auto typRes = parseType(preCtx, i + 1, None);
        result.push_back(
            ast::TypedGeneric::get(result.size(), IdentifierAt(i), typRes.first, RangeSpan(i, typRes.second)));
        i = typRes.second;
      } else {
        result.push_back(ast::TypedGeneric::get(result.size(), IdentifierAt(i), None, token.fileRange));
      }
      if (isNext(TokenType::separator, i)) {
        i++;
        continue;
      } else if (isNext(TokenType::genericTypeEnd, i)) {
        break;
      } else {
        Error("Unexpected token after identifier in generic parameter specification", token.fileRange);
      }
    } else if (token.type == TokenType::constant) {
      auto start = i;
      if (isNext(TokenType::identifier, i)) {
        auto constGenName = IdentifierAt(i + 1);
        if (isNext(TokenType::typeSeparator, i + 1)) {
          if (isNext(TokenType::separator, i + 2) || isNext(TokenType::genericTypeEnd, i + 2)) {
            Error("Expected type for the constant generic parameter", RangeSpan(i, i + 3));
          }
          auto typeRes = parseType(preCtx, i + 2, None);
          if (typeRes.second >= upto) {
            Error(
                "Parsing type for the const generic parameter surpassed the end of the recognised generic type specification",
                RangeSpan(i, typeRes.second));
          }
          i = typeRes.second;
          Maybe<ast::ConstantExpression*> defaultValue;
          if (isNext(TokenType::assignment, i)) {
            if (isNext(TokenType::separator, i + 1) || isNext(TokenType::genericTypeEnd, i + 1)) {
              Error(
                  "Expected a constant expression for the default value of the const generic parameter. Found nothing.",
                  RangeSpan(start, i + 2));
            }
            auto constRes = parseConstantExpression(preCtx, i + 1, None);
            if (constRes.second >= upto) {
              Error(
                  "Parsing constant expression for the default value for the const generic parameter surpassed the end of the recognised generic type specification",
                  RangeSpan(start, constRes.second));
            }
            defaultValue = constRes.first;
            i            = constRes.second;
          }
          result.push_back(
              ast::ConstGeneric::get(result.size(), constGenName, typeRes.first, defaultValue, RangeSpan(start, i)));
          if (isNext(TokenType::separator, i)) {
            i++;
          } else if (isNext(TokenType::genericTypeEnd, i)) {
            break;
          }
        } else {
          Error("Expected :: after the name of the const generic parameter", RangeSpan(i, i + 1));
        }
      } else {
        Error("Expected an identifier found after `const` in generic parameter specification", RangeAt(i));
      }
    } else {
      Error("Unexpected token found in the generic parameter specification", token.fileRange);
    }
  }
  return result;
}

Vec<ast::Node*> Parser::parse(ParserContext preCtx, // NOLINT(misc-no-recursion)
                              usize from, usize upto) {
  if (parseRecurseCount == 0) {
    latestStartTime = std::chrono::high_resolution_clock::now();
    SHOW("Parse start time: " << latestStartTime.time_since_epoch().count())
  }
  parseRecurseCount++;
  SHOW("Parse main function recurse count: " << parseRecurseCount)
  Vec<ast::Node*> result;
  using lexer::Token;
  using lexer::TokenType;

  if (upto == -1) {
    upto = tokens->size();
  }

  ParserContext ctx(preCtx);

  Maybe<CacheSymbol> c_sym           = None;
  auto               setCachedSymbol = [&c_sym](const CacheSymbol& sym) { c_sym = sym; };
  auto               getCachedSymbol = [&c_sym]() {
    auto result = c_sym.value();
    c_sym       = None;
    return result;
  };
  auto hasCachedSymbol = [&c_sym]() { return c_sym.has_value(); };

  std::deque<Token>         cacheT;
  std::deque<ast::QatType*> cacheTy;
  String                    context = "global";

  Maybe<utils::VisibilityKind> visibility;
  auto                         setVisibility = [&](utils::VisibilityKind kind) { visibility = kind; };
  auto                         getVisibility = [&]() {
    auto res   = visibility.value_or(utils::VisibilityKind::parent);
    visibility = None;
    return res;
  };

  Maybe<bool> asyncState;
  auto        setAsync = [&]() { asyncState = true; };
  auto        getAsync = [&]() {
    bool res   = asyncState.value_or(false);
    asyncState = false;
    return res;
  };

  for (usize i = (from + 1); i < upto; i++) {
    Token& token = tokens->at(i);
    switch (token.type) { // NOLINT(clang-diagnostic-switch)
      case TokenType::endOfFile: {
        break;
      }
      case TokenType::Public: {
        auto kindRes = parseVisibilityKind(i);
        setVisibility(kindRes.first);
        i = kindRes.second;
        break;
      }
      case TokenType::meta: {
        if (isNext(TokenType::child, i)) {
          if (isNext(TokenType::identifier, i + 1)) {
            if (ValueAt(i + 2) == "moduleInfo") {
              if (isNext(TokenType::curlybraceOpen, i + 2)) {
                auto bCloseRes = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 3, false);
                if (bCloseRes.has_value()) {
                  result.push_back(parseModuleInfo(i + 3, bCloseRes.value(), RangeAt(i)));
                  i = bCloseRes.value();
                } else {
                  Error("Expected end for {", RangeAt(i + 3));
                }
              } else {
                Error("Expected { to start the module information", RangeAt(i + 2));
              }
            } else {
              Error("Unexpected metadata", RangeSpan(i, i + 2));
            }
          } else {
            Error("Expected an identifier after meta'", RangeSpan(i, i + 1));
          }
        } else {
          Error("Unexpected token", RangeAt(i));
        }
        break;
      }
      case TokenType::New: {
        auto start = i;
        bool isVar = false;
        if (isNext(TokenType::var, i)) {
          isVar = true;
          i++;
        }
        if (isNext(TokenType::identifier, i)) {
          ast::Expression* exp    = nullptr;
          ast::QatType*    typ    = nullptr;
          auto             endRes = firstPrimaryPosition(TokenType::stop, i + 1);
          if (!endRes.has_value()) {
            Error("Expected end for the global declaration", RangeSpan(start, i + 1));
          }
          if (isNext(TokenType::colon, i + 1)) {
            if (isPrimaryWithin(TokenType::assignment, i + 2, endRes.value())) {
              auto assignPos = firstPrimaryPosition(TokenType::assignment, i + 2).value();
              typ            = parseType(preCtx, i + 2, assignPos).first;
              exp            = parseExpression(preCtx, None, assignPos, endRes.value()).first;
            } else {
              typ = parseType(preCtx, i + 2, endRes.value()).first;
            }
          }
          result.push_back(new ast::GlobalDeclaration(IdentifierAt(i + 1), typ, exp, isVar, getVisibility(),
                                                      RangeSpan(start, endRes.value())));
          i = endRes.value();
        } else {
          Error("Expected name for the global declaration",
                isVar ? FileRange(tokens->at(start).fileRange, tokens->at(start + 1).fileRange) : RangeAt(start));
        }
        break;
      }
      case TokenType::bring: {
        // FIXME - Support bringing entities
        if (isNext(TokenType::StringLiteral, i) ||
            (isNext(TokenType::child, i) && isNext(TokenType::identifier, i + 1) && (ValueAt(i + 2) == "member"))) {
          bool isMember = !isNext(TokenType::StringLiteral, i);
          auto endRes   = firstPrimaryPosition(TokenType::stop, i);
          if (endRes.has_value()) {
            result.push_back(
                parseBroughtPaths(isMember, isMember ? i + 2 : i, endRes.value(), getVisibility(), RangeAt(i)));
            i = endRes.value();
          } else {
            Error("Expected end of bring sentence", token.fileRange);
          }
        } else if (isNext(TokenType::identifier, i)) {
          auto endRes = firstPrimaryPosition(TokenType::stop, i);
          if (endRes.has_value()) {
            result.push_back(parseBroughtEntities(ctx, i, endRes.value()));
            i = endRes.value();
          }
        }
        break;
      }
      case TokenType::mix: {
        if (isNext(TokenType::identifier, i)) {
          if (isNext(TokenType::curlybraceOpen, i + 1)) {
            auto bCloseRes = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 2, false);
            if (bCloseRes.has_value()) {
              auto                                        bClose = bCloseRes.value();
              Vec<Pair<Identifier, Maybe<ast::QatType*>>> subTypes;
              Vec<FileRange>                              fileRanges;
              Maybe<usize>                                defaultVal;
              parseMixType(preCtx, i + 2, bClose, subTypes, fileRanges, defaultVal);
              // FIXME - Support packing
              result.push_back(new ast::DefineMixType(IdentifierAt(i + 1), std::move(subTypes), std::move(fileRanges),
                                                      defaultVal, false, getVisibility(), RangeSpan(i, bClose)));
              i = bClose;
            } else {
              Error("Expected end for {", RangeAt(i + 2));
            }
          } else {
            Error("Expected { to start the definition of the mix type", RangeSpan(i, i + 1));
          }
        } else {
          Error("Expected an identifier for the name of the mix type", RangeAt(i));
        }
        break;
      }
      case TokenType::choice: {
        if (isNext(TokenType::identifier, i)) {
          if (isNext(TokenType::curlybraceOpen, i + 1)) {
            auto bCloseRes = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 2, false);
            if (bCloseRes.has_value()) {
              auto                                                       bClose = bCloseRes.value();
              Vec<Pair<Identifier, Maybe<ast::DefineChoiceType::Value>>> fields;
              Maybe<usize>                                               defaultVal;
              parseChoiceType(i + 2, bClose, fields, defaultVal);
              result.push_back(new ast::DefineChoiceType(IdentifierAt(i + 1), std::move(fields), defaultVal,
                                                         getVisibility(), RangeSpan(i, bClose)));
              i = bClose;
            } else {
              Error("Expected end for {", RangeAt(i + 2));
            }
          } else {
            Error("Expected { to start the definition of the choice type", RangeSpan(i, i + 1));
          }
        } else {
          Error("Expected an identifier for the name of the choice type", RangeAt(i));
        }
        break;
      }
      case TokenType::region: {
        if (isNext(TokenType::identifier, i)) {
          if (isNext(TokenType::stop, i + 1)) {
            result.push_back(new ast::DefineRegion(IdentifierAt(i + 1), getVisibility(), RangeSpan(i, i + 2)));
            i += 2;
          } else {
            Error("Expected . to end the region declaration", RangeSpan(i, i + 1));
          }
        } else {
          Error("Expected an identifier for the name of the region", RangeAt(i));
        }
        break;
      }
      case TokenType::Type: {
        // TODO - Consider other possible tokens and generic types instead of
        // just identifiers
        if (isNext(TokenType::identifier, i)) {
          auto                           name = IdentifierAt(i + 1);
          Vec<ast::GenericAbstractType*> genericList;
          auto                           ctx = ParserContext();
          i++;
          if (isNext(TokenType::genericTypeStart, i)) {
            auto endRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1, false);
            if (endRes.has_value()) {
              auto end    = endRes.value();
              genericList = parseGenericAbstractTypes(preCtx, i + 1, end);
              for (auto* temp : genericList) {
                ctx.addAbstractGeneric(temp);
              }
              i = end;
            } else {
              Error("Expected end for generic type specification", RangeAt(i + 1));
            }
          }
          if (isNext(TokenType::assignment, i)) {
            SHOW("Parsing type definition")
            auto endRes = firstPrimaryPosition(TokenType::stop, i + 1);
            if (endRes.has_value()) {
              auto* typ = parseType(ctx, i + 1, endRes.value()).first;
              result.push_back(new ast::TypeDefinition(name, typ, RangeSpan(i, endRes.value()), getVisibility()));
              i = endRes.value();
              break;
            } else {
              Error("Invalid end of type definition", RangeSpan(i, i + 2));
            }
          } else if (isNext(TokenType::curlybraceOpen, i)) {
            auto bClose = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1, false);
            if (bClose.has_value()) {
              auto* tRes = new ast::DefineCoreType(name, getVisibility(), {token.fileRange, RangeAt(bClose.value())},
                                                   genericList);
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
            auto bClose = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 2, false);
            if (bClose.has_value()) {
              auto contents = parse(ctx, i + 2, bClose.value());
              result.push_back(new ast::Lib(IdentifierAt(i + 1), contents, getVisibility(),
                                            FileRange(token.fileRange, RangeAt(bClose.value()))));
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
            auto bClose = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 2, false);
            if (bClose.has_value()) {
              auto contents = parse(ctx, i + 2, bClose.value());
              result.push_back(new ast::Box(IdentifierAt(i + 1), contents, getVisibility(),
                                            FileRange(token.fileRange, RangeAt(bClose.value()))));
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
      case TokenType::var: {
        auto typeRes = parseType(preCtx, i, None);
        i            = typeRes.second;
        if (isNext(TokenType::identifier, i)) {

        } else {
          Error("Expected identifier for the name of the global variable after the type", typeRes.first->fileRange);
        }
        break;
      }
      case TokenType::Async: {
        if (!isNext(TokenType::identifier, i)) {
          Error("Expected an identifier for the name of the function, after async", RangeAt(i));
        }
        setAsync();
        break;
      }
      case TokenType::identifier: {
        auto start   = i;
        auto sym_res = parseSymbol(ctx, i);
        i            = sym_res.second;
        Vec<ast::GenericAbstractType*> genericList;
        auto                           ctx = ParserContext();
        if (isNext(TokenType::genericTypeStart, i)) {
          auto endRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1, false);
          if (endRes.has_value()) {
            auto end    = endRes.value();
            genericList = parseGenericAbstractTypes(preCtx, i + 1, end);
            for (auto* temp : genericList) {
              ctx.addAbstractGeneric(temp);
            }
            if (isNext(TokenType::givenTypeSeparator, end) || isNext(TokenType::parenthesisOpen, end)) {
              i = end;
            } else {
              // FIXME - Generic types in global declaration
              if (isNext(TokenType::colon, end)) {
                if (isNext(TokenType::identifier, end + 1)) {
                }
              }
            }
          } else {
            Error("Expected end for generic type specification", RangeAt(i + 1));
          }
        }
        if (isNext(TokenType::parenthesisOpen, i) || isNext(TokenType::givenTypeSeparator, i)) {
          ast::QatType* retType = nullptr;
          if (isNext(TokenType::givenTypeSeparator, i)) {
            auto typRes = parseType(ctx, i + 1, None);
            retType     = typRes.first;
            i           = typRes.second;
          } else {
            retType = new ast::VoidType(false, RangeAt(start));
          }
          if (!isNext(TokenType::parenthesisOpen, i)) {
            Error("Expected ( for arguments in function declaration", RangeSpan(start, i));
          }
          SHOW("Function with void return type")
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
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
                Error("Expected calling convention string after extern", RangeAt(i + 1));
              }
            }
            SHOW("Creating prototype")
            auto  IsAsync   = getAsync();
            auto* prototype = new ast::FunctionPrototype(
                IdentifierAt(start), argResult.first, argResult.second, retType, IsAsync,
                llvm::GlobalValue::WeakAnyLinkage, callConv, getVisibility(),
                RangeSpan((isPrev(TokenType::identifier, start) ? start - 1 : start), pClose), genericList);
            SHOW("Prototype created")
            if (isNext(TokenType::bracketOpen, i)) {
              auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
              if (bCloseRes.has_value()) {
                auto bClose    = bCloseRes.value();
                auto sentences = parseSentences(ctx, i + 1, bClose);
                SHOW("Function definition created")
                result.push_back(new ast::FunctionDefinition(prototype, sentences, RangeSpan(i + 1, bClose)));
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
        // TODO - Add support for generic types for functions
        if (!hasCachedSymbol()) {
          Error("Function name not provided", token.fileRange);
        }
        SHOW("Parsing Type")
        auto retTypeRes = parseType(ctx, i, None);
        SHOW("Type parsing complete")
        auto* retType = retTypeRes.first;
        i             = retTypeRes.second;
        if (isNext(TokenType::parenthesisOpen, i)) {
          SHOW("Found (")
          auto pCloseResult = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
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
          for (auto* arg : argResult.first) {
            SHOW("Arg name " << arg->getName().value)
          }
          bool IsAsync  = getAsync();
          auto cacheSym = getCachedSymbol();
          if (cacheSym.name.size() > 1) {
            Error("Function name should be just one identifier", cacheSym.fileRange);
          }
          auto* prototype = new ast::FunctionPrototype(
              cacheSym.name.front(), argResult.first, argResult.second, retType, IsAsync,
              isExternal ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::WeakAnyLinkage, callConv,
              getVisibility(),
              isPrev(TokenType::Async, cacheSym.tokenIndex)
                  ? FileRange{RangeAt(cacheSym.tokenIndex - 1), token.fileRange}
                  : FileRange{RangeAt(cacheSym.tokenIndex), token.fileRange});
          SHOW("Prototype created")
          if (!isExternal) {
            if (isNext(TokenType::bracketOpen, pClose)) {
              auto bCloseResult = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, pClose + 1, false);
              if (!bCloseResult.has_value() || (bCloseResult.value() >= tokens->size())) {
                Error("Expected ] at the end of the Function Definition", RangeAt(pClose + 1));
              }
              SHOW("HAS BCLOSE")
              auto bClose = bCloseResult.value();
              SHOW("Starting sentence parsing")
              auto sentences = parseSentences(ctx, pClose + 1, bClose);
              SHOW("Sentence parsing completed")
              auto* definition =
                  new ast::FunctionDefinition(prototype, sentences, FileRange(RangeAt(pClose + 1), RangeAt(bClose)));
              SHOW("Function definition created")
              result.push_back(definition);
              i = bClose;
              continue;
            } else {
              // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
              Error("Expected definition for non-external function", token.fileRange);
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
  parseRecurseCount--;
  if (parseRecurseCount == 0) {
    auto parseEndTime = std::chrono::high_resolution_clock::now();
    SHOW("Parse end time: " << parseEndTime.time_since_epoch().count())
    timeInMicroSeconds += std::chrono::duration_cast<std::chrono::microseconds>(parseEndTime - latestStartTime).count();
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
        Error("Invalid identifier found after pub' for visibility", RangeAt(from + 2));
      }
    } else {
      Error("Invalid token found after pub' for visibility", RangeSpan(from, from + 1));
    }
  } else {
    return {utils::VisibilityKind::pub, from};
  }
} // NOLINT(clang-diagnostic-return-type)

// FIXME - Finish functionality for parsing type contents
void Parser::parseCoreType(ParserContext& preCtx, usize from, usize upto, ast::DefineCoreType* coreTy) {
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
  auto                         setVisibility = [&](utils::VisibilityKind kind) { visibility = kind; };
  auto                         getVisibility = [&]() {
    auto res   = visibility.value_or(broadVisib.value_or(utils::VisibilityKind::type));
    visibility = None;
    return res;
  };

  Maybe<ast::QatType*> cacheTy;

  for (usize i = from + 1; i < upto; i++) {
    Token& token = tokens->at(i);
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
        tokens->at(i);
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
        if (!isNext(TokenType::identifier, i) && !isNext(TokenType::Async, i) && !isNext(TokenType::Operator, i)) {
          Error("Expected `operator`, `async` or the identifier for the name of "
                "the member function after <~",
                RangeAt(i));
        }
        setVariation();
        break;
      }
      case TokenType::Async: {
        if (!isNext(TokenType::identifier, i)) {
          Error("Expected identifier after async. Unexpected token found", RangeAt(i));
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
        auto typeRes = parseType(preCtx, i - 1, None);
        cacheTy      = typeRes.first;
        i            = typeRes.second;
        break;
      }
      case TokenType::identifier: {
        SHOW("Identifier inside core type: " << token.value)
        auto start = i;
        if (isNext(TokenType::givenTypeSeparator, i) || isNext(TokenType::parenthesisOpen, i)) {
          SHOW("Member function start")
          ast::QatType* retTy = isNext(TokenType::parenthesisOpen, i) ? new ast::VoidType(false, RangeAt(i)) : nullptr;
          if (!retTy) {
            auto typeRes = parseType(preCtx, i + 1, None);
            retTy        = typeRes.first;
            i            = typeRes.second;
          }
          if (isNext(TokenType::parenthesisOpen, i)) {
            auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
            if (pCloseRes.has_value()) {
              auto pClose  = pCloseRes.value();
              auto argsRes = parseFunctionParameters(preCtx, i + 1, pClose);
              if (isNext(TokenType::bracketOpen, pClose)) {
                auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, pClose + 1, false);
                if (bCloseRes.has_value()) {
                  auto bClose = bCloseRes.value();
                  auto snts   = parseSentences(preCtx, pClose + 1, bClose);
                  SHOW("Creating member function prototype")
                  coreTy->addMemberDefinition(new ast::MemberDefinition(
                      getStatic()
                          ? ast::MemberPrototype::Static(IdentifierAt(start), argsRes.first, argsRes.second, retTy,
                                                         getAsync(), getVisibility(), RangeSpan(start, pClose + 1))
                          : ast::MemberPrototype::Normal(getVariation(), IdentifierAt(start), argsRes.first,
                                                         argsRes.second, retTy, getAsync(), getVisibility(),
                                                         RangeSpan(start, pClose + 1)),
                      snts, RangeSpan(start, bClose)));
                  i = bClose;
                }
              }
            } else {
              Error("Expected end for (", RangeAt(i + 1));
            }
          } else {
            Error("Expected ( for the arguments of the member function", RangeSpan(start, i));
          }
        } else if (isNext(TokenType::typeSeparator, i)) {
          auto stop = firstPrimaryPosition(TokenType::stop, i + 1);
          if (stop.has_value()) {
            auto typeRes = parseType(preCtx, i + 1, stop.value());
            coreTy->addMember(new ast::DefineCoreType::Member(typeRes.first, Identifier(token.value, token.fileRange),
                                                              !getConst(), getVisibility(),
                                                              FileRange(typeRes.first->fileRange, token.fileRange)));
            cacheTy = None;
            i       = stop.value();
          } else {
            Error("Expected . after member declaration", token.fileRange);
          }
        } else {
          auto typeRes = parseType(preCtx, i - 1, None);
          cacheTy      = typeRes.first;
          SHOW("Type parsed inside core type is " << typeRes.first->toString())
          SHOW("Type parsed inside core type kind is " << (int)typeRes.first->typeKind())
          i = typeRes.second;
        }
        break;
      }
      case TokenType::Default: {
        auto start = i;
        if (coreTy->hasDefaultConstructor()) {
          Error("A default destructor is already defined for the core type", RangeAt(i));
        }
        if (isNext(TokenType::bracketOpen, i)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
          if (bCloseRes.has_value()) {
            auto bClose = bCloseRes.value();
            auto snts   = parseSentences(preCtx, i + 1, bClose);
            coreTy->setDefaultConstructor(new ast::ConstructorDefinition(
                ast::ConstructorPrototype::Default(getVisibility(), RangeAt(start), RangeAt(i)), std::move(snts),
                RangeSpan(i, bClose)));
            i = bClose;
          } else {
            Error("Expected end for [", RangeAt(i + 1));
          }
        } else {
          Error("Expected [ to start the definition of the default constructor", RangeAt(i));
        }
        break;
      }
      case TokenType::copy: {
        auto start = i;
        if (coreTy->hasCopyConstructor()) {
          Error("A copy constructor is already defined for the core type", RangeAt(i));
        }
        if (isNext(TokenType::identifier, i)) {
          auto argName = IdentifierAt(i + 1);
          if (isNext(TokenType::bracketOpen, i + 1)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 2, false);
            if (bCloseRes.has_value()) {
              auto bClose = bCloseRes.value();
              auto snts   = parseSentences(preCtx, i + 2, bClose);
              coreTy->setCopyConstructor(new ast::ConstructorDefinition(
                  ast::ConstructorPrototype::Copy(RangeAt(start), RangeSpan(i, i + 1), argName), std::move(snts),
                  RangeSpan(i, bClose)));
              i = bClose;
            } else {
              Error("Expected end for [", RangeAt(i + 2));
            }
          } else {
            Error("Expected [ to start the definition of the copy constructor", RangeSpan(i, i + 1));
          }
        } else {
          Error("Expected name for the argument, which represents the existing "
                "instance of the type, after copy keyword",
                RangeAt(i));
        }
        break;
      }
      case TokenType::move: {
        auto start = i;
        if (coreTy->hasMoveConstructor()) {
          Error("A move constructor is already defined for the core type", RangeAt(i));
        }
        if (isNext(TokenType::identifier, i)) {
          auto argName = IdentifierAt(i + 1);
          if (isNext(TokenType::bracketOpen, i + 1)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 2, false);
            if (bCloseRes.has_value()) {
              auto bClose = bCloseRes.value();
              auto snts   = parseSentences(preCtx, i + 2, bClose);
              coreTy->setMoveConstructor(new ast::ConstructorDefinition(
                  ast::ConstructorPrototype::Move(RangeAt(start), RangeSpan(i, i + 1), argName), std::move(snts),
                  RangeSpan(i, bClose)));
              i = bClose;
            } else {
              Error("Expected end for [", RangeAt(i + 2));
            }
          } else {
            Error("Expected [ to start the definition of the move constructor", RangeSpan(i, i + 1));
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
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto pClose  = pCloseRes.value();
            auto argsRes = parseFunctionParameters(preCtx, i + 1, pClose);
            if (isNext(TokenType::bracketOpen, pClose)) {
              auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, pClose + 1, false);
              if (bCloseRes.has_value()) {
                auto bClose = bCloseRes.value();
                auto snts   = parseSentences(preCtx, pClose + 1, bClose);
                if (argsRes.first.size() == 1) {
                  // NOTE - Convertor
                  coreTy->addConvertorDefinition(new ast::ConvertorDefinition(
                      ast::ConvertorPrototype::From(
                          RangeAt(start), argsRes.first.front()->getName(), argsRes.first.front()->getType(),
                          argsRes.first.front()->isTypeMember(), getVisibility(), RangeSpan(start, pClose)),
                      snts, RangeSpan(start, bClose)));
                } else {
                  // NOTE = Constructor
                  coreTy->addConstructorDefinition(new ast::ConstructorDefinition(
                      ast::ConstructorPrototype::Normal(RangeAt(start), std::move(argsRes.first), getVisibility(),
                                                        RangeSpan(start, pClose)),
                      snts, RangeSpan(start, bClose)));
                }
                i = bClose;
              } else {
                Error("Expected end for [", RangeAt(pClose + 1));
              }
            } else {
              Error("Expected definition for constructor", RangeSpan(start, pClose));
            }
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        }
        break;
      }
      case TokenType::Operator: {
        auto start = i;
        if (isNext(TokenType::copy, i)) {
          if (coreTy->hasCopyAssignment()) {
            Error("The copy assignment operator is already defined for this core "
                  "type",
                  RangeSpan(i, i + 1));
          }
          if (isNext(TokenType::assignment, i + 1)) {
            if (isNext(TokenType::identifier, i + 2)) {
              if (isNext(TokenType::bracketOpen, i + 3)) {
                auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 4, false);
                if (bCloseRes) {
                  auto bClose = bCloseRes.value();
                  auto snts   = parseSentences(preCtx, i + 4, bClose);
                  coreTy->setCopyAssignment(new ast::OperatorDefinition(
                      new ast::OperatorPrototype(true, ast::Op::copyAssignment, RangeSpan(start, start + 1), {},
                                                 nullptr, utils::VisibilityKind::pub, RangeSpan(i, i + 3),
                                                 IdentifierAt(i + 3)),
                      std::move(snts), RangeSpan(i, bClose)));
                  i = bClose;
                } else {
                  Error("Expected end for [", RangeAt(i + 4));
                }
              } else {
                Error("Expected definition for the copy assignment operator", RangeSpan(i, i + 3));
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
                auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 4, false);
                if (bCloseRes) {
                  auto bClose = bCloseRes.value();
                  auto snts   = parseSentences(preCtx, i + 4, bClose);
                  coreTy->setMoveAssignment(new ast::OperatorDefinition(
                      new ast::OperatorPrototype(true, ast::Op::moveAssignment, RangeAt(start), {}, nullptr,
                                                 utils::VisibilityKind::pub, RangeSpan(i, i + 3), IdentifierAt(i + 3)),
                      std::move(snts), RangeSpan(i, bClose)));
                  i = bClose;
                } else {
                  Error("Expected end for [", RangeAt(i + 4));
                }
              } else {
                Error("Expected definition for the move assignment operator", RangeSpan(i, i + 3));
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
        start = i;
        String              opr;
        bool                isUnary  = false;
        ast::QatType*       returnTy = new ast::VoidType(false, FileRange{"", {0u, 0u}, {0u, 0u}});
        Vec<ast::Argument*> args;
        if (isNext(TokenType::binaryOperator, i)) {
          SHOW("Binary operator for core type: " << ValueAt(i + 1))
          if (ValueAt(i + 1) == "-") {
            if (isNext(TokenType::parenthesisOpen, i) && isNext(TokenType::parenthesisClose, i + 1)) {
              isUnary = true;
            } else if (isNext(TokenType::givenTypeSeparator, i)) {
              auto typRes = parseType(preCtx, i + 1, None);
              returnTy    = typRes.first;
              i           = typRes.second;
              if (isNext(TokenType::parenthesisOpen, i) && isNext(TokenType::parenthesisClose, i + 1)) {
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
        } else if (isNext(TokenType::pointerType, i)) {
          isUnary = true;
          opr     = "#";
          i++;
        } else if (isNext(TokenType::assignment, i)) {
          opr = "=";
          i++;
        } else if (isNext(TokenType::bracketOpen, i)) {
          if (isNext(TokenType::bracketClose, i + 1)) {
            opr = "[]";
            i += 2;
          } else {
            Error("[ is an invalid operator. Did you mean []", RangeSpan(start, start + 1));
          }
        } else {
          Error("Invalid operator", RangeAt(i));
        }
        if (isNext(TokenType::givenTypeSeparator, i)) {
          auto typRes = parseType(preCtx, i + 1, None);
          returnTy    = typRes.first;
          i           = typRes.second;
        }
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto pClose    = pCloseRes.value();
            auto fnArgsRes = parseFunctionParameters(preCtx, i + 1, pClose);
            if (fnArgsRes.second) {
              Error("Operator functions cannot have variadic arguments", RangeSpan(start, pClose));
            }
            args = fnArgsRes.first;
            i    = pClose;
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        }
        auto* prototype = new ast::OperatorPrototype(
            getVariation(), isUnary ? (opr == "-" ? ast::Op::minus : ast::OpFromString(opr)) : ast::OpFromString(opr),
            RangeAt(start), args, returnTy, getVisibility(), RangeSpan(start, i), None);
        if (isNext(TokenType::bracketOpen, i)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
          if (bCloseRes.has_value()) {
            auto bClose = bCloseRes.value();
            auto snts   = parseSentences(preCtx, i + 1, bClose);
            coreTy->addOperatorDefinition(new ast::OperatorDefinition(prototype, snts, RangeSpan(start, bClose)));
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
        auto start  = i;
        auto typRes = parseType(preCtx, i, None);
        i           = typRes.second;
        if (isNext(TokenType::altArrow, i)) {
          if (isNext(TokenType::bracketOpen, i + 1)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 2, false);
            if (bCloseRes.has_value()) {
              auto bClose = bCloseRes.value();
              auto snts   = parseSentences(preCtx, i + 2, bClose);
              coreTy->addConvertorDefinition(new ast::ConvertorDefinition(
                  ast::ConvertorPrototype::To(RangeAt(start), typRes.first, getVisibility(), RangeSpan(start, i + 1)),
                  std::move(snts), RangeSpan(start, bClose)));
              i = bClose;
              break;
            } else {
              Error("Expected end for [", RangeAt(i + 2));
            }
          } else {
            Error("Expected [ to start the definition of the to convertor", RangeAt(i + 2));
          }
        } else {
          Error("Expected => after the type for conversion", RangeSpan(start, i));
        }
        break;
      }
      case TokenType::end: {
        auto start = i;
        if (coreTy->hasDestructor()) {
          Error("A destructor is already defined for the core type", RangeAt(i));
        }
        if (isNext(TokenType::bracketOpen, i)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
          if (bCloseRes) {
            auto snts = parseSentences(preCtx, i + 1, bCloseRes.value());
            coreTy->setDestructorDefinition(
                new ast::DestructorDefinition(RangeAt(start), snts, RangeSpan(i, bCloseRes.value())));
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

void Parser::parseMixType(ParserContext& preCtx, usize from, usize upto,
                          Vec<Pair<Identifier, Maybe<ast::QatType*>>>& uRef, Vec<FileRange>& fileRanges,
                          Maybe<usize>& defaultVal) {
  using lexer::TokenType;

  for (auto i = from + 1; i < upto; i++) {
    auto& token = tokens->at(i);
    switch (token.type) {
      case TokenType::Default: {
        if (!defaultVal.has_value()) {
          if (isNext(TokenType::identifier, i)) {
            defaultVal = uRef.size();
            break;
          } else {
            Error("Invalid token found after default", RangeAt(i));
          }
        } else {
          Error("Default value for mix type already provided", RangeAt(i));
        }
      }
      case TokenType::identifier: {
        auto start = i;
        if (isNext(TokenType::separator, i) || (isNext(TokenType::curlybraceClose, i) && (i + 1 == upto))) {
          uRef.push_back(Pair<Identifier, Maybe<ast::QatType*>>(IdentifierAt(start), None));
          fileRanges.push_back(isPrev(TokenType::Default, start)
                                   ? FileRange(tokens->at(start - 1).fileRange, tokens->at(start).fileRange)
                                   : RangeAt(start));
          i++;
        } else if (isNext(TokenType::typeSeparator, i)) {
          ast::QatType* typ;
          if (isPrimaryWithin(TokenType::separator, i + 1, upto)) {
            auto sepPos = firstPrimaryPosition(TokenType::separator, i + 1).value();
            typ         = parseType(preCtx, i + 1, sepPos).first;
            i           = sepPos;
          } else {
            typ = parseType(preCtx, i + 1, upto).first;
            i   = upto;
          }
          uRef.push_back(Pair<Identifier, Maybe<ast::QatType*>>(IdentifierAt(start), typ));
          fileRanges.push_back(isPrev(TokenType::Default, start)
                                   ? FileRange(tokens->at(start - 1).fileRange, tokens->at(start).fileRange)
                                   : RangeAt(start));
        } else {
          Error("Invalid token found after identifier in mix type definition", RangeAt(i));
        }
        break;
      }
      default: {
        Error("Invalid token found after identifier in mix type definition", RangeAt(i));
      }
    }
  }
}

void Parser::parseChoiceType(usize from, usize upto, Vec<Pair<Identifier, Maybe<ast::DefineChoiceType::Value>>>& fields,
                             Maybe<usize>& defaultVal) {
  using lexer::TokenType;

  for (usize i = from + 1; i < upto; i++) {
    auto& token = tokens->at(i);
    switch (token.type) {
      case TokenType::Default: {
        if (!defaultVal.has_value()) {
          if (isNext(TokenType::identifier, i)) {
            defaultVal = fields.size();
            break;
          } else {
            Error("Invalid token found after default", RangeAt(i));
          }
        } else {
          Error("Default value for choice type already provided", RangeAt(i));
        }
      }
      case TokenType::identifier: {
        auto start = i;
        if (isNext(TokenType::separator, i) || (i + 1 == upto)) {
          fields.push_back(Pair<Identifier, Maybe<ast::DefineChoiceType::Value>>(
              {ValueAt(i), isPrev(TokenType::Default, i)
                               ? FileRange(tokens->at(i - 1).fileRange, tokens->at(i).fileRange)
                               : RangeAt(i)},
              None));
          i++;
        } else if (isNext(TokenType::assignment, i)) {
          auto fieldName  = ValueAt(i);
          bool isNegative = false;
          if (isNext(TokenType::binaryOperator, i + 1) && ValueAt(i + 2) == "-") {
            isNegative = true;
            i += 2;
          } else {
            i += 1;
          }
          i64       val = 0;
          FileRange valRange{"", {0u, 0u}, {0u, 0u}};
          if (isNext(TokenType::integerLiteral, i) || isNext(TokenType::unsignedLiteral, i)) {
            if (ValueAt(i + 1).find('_') != String::npos) {
              val = std::stoi(ValueAt(i + 1).substr(0, ValueAt(i + 1).find('_')));
            } else {
              val = std::stoi(ValueAt(i + 1));
            }
            i++;
          } else {
            Error("Expected an integer or unsigned integer literal as the value for the variant of the choice type",
                  RangeAt(i));
          }
          if (isNegative) {
            val = -val;
          }
          if (isNext(TokenType::separator, i) || (isNext(TokenType::curlybraceClose, i) && (i + 1 == upto))) {
            fields.push_back(Pair<Identifier, Maybe<ast::DefineChoiceType::Value>>(
                Identifier{ValueAt(start), isPrev(TokenType::Default, start)
                                               ? FileRange(tokens->at(start - 1).fileRange, tokens->at(start).fileRange)
                                               : RangeAt(start)},
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

void Parser::parseMatchContents(ParserContext& preCtx, usize from, usize upto,
                                Vec<Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>>& chain,
                                Maybe<Pair<Vec<ast::Sentence*>, FileRange>>& elseCase, bool isTypeMatch) {
  using lexer::TokenType;

  for (usize i = from + 1; i < upto; i++) {
    auto& token = tokens->at(i);
    switch (token.type) {
      case TokenType::typeSeparator: {
        auto                  start            = i;
        bool                  isValueRequested = false;
        Vec<ast::MatchValue*> matchVals;
        if (isNext(TokenType::identifier, i)) {
          auto              fieldName = IdentifierAt(i + 1);
          Maybe<Identifier> valueName;
          bool              isVar = false;
          if (isNext(TokenType::parenthesisOpen, i + 1)) {
            isValueRequested = true;
            auto pCloseRes   = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2, false);
            if (pCloseRes) {
              if (isNext(TokenType::var, i + 2)) {
                isVar = true;
                i += 3;
              } else {
                i += 2;
              }
              if (isNext(TokenType::identifier, i)) {
                SHOW("Value name is: " << ValueAt(i + 1))
                matchVals.push_back(new ast::MixMatchValue(fieldName, IdentifierAt(i + 1), isVar));
                if (isNext(TokenType::parenthesisClose, i + 1)) {
                  i = pCloseRes.value();
                } else {
                  Error("Unexpected token found after identifier in match case "
                        "value name specification",
                        RangeAt(i + 1));
                }
              } else {
                Error("Expected value name for the mix subfield", RangeSpan(start, i));
              }
            } else {
              Error("Expected end for (", RangeAt(i + 2));
            }
          } else {
            matchVals.push_back(new ast::MixMatchValue(IdentifierAt(i + 1), None, false));
            i++;
          }
          auto j = i;
          while (isNext(TokenType::separator, j)) {
            if (isValueRequested) {
              Error("Matched value is requested once and hence a list of subfields to match cannot be provided",
                    RangeSpan(start, j));
            }
            if (isNext(TokenType::typeSeparator, j + 1)) {
              if (isNext(TokenType::identifier, j + 2)) {
                if (isNext(TokenType::parenthesisOpen, j + 3)) {
                  Error("Cannot request value for the matched subfield, since a list of values to match are provided",
                        RangeAt(j + 4));
                }
                matchVals.push_back(new ast::MixMatchValue(IdentifierAt(j + 3), None, false));
                if (isNext(TokenType::givenTypeSeparator, j + 3)) {
                  i = j + 3;
                  break;
                } else {
                  j += 3;
                }
              } else {
                Error("Expected name for the subfield of the mix type to match", RangeAt(j + 2));
              }
            } else {
              Error("Using normal expressions as cases for matching mix types is not supported yet", RangeAt(j + 1));
            }
          }
          if (isNext(TokenType::givenTypeSeparator, i)) {
            if (!isNext(TokenType::bracketOpen, i + 1)) {
              Error(
                  "Expected [ after -> to start the case block that contains the sentences to be executed for this case",
                  RangeAt(i + 1));
            }
            i++;
            auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
            if (bCloseRes) {
              chain.push_back(Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>(
                  matchVals, parseSentences(preCtx, i + 1, bCloseRes.value())));
              i = bCloseRes.value();
            } else {
              Error("Expected end of [", RangeAt(i + 1));
            }
          } else {
            Error("Expected -> to before the block that contains sentences to be executed for this case",
                  RangeSpan(i, i + 1));
          }
        } else {
          Error("Expected name for the subfield of the mix type to match", RangeAt(i));
        }
        break;
      }
      case TokenType::child: {
        auto start = i;
        if (isNext(TokenType::identifier, i)) {
          Vec<ast::MatchValue*> matchVals;
          matchVals.push_back(new ast::ChoiceMatchValue(IdentifierAt(i + 1)));
          if (isNext(TokenType::separator, i + 1)) {
            usize j = i + 1;
            while (isNext(TokenType::separator, j)) {
              if (isNext(TokenType::child, j + 1)) {
                if (isNext(TokenType::identifier, j + 2)) {
                  matchVals.push_back(new ast::ChoiceMatchValue(IdentifierAt(j + 3)));
                  j += 3;
                  if (isNext(TokenType::givenTypeSeparator, j)) {
                    SHOW("Found givenTypeSeparator after list of trivial choice values")
                    i = j;
                    break;
                  }
                } else {
                  Error("Expected name of the variant of the choice type to match", RangeAt(j + 1));
                }
              } else {
                if (isPrimaryWithin(TokenType::givenTypeSeparator, j + 1, upto)) {
                  auto valueEnd = firstPrimaryPosition(TokenType::givenTypeSeparator, j + 1);
                  if (isPrimaryWithin(TokenType::separator, j + 1, upto)) {
                    auto sepPos = primaryPositionsWithin(TokenType::separator, j + 1, upto);
                    matchVals.push_back(
                        new ast::ExpressionMatchValue(parseExpression(preCtx, None, j + 1, sepPos.front()).first));
                    for (usize k = 1; k < sepPos.size(); k++) {
                      if (isNext(TokenType::child, sepPos.at(k - 1))) {
                        if (isNext(TokenType::identifier, sepPos.at(k - 1) + 1)) {
                          if (isNext(TokenType::separator, sepPos.at(k - 1) + 2)) {
                            matchVals.push_back(new ast::ChoiceMatchValue(IdentifierAt(sepPos.at(k - 1) + 2)));
                          } else {
                            Error("Unexpected token found after the choice value",
                                  RangeSpan(sepPos.at(k - 1) + 1, sepPos.at(k - 1) + 2));
                          }
                        } else {
                          Error("Expected name of the variant of the choice type to match",
                                RangeAt(sepPos.at(k - 1) + 1));
                        }
                      } else {
                        matchVals.push_back(new ast::ExpressionMatchValue(
                            parseExpression(preCtx, None, sepPos.at(k - 1), sepPos.at(k)).first));
                      }
                    }
                    if (isNext(TokenType::child, sepPos.back())) {
                      if (isNext(TokenType::identifier, sepPos.back() + 1)) {
                        if (isNext(TokenType::givenTypeSeparator, sepPos.back() + 2)) {
                          matchVals.push_back(new ast::ChoiceMatchValue(IdentifierAt(sepPos.back() + 2)));
                        } else {
                          Error("Unexpected token found after the choice value",
                                RangeSpan(sepPos.back(), sepPos.back() + 1));
                        }
                      } else {
                        Error("Expected name of the variant of the choice type to match", RangeAt(sepPos.back() + 1));
                      }
                    } else {
                      matchVals.push_back(
                          new ast::ExpressionMatchValue(parseExpression(preCtx, None, sepPos.back(), valueEnd).first));
                    }
                  } else {
                    matchVals.push_back(
                        new ast::ExpressionMatchValue(parseExpression(preCtx, None, j + 1, valueEnd).first));
                  }
                  i = valueEnd.value();
                  break;
                } else {
                  Error(
                      "Using normal expressions as cases for matching choice types requires the list of values to end with ->",
                      RangeAt(j + 1));
                }
              }
            }
          } else {
            i++;
          }
          if (isNext(TokenType::givenTypeSeparator, i)) {
            if (!isNext(TokenType::bracketOpen, i + 1)) {
              Error("Expected [ after -> to start the block that contains the sentences to be executed for this case",
                    RangeAt(i + 1));
            }
            i++;
            auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
            if (bCloseRes) {
              auto bClose = bCloseRes.value();
              auto snts   = parseSentences(preCtx, i + 1, bClose);
              chain.push_back(Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>(matchVals, std::move(snts)));
              i = bClose;
            } else {
              Error("Expected end for [", RangeAt(i + 1));
            }
          } else {
            Error("Expected -> before the block that contains sentences to be executed for this case",
                  RangeSpan(start, i));
          }
        } else {
          Error("Expected name of the variant of the choice type to match", RangeAt(i));
        }
        break;
      }
      case TokenType::Else: {
        auto start = i;
        if (elseCase.has_value()) {
          Error("Else case for match sentence is already provided. Please check "
                "logic and make neceassary changes",
                RangeAt(i));
        }
        if (isNext(TokenType::bracketOpen, i)) {
          auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
          if (bCloseRes.has_value()) {
            auto snts = parseSentences(preCtx, i + 1, bCloseRes.value());
            elseCase =
                Pair<Vec<ast::Sentence*>, FileRange>{std::move(snts), FileRange RangeSpan(start, bCloseRes.value())};
            i = bCloseRes.value();
            if (i + 1 != upto) {
              Warning(
                  "Expected match sentence to end after the else case. Make sure that the else case is the last branch in a match sentence",
                  RangeSpan(i + 1, bCloseRes.value()));
            }
          } else {
            Error("Expected end for [", RangeAt(i + 1));
          }
        } else if (isNext(TokenType::givenTypeSeparator, i)) {
          Error("error case for match block doesn't need -> before the block", RangeAt(i));
        } else {
          Error("Expected sentences for the else case in match sentence", RangeAt(i));
        }
        break;
      }
      default: {
        if (isPrimaryWithin(TokenType::givenTypeSeparator, i, upto)) {
          auto                  start       = i;
          auto                  matchValEnd = firstPrimaryPosition(TokenType::givenTypeSeparator, i).value();
          Vec<ast::MatchValue*> matchVals;
          if (isPrimaryWithin(TokenType::separator, i, matchValEnd)) {
            auto sepPoss = primaryPositionsWithin(TokenType::separator, i, matchValEnd);
            matchVals.push_back(
                new ast::ExpressionMatchValue(parseExpression(preCtx, None, i - 1, sepPoss.front()).first));
            for (usize j = 1; j < sepPoss.size(); j++) {
              if (isNext(TokenType::child, sepPoss.at(j - 1))) {
                if (isNext(TokenType::identifier, sepPoss.at(j - 1) + 1)) {
                  if (isNext(TokenType::separator, sepPoss.at(j - 1) + 2)) {
                    matchVals.push_back(new ast::ChoiceMatchValue(IdentifierAt(sepPoss.at(j - 1) + 2)));
                  } else {
                    Error("Unexpected token found after the choice value",
                          RangeSpan(sepPoss.at(j - 1) + 1, sepPoss.at(j - 1) + 2));
                  }
                } else {
                  Error("Expected name of the variant of the choice type to match", RangeAt(sepPoss.at(j - 1) + 1));
                }
              } else if (isNext(TokenType::typeSeparator, sepPoss.at(j - 1))) {
                if (isNext(TokenType::identifier, sepPoss.at(j - 1) + 1)) {
                  if (isNext(TokenType::separator, sepPoss.at(j - 1) + 2)) {
                    matchVals.push_back(new ast::MixMatchValue(IdentifierAt(sepPoss.at(j - 1) + 2), None, false));
                  } else if (isNext(TokenType::parenthesisOpen, sepPoss.at(j - 1) + 2)) {
                    Error(
                        "Multiple values are provided to be matched for this case, so the matched value of the mix type subfield variant cannot be retrieved for use",
                        RangeSpan(sepPoss.at(j - 1) + 1, sepPoss.at(j - 1) + 2));
                  } else {
                    Error("Unexpected token found after mix subfield name",
                          RangeSpan(sepPoss.at(j - 1) + 1, sepPoss.at(j - 1) + 2));
                  }
                } else {
                  Error("Expected name for the subfield of the mix type to match", RangeAt(sepPoss.at(j - 1) + 1));
                }
              } else {
                matchVals.push_back(new ast::ExpressionMatchValue(
                    parseExpression(preCtx, None, sepPoss.at(j - 1), sepPoss.at(j)).first));
              }
            }
            if (isNext(TokenType::child, sepPoss.back())) {
              if (isNext(TokenType::identifier, sepPoss.back() + 1)) {
                if (isNext(TokenType::givenTypeSeparator, sepPoss.back() + 2)) {
                  matchVals.push_back(new ast::ChoiceMatchValue(IdentifierAt(sepPoss.back() + 2)));
                } else {
                  Error("Unexpected token found after the choice value", RangeSpan(sepPoss.back(), sepPoss.back() + 1));
                }
              } else {
                Error("Expected name of the variant of the choice type to match", RangeAt(sepPoss.back() + 1));
              }
            } else if (isNext(TokenType::typeSeparator, sepPoss.back())) {
              if (isNext(TokenType::identifier, sepPoss.back() + 1)) {
                if (isNext(TokenType::givenTypeSeparator, sepPoss.back() + 2)) {
                  matchVals.push_back(new ast::MixMatchValue(IdentifierAt(sepPoss.back() + 2), None, false));
                } else {
                  Error("Unexpected token found after the choice value", RangeSpan(sepPoss.back(), sepPoss.back() + 1));
                }
              } else {
                Error("Expected name for the subfield of the mix type to match", RangeAt(sepPoss.back() + 1));
              }
            } else {
              matchVals.push_back(
                  new ast::ExpressionMatchValue(parseExpression(preCtx, None, sepPoss.back(), matchValEnd).first));
            }
          } else {
            matchVals.push_back(new ast::ExpressionMatchValue(parseExpression(preCtx, None, i - 1, matchValEnd).first));
          }
          i = matchValEnd;
          if (isNext(TokenType::bracketOpen, i)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
            if (bCloseRes.has_value()) {
              chain.push_back(Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>(
                  matchVals, parseSentences(preCtx, i + 1, bCloseRes.value())));
              i = bCloseRes.value();
            } else {
              Error("Expected end for [", RangeAt(i + 1));
            }
          } else {
            Error("Expected [ to start the case block that contains sentences to be executed for this case block",
                  RangeSpan(start, i));
          }
          break;
        } else {
          Error(
              "You forgot to add -> at the end of the expression/list of expressions to match to, which is expected if expressions are used or if there are multiple values to match to",
              token.fileRange);
        }
      }
    }
  }
}

ast::PlainInitialiser* Parser::parsePlainInitialiser(ParserContext& preCtx, ast::QatType* type, usize from,
                                                     usize upto) {
  using lexer::TokenType;
  if (isPrimaryWithin(TokenType::associatedAssignment, from, upto)) {
    Vec<Pair<String, FileRange>> fields;
    Vec<ast::Expression*>        fieldValues;
    for (usize j = from + 1; j < upto; j++) {
      if (isNext(TokenType::identifier, j - 1)) {
        fields.push_back(Pair<String, FileRange>(ValueAt(j), RangeAt(j)));
        if (isNext(TokenType::associatedAssignment, j)) {
          if (isPrimaryWithin(TokenType::separator, j + 1, upto)) {
            auto sepRes = firstPrimaryPosition(TokenType::separator, j + 1);
            if (sepRes) {
              auto sep = sepRes.value();
              if (sep == j + 2) {
                Error("No expression for the member found", RangeSpan(j + 1, j + 2));
              }
              fieldValues.push_back(parseExpression(preCtx, None, j + 1, sep).first);
              j = sep;
              continue;
            } else {
              Error("Expected ,", RangeSpan(j + 1, upto));
            }
          } else {
            if (upto == j + 2) {
              Error("No expression for the member found", RangeSpan(j + 1, j + 2));
            }
            fieldValues.push_back(parseExpression(preCtx, None, j + 1, upto).first);
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
    return new ast::PlainInitialiser(type, fields, fieldValues, {type->fileRange, RangeAt(upto)});
  } else {
    auto exps = parseSeparatedExpressions(preCtx, from, upto);
    return new ast::PlainInitialiser(type, {}, exps, {type->fileRange, RangeAt(upto)});
  }
}

Pair<ast::Expression*, usize> Parser::parseExpression(ParserContext&            preCtx, // NOLINT(misc-no-recursion)
                                                      const Maybe<CacheSymbol>& _symbol, usize from, Maybe<usize> upto,
                                                      Maybe<ast::Expression*> _cachedExps) {
  using ast::Expression;
  using lexer::Token;
  using lexer::TokenType;

  SHOW("\n	CachedSymbol : " << (_symbol.has_value() ? "true" : "false"))
  if (_symbol.has_value()) {
    SHOW("			" << Identifier::fullName(_symbol.value().name).value)
  }
  SHOW("	CachedExpression : " << (_cachedExps.has_value() ? "true" : "false"))
  if (_cachedExps.has_value()) {
    SHOW("			" << ((_cachedExps.value())->toJson()["nodeType"].asString()))
  }
  SHOW("	FROM : " << from << " range: " << RangeAt(from).start.line << ":" << RangeAt(from).start.character << " - "
                   << RangeAt(from).end.line << ":" << RangeAt(from).end.character)
  SHOW("	UPTO : " << (upto.has_value() ? "true" : "false")
                   << (upto.has_value() ? "\n		" + std::to_string(upto.value()) : ""))

  usize i = from + 1; // NOLINT(readability-identifier-length)

  Maybe<CacheSymbol> _cachedSymbol_(_symbol);
  auto               hasCachedSymbol     = [&]() { return _cachedSymbol_.has_value(); };
  auto               consumeCachedSymbol = [&]() {
    if (_cachedSymbol_.has_value()) {
      auto res       = _cachedSymbol_.value();
      _cachedSymbol_ = None;
      return res;
    } else {
      Error("Internal error : No cached symbol found, but it is requested here", RangeAt(i));
    }
  };
  auto setSymbol = [&](const CacheSymbol& other) {
    if (_cachedSymbol_.has_value()) {
      Error("Internal error : A cached symbol is already present, but an attempt to set another symbol was made",
            RangeAt(i));
    } else {
      _cachedSymbol_ = other;
    }
  };

  Maybe<Expression*> _cachedExpressions_(_cachedExps);
  auto               hasCachedExpr     = [&]() { return _cachedExpressions_.has_value(); };
  auto               consumeCachedExpr = [&]() {
    if (_cachedExpressions_.has_value()) {
      auto* res           = _cachedExpressions_.value();
      _cachedExpressions_ = None;
      return res;
    } else {
      Error("Internal error : No cached expression found, but it is requested here", RangeAt(i));
    }
  };
  auto setCachedExpr = [&](ast::Expression* other) {
    if (_cachedExpressions_.has_value()) {
      Error(
          "Internal error : A cached expression is already present, but there was an attempt to set another expression",
          RangeAt(i));
    } else {
      _cachedExpressions_ = other;
    }
  };

  Maybe<Pair<Expression*, Token>> binaryOps;

  for (; upto.has_value() ? (i < upto.value()) : (i < tokens->size()); i++) {
    Token& token = tokens->at(i);
    switch (token.type) {
      case TokenType::Type: {
        if (isNext(TokenType::child, i)) {
          if (isNext(TokenType::identifier, i + 1)) {
            if (isNext(TokenType::parenthesisOpen, i + 2)) {
              auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 3, false);
              if (pCloseRes) {
                auto pClose = pCloseRes.value();
                auto types  = parseSeparatedTypes(preCtx, i + 3, pClose);
                setCachedExpr(new ast::TypeChecker(ValueAt(i + 2), types, RangeSpan(i, pClose)));
                i = pClose;
              } else {
                Error("Expected end for (", RangeAt(i + 3));
              }
            } else {
              Error("Expected ( to start the argument for the type checker", RangeSpan(i, i + 2));
            }
          } else {
            Error("Invalid expression", RangeSpan(i, i + 1));
          }
        } else {
          Error("Invalid expression", RangeAt(i));
        }
        break;
      }
      case TokenType::unsignedLiteral: {
        if ((token.value.find('_') != String::npos) && (token.value.find('u') != (token.value.length() - 1))) {
          u64    bits  = 32; // NOLINT(readability-magic-numbers)
          auto   split = token.value.find('_');
          String number(token.value.substr(0, split));
          if ((split + 2) < token.value.length()) {
            if (token.value.substr(split + 1) == "usize") {
              bits = (sizeof(usize) * 8u); // NOLINT(readability-magic-numbers)
            } else {
              bits = std::stoul(token.value.substr(split + 2));
            }
          }
          setCachedExpr(new ast::CustomIntegerLiteral(number, true, bits, token.fileRange));
        } else {
          setCachedExpr(new ast::UnsignedLiteral(
              (token.value.find('_') != String::npos) ? token.value.substr(0, token.value.find('_')) : token.value,
              token.fileRange));
        }
        break;
      }
      case TokenType::integerLiteral: {
        if ((token.value.find('_') != String::npos) && (token.value.find('i') != (token.value.length() - 1))) {
          u64    bits  = 32; // NOLINT(readability-magic-numbers)
          auto   split = token.value.find('_');
          String number(token.value.substr(0, split));
          if ((split + 2) < token.value.length()) {
            bits = std::stoul(token.value.substr(split + 2));
          }
          setCachedExpr(new ast::CustomIntegerLiteral(number, false, bits, token.fileRange));
        } else {
          setCachedExpr(new ast::IntegerLiteral(
              (token.value.find('_') != String::npos) ? token.value.substr(0, token.value.find('_')) : token.value,
              token.fileRange));
        }
        break;
      }
      case TokenType::floatLiteral: {
        auto number = token.value;
        if (number.find('_') != String::npos) {
          SHOW("Found custom float literal: " << number.substr(number.find('_') + 1))
          setCachedExpr(new ast::CustomFloatLiteral(number.substr(0, number.find('_')),
                                                    number.substr(number.find('_') + 1), token.fileRange));
        } else {
          SHOW("Found float literal")
          setCachedExpr(new ast::FloatLiteral(token.value, token.fileRange));
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
            fRange = FileRange(fRange, RangeAt(pos + 1));
            pos++;
          }
          i = pos;
        }
        setCachedExpr(new ast::StringLiteral(val, fRange));
        break;
      }
      case TokenType::none: {
        ast::QatType* noneType = nullptr;
        auto          range    = RangeAt(i);
        if (isNext(TokenType::genericTypeStart, i)) {
          auto endRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1, false);
          if (endRes.has_value()) {
            auto typeRes = parseType(preCtx, i + 1, endRes.value());
            if (typeRes.second + 1 != endRes.value()) {
              Error("Unexpected end for the type of the none expression", RangeSpan(i, typeRes.second));
            } else {
              noneType = typeRes.first;
              range    = RangeSpan(i, endRes.value());
              i        = endRes.value();
            }
          } else {
            Error("Expected end for generic type specification", RangeAt(i + 1));
          }
        }
        setCachedExpr(new ast::NoneExpression(noneType, range));
        break;
      }
      case TokenType::sizeOf: {
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto* type = parseType(preCtx, i + 1, pCloseRes.value()).first;
            setCachedExpr(new ast::SizeOfType(type, RangeSpan(i, pCloseRes.value())));
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
        setCachedExpr(new ast::NullPointer(token.fileRange));
        break;
      }
      case TokenType::FALSE:
      case TokenType::TRUE: {
        setCachedExpr(new ast::BooleanLiteral(token.type == TokenType::TRUE, RangeAt(i)));
        break;
      }
      case TokenType::super:
      case TokenType::identifier: {
        auto symbolRes = parseSymbol(preCtx, i);
        setSymbol(symbolRes.first);
        i = symbolRes.second;
        break;
      }
      case TokenType::Default: {
        setCachedExpr(new ast::Default(RangeAt(i)));
        break;
      }
      case TokenType::from: {
        if (hasCachedSymbol()) {
          if (isNext(TokenType::parenthesisOpen, i)) {
            auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
            if (pCloseRes.has_value()) {
              auto pClose = pCloseRes.value();
              auto exps   = parseSeparatedExpressions(preCtx, i + 1, pClose);
              auto symbol = consumeCachedSymbol();
              setCachedExpr(
                  new ast::ConstructorCall(new ast::NamedType(symbol.relative, symbol.name, false, symbol.fileRange),
                                           exps, None, None, None, {symbol.fileRange, RangeAt(pClose)}));
              i = pClose;
            } else {
              Error("Expected end for (", RangeAt(i + 1));
            }
          }
        }
        break;
      }
      case TokenType::genericTypeStart: {
        SHOW("Generic type start: " << i)
        if (hasCachedSymbol()) {
          auto endRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i, false);
          if (endRes.has_value()) {
            auto end = endRes.value();
            SHOW("Generic type end: " << end)
            auto types = parseGenericFill(preCtx, i, end);
            if (isNext(TokenType::curlybraceOpen, end)) {
              auto cEnd = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, end + 1, false);
              if (cEnd.has_value()) {
                auto symbol = consumeCachedSymbol();
                setCachedExpr(parsePlainInitialiser(
                    preCtx, new ast::GenericNamedType(symbol.relative, symbol.name, types, false, symbol.fileRange),
                    end + 1, cEnd.value()));
                i = cEnd.value();
              } else {
                Error("Expected end for { in plain initialisation", RangeAt(end + 1));
              }
            } else if (isNext(TokenType::from, end)) {
              SHOW("Found from")
              i = end + 1;
              if (isNext(TokenType::parenthesisOpen, i)) {
                auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
                if (pCloseRes.has_value()) {
                  auto pClose = pCloseRes.value();
                  auto exps   = parseSeparatedExpressions(preCtx, i + 1, pClose);
                  auto symbol = consumeCachedSymbol();
                  setCachedExpr(new ast::ConstructorCall(
                      new ast::GenericNamedType(symbol.relative, symbol.name, types, false, symbol.fileRange), exps,
                      None, None, None, {symbol.fileRange, RangeAt(pClose)}));
                  i = pClose;
                } else {
                  Error("Expected end for (", RangeAt(i + 1));
                }
              } else {
                Error("Expected ( to start the constructor call", RangeAt(end + 1));
              }
            } else {
              auto symbol = consumeCachedSymbol();
              setCachedExpr(
                  new ast::GenericEntity(symbol.relative, symbol.name, types, {symbol.fileRange, RangeAt(end)}));
              i = end;
            }
          } else {
            Error("Expected end for generic type specification", RangeAt(i));
          }
        } else {
          Error("Expected identifier or symbol before generic type specification", RangeAt(i));
        }
        break;
      }
      case TokenType::self: {
        if (isNext(TokenType::identifier, i)) {
          if (isNext(TokenType::parenthesisOpen, i + 1)) {
            // FIXME - Self member function call
          } else {
            setCachedExpr(new ast::SelfMember(IdentifierAt(i + 1), RangeSpan(i, i + 1)));
            i++;
          }
        } else {
          SHOW("Creating self in AST")
          setCachedExpr(new ast::Self(RangeAt(i)));
        }
        break;
      }
      case TokenType::heap: {
        if (isNext(TokenType::child, i)) {
          if (isNext(TokenType::identifier, i + 1)) {
            if (ValueAt(i + 2) == "get") {
              if (isNext(TokenType::genericTypeStart, i + 2)) {
                auto tEndRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 3, false);
                if (tEndRes.has_value()) {
                  auto  tEnd    = tEndRes.value();
                  auto  typeRes = parseType(preCtx, i + 3, tEnd);
                  auto* type    = typeRes.first;
                  if (typeRes.second != tEnd - 1) {
                    Error("Invalid type for heap'get", RangeSpan(i, typeRes.second));
                  }
                  if (isNext(TokenType::parenthesisOpen, tEnd)) {
                    auto pCloseRes =
                        getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, tEnd + 1, false);
                    if (pCloseRes.has_value()) {
                      auto  pClose = pCloseRes.value();
                      auto* exp    = parseExpression(preCtx, None, tEnd + 1, pClose).first;
                      setCachedExpr(new ast::HeapGet(type, exp, RangeAt(i)));
                      i = pClose;
                      break;
                    } else {
                      Error("Expected end for (", RangeAt(tEnd + 1));
                    }
                  } else {
                    setCachedExpr(new ast::HeapGet(type, nullptr, RangeAt(i)));
                    i = tEnd;
                    break;
                  }
                } else {
                  Error("Expected end for generic type specification", RangeAt(i + 3));
                }
              } else {
                Error("Expected generic type specification for the type to allocate the memory for",
                      RangeSpan(i, i + 2));
              }
            } else if (ValueAt(i + 2) == "put") {
              if (isNext(TokenType::parenthesisOpen, i + 2)) {
                auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 3, false);
                if (pCloseRes.has_value()) {
                  auto* exp = parseExpression(preCtx, None, i + 3, pCloseRes.value()).first;
                  setCachedExpr(new ast::HeapPut(exp, RangeSpan(i, pCloseRes.value())));
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
              if (isNext(TokenType::genericTypeStart, i + 2)) {
                auto tEndRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 3, false);
                if (tEndRes.has_value()) {
                  auto  tEnd = tEndRes.value();
                  auto* type = parseType(preCtx, i + 3, tEnd).first;
                  if (isNext(TokenType::parenthesisOpen, tEnd)) {
                    auto pCloseRes =
                        getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, tEnd + 1, false);
                    if (pCloseRes.has_value()) {
                      auto pClose = pCloseRes.value();
                      if (isPrimaryWithin(TokenType::separator, tEnd + 1, pClose)) {
                        auto  split = firstPrimaryPosition(TokenType::separator, tEnd + 1).value();
                        auto* exp   = parseExpression(preCtx, None, tEnd + 1, split).first;
                        auto* count = parseExpression(preCtx, None, split, pClose).first;
                        setCachedExpr(new ast::HeapGrow(type, exp, count, RangeAt(i)));
                        i = pClose;
                      } else {
                        Error("Expected 2 argument values for heap'grow", RangeSpan(tEnd + 1, pClose));
                      }
                      break;
                    } else {
                      Error("Expected end for (", RangeAt(tEnd + 1));
                    }
                  } else {
                    Error("Expected arguments for heap'grow", RangeSpan(i, tEnd));
                  }
                } else {
                  Error("Expected end for the generic type specification", RangeAt(i + 3));
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
        if (hasCachedSymbol()) {
          auto cCloseRes = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i, false);
          if (cCloseRes.has_value()) {
            auto symbol = consumeCachedSymbol();
            setCachedExpr(
                parsePlainInitialiser(preCtx, new ast::NamedType(symbol.relative, symbol.name, false, symbol.fileRange),
                                      i, cCloseRes.value()));
            i = cCloseRes.value();
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
        auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
        if (bCloseRes) {
          if (hasCachedSymbol()) {
            auto symbol = consumeCachedSymbol();
            setCachedExpr(new ast::Entity(symbol.relative, symbol.name, symbol.fileRange));
          }
          if (binaryOps || (!hasCachedExpr())) {
            auto bClose = bCloseRes.value();
            if (bClose == i + 1) {
              // Empty array literal
              setCachedExpr(new ast::ArrayLiteral({}, RangeSpan(i, bClose)));
              i = bClose;
            } else {
              auto vals = parseSeparatedExpressions(preCtx, i, bClose);
              setCachedExpr(new ast::ArrayLiteral(vals, RangeSpan(i, bClose)));
              i = bClose;
            }
            break;
          } else if (hasCachedExpr()) {
            auto* exp = parseExpression(preCtx, None, i, bCloseRes.value()).first;
            auto* ent = consumeCachedExpr();
            setCachedExpr(new ast::IndexAccess(ent, exp, {ent->fileRange, exp->fileRange}));
            i = bCloseRes.value();
          }
        } else {
          Error("Invalid end for [", token.fileRange);
        }
        break;
      }
      case TokenType::parenthesisOpen: {
        SHOW("Found paranthesis")
        if (!binaryOps && (hasCachedExpr() || hasCachedSymbol())) {
          // This parenthesis is supposed to indicate a function call
          auto p_close = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i, false);
          if (p_close.has_value()) {
            SHOW("Found end of paranthesis")
            if (upto.has_value() && (p_close.value() >= upto)) {
              Error("Invalid position of )", token.fileRange);
            } else {
              SHOW("About to parse arguments")
              Vec<ast::Expression*> args;
              if (isPrimaryWithin(TokenType::separator, i, p_close.value())) {
                args = parseSeparatedExpressions(preCtx, i, p_close.value());
              } else if (i < (p_close.value() - 1)) {
                args.push_back(parseExpression(preCtx, None, i, p_close.value()).first);
              }
              if (!hasCachedExpr()) {
                SHOW("Expressions cache is empty")
                if (hasCachedSymbol()) {
                  auto  symbol = consumeCachedSymbol();
                  auto* ent    = new ast::Entity(symbol.relative, symbol.name, symbol.fileRange);
                  setCachedExpr(new ast::FunctionCall(ent, args, symbol.extend_fileRange(RangeAt(p_close.value()))));
                } else {
                  Error("No expression found to be passed to the function call. "
                        "And no function name found for "
                        "the static function call",
                        {token.fileRange, RangeAt(p_close.value())});
                }
              } else {
                auto* expression = consumeCachedExpr();
                setCachedExpr(new ast::FunctionCall(expression, args,
                                                    FileRange(expression->fileRange, RangeAt(p_close.value()))));
              }
              i = p_close.value();
            }
          } else {
            Error("Expected ) to close the scope started by this opening "
                  "paranthesis",
                  token.fileRange);
          }
        } else {
          auto p_close_res = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i, false);
          if (!p_close_res.has_value()) {
            Error("Expected )", token.fileRange);
          }
          if (upto.has_value() && (p_close_res.value() >= upto)) {
            Error("Invalid position of )", token.fileRange);
          }
          auto p_close = p_close_res.value();
          if (isPrimaryWithin(TokenType::semiColon, i, p_close)) {
            Vec<ast::Expression*> values;
            auto                  separations = primaryPositionsWithin(TokenType::semiColon, i, p_close);
            values.push_back(parseExpression(preCtx, None, i, separations.front()).first);
            bool shouldContinue = true;
            if (separations.size() == 1) {
              SHOW("Only 1 separation")
              shouldContinue = (separations.at(0) != (p_close - 1));
              SHOW("Found condition")
            }
            if (shouldContinue) {
              for (usize j = 0; j < (separations.size() - 1); j += 1) {
                values.push_back(parseExpression(preCtx, None, separations.at(j), separations.at(j + 1)).first);
              }
              values.push_back(parseExpression(preCtx, None, separations.back(), p_close).first);
            }
            setCachedExpr(new ast::TupleValue(values, FileRange(token.fileRange, RangeAt(p_close))));
            i = p_close;
          } else {
            setCachedExpr(parseExpression(preCtx, None, i, p_close).first);
            i = p_close;
          }
        }
        break;
      }
      case TokenType::loop: {
        if (isNext(TokenType::child, i)) {
          if (isNext(TokenType::identifier, i + 1)) {
            setCachedExpr(new ast::LoopIndex((ValueAt(i + 2) == "index") ? "" : ValueAt(i + 2), RangeSpan(i, i + 2)));
            i = i + 2;
          } else {
            Error("Unexpected token after loop'", {token.fileRange, RangeAt(i + 1)});
          }
        } else {
          Error("Invalid token after loop", token.fileRange);
        }
        break;
      }
      case TokenType::move: {
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes) {
            auto  pClose = pCloseRes.value();
            auto* exp    = parseExpression(preCtx, None, i + 1, pClose).first;
            setCachedExpr(new ast::Move(exp, RangeSpan(i, pClose)));
            i = pClose;
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        } else {
          Error("Expected ( to start the expression to move", RangeAt(i));
        }
        break;
      }
      case TokenType::copy: {
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes) {
            auto  pClose = pCloseRes.value();
            auto* exp    = parseExpression(preCtx, None, i + 1, pClose).first;
            setCachedExpr(new ast::Copy(exp, RangeSpan(i, pClose)));
            i = pClose;
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        } else {
          Error("Expected ( to start the expression to copy", RangeAt(i));
        }
        break;
      }
      case TokenType::unaryOperator: {
        if (ValueAt(i) == "!" || ValueAt(i) == "-") {
          auto exps = parseExpression(preCtx, None, i, None);
          if (ValueAt(i) == "!") {
            setCachedExpr(new ast::Not(exps.first, RangeSpan(i, exps.second)));
          } else {
            // FIXME - Handle minus operator
          }
          i = exps.second;
        } else {
        }
        break;
      }
      case TokenType::own: {
        SHOW("Found own")
        auto                 start = i;
        auto                 ownTy = ast::ConstructorCall::OwnType::parent;
        Maybe<ast::QatType*> ownerType;
        Maybe<Expression*>   ownCount;
        if (isNext(TokenType::child, i)) {
          SHOW("Custom own type")
          if (isNext(TokenType::heap, i + 1)) {
            ownTy = ast::ConstructorCall::OwnType::heap;
            i += 2;
          } else if (isNext(TokenType::Type, i + 1)) {
            ownTy = ast::ConstructorCall::OwnType::type;
            i += 2;
          } else if (isNext(TokenType::region, i + 1)) {
            ownTy = ast::ConstructorCall::OwnType::region;
            if (isNext(TokenType::parenthesisOpen, i + 2)) {
              auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 3, false);
              if (pCloseRes.has_value()) {
                ownerType = parseType(preCtx, i + 3, pCloseRes.value()).first;
                i         = pCloseRes.value();
              } else {
                Error("Expected end for (", RangeAt(i + 3));
              }
            } else {
              Error("Expected ( and ) to enclose the region for ownership", RangeAt(i + 3));
            }
          } else {
            Error("Unexpected ownership type", RangeSpan(i, i + 1));
          }
        }
        if (isNext(TokenType::bracketOpen, i)) {
          SHOW("Multiple owns")
          auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
          if (bCloseRes.has_value()) {
            SHOW("Own count ends at: " << bCloseRes.value())
            ownCount = parseExpression(preCtx, None, i + 1, bCloseRes.value()).first;
            i        = bCloseRes.value();
          } else {
            Error("Expected end for [", RangeAt(i + 1));
          }
        }
        // FIXME - Add heaped plain initialisation
        auto symRes = parseSymbol(preCtx, i + 1);
        setSymbol(symRes.first);
        i                  = symRes.second;
        ast::QatType* type = nullptr;
        if (isNext(TokenType::genericTypeStart, i)) {
          auto tEndRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1, false);
          if (tEndRes.has_value()) {
            auto                   tEnd = tEndRes.value();
            Vec<ast::FillGeneric*> types;
            if (tEnd != i + 2) {
              types = parseGenericFill(preCtx, i + 1, tEnd);
            }
            auto symbol = consumeCachedSymbol();
            type        = new ast::GenericNamedType(symbol.relative, symbol.name, types, false, symbol.fileRange);
            i           = tEnd;
          } else {
            Error("Expected end for the generic type specification", RangeAt(i + 1));
          }
        } else {
          auto symbol = consumeCachedSymbol();
          type        = new ast::NamedType(symbol.relative, symbol.name, false, symbol.fileRange);
        }
        if (isNext(TokenType::from, i)) {
          if (isNext(TokenType::parenthesisOpen, i + 1)) {
            auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2, false);
            if (pCloseRes) {
              auto pClose = pCloseRes.value();
              auto args   = parseSeparatedExpressions(preCtx, i + 2, pClose);
              setCachedExpr(new ast::ConstructorCall(type, args, ownTy, ownerType, ownCount, RangeSpan(start, pClose)));
              i = pClose;
            } else {
              Error("Expected end for (", RangeAt(i + 2));
            }
          }
        }
        break;
      }
      case TokenType::pointerType: {
        if (hasCachedExpr() || hasCachedSymbol()) {
          if (hasCachedSymbol()) {
            if (hasCachedExpr()) {
              Error("Compiler internal error : Both a cached expression and a cached symbol found", RangeAt(i));
            } else {
              auto symbol = consumeCachedSymbol();
              new ast::Entity(symbol.relative, symbol.name, symbol.fileRange);
            }
          }
          auto* exp = consumeCachedExpr();
          setCachedExpr(new ast::Dereference(exp, {exp->fileRange, RangeAt(i)}));
        } else {
          Error("No expression found before pointer dereference operator", RangeAt(i));
        }
        break;
      }
      case TokenType::Await: {
        auto expRes = parseExpression(preCtx, None, i, None);
        setCachedExpr(new ast::Await(expRes.first, RangeSpan(i, expRes.second)));
        i = expRes.second;
        break;
      }
      case TokenType::binaryOperator: {
        if (binaryOps) {
          Error("A binary expression already found before this, without having an RHS expression",
                {binaryOps->first->fileRange, RangeAt(i)});
        }
        SHOW("Binary operator found: " << token.value)
        if (!hasCachedExpr() && !hasCachedSymbol()) {
          Error("No expression found on the left side of the binary operator " + token.value, token.fileRange);
        } else if (hasCachedSymbol()) {
          auto symbol = consumeCachedSymbol();
          binaryOps =
              Pair<ast::Expression*, Token>(new ast::Entity(symbol.relative, symbol.name, symbol.fileRange), token);
        } else {
          binaryOps = Pair<ast::Expression*, Token>(consumeCachedExpr(), token);
        }
        break;
      }
      case TokenType::stringSliceType: {
        if (isNext(TokenType::curlybraceOpen, i)) {
          auto pCloseRes = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto pClose = pCloseRes.value();
            auto args   = parseSeparatedExpressions(preCtx, i + 1, pClose);
            setCachedExpr(
                new ast::PlainInitialiser(new ast::StringSliceType(false, RangeAt(i)), {}, args, RangeSpan(i, pClose)));
            i = pClose;
          } else {
            Error("Expected end for {", RangeAt(i + 2));
          }
        } else {
          Error("Expected { to start the arguments for creating a string slice", RangeSpan(i, i + 1));
        }
        break;
      }
      case TokenType::to: {
        SHOW("Found to expression")
        ast::Expression* source = nullptr;
        if (!hasCachedExpr()) {
          if (hasCachedSymbol()) {
            auto symbol = consumeCachedSymbol();
            setCachedExpr(new ast::Entity(symbol.relative, symbol.name, symbol.fileRange));
          }
        } else {
          source = consumeCachedExpr();
        }
        if (source) {
          auto destTyRes = parseType(preCtx, i, upto);
          setCachedExpr(
              new ast::ToConversion(source, destTyRes.first, FileRange(source->fileRange, RangeAt(destTyRes.second))));
          i = destTyRes.second;
        } else {
          Error("No expression found for conversion", token.fileRange);
        }
        break;
      }
      case TokenType::ternary: {
        ast::Expression* cond = nullptr;
        if (!hasCachedExpr()) {
          if (hasCachedSymbol()) {
            auto symbol = consumeCachedSymbol();
            setCachedExpr(new ast::Entity(symbol.relative, symbol.name, symbol.fileRange));
          } else {
            Error("No expression found before ternary operator", RangeAt(i));
          }
        } else {
          cond = consumeCachedExpr();
        }
        if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto pClose = pCloseRes.value();
            if (isPrimaryWithin(TokenType::separator, i + 1, pClose)) {
              auto splitRes = firstPrimaryPosition(TokenType::separator, i + 1);
              if (splitRes.has_value()) {
                auto  split     = splitRes.value();
                auto* trueExpr  = parseExpression(preCtx, None, i + 1, split).first;
                auto* falseExpr = parseExpression(preCtx, None, split, pClose).first;
                setCachedExpr(
                    new ast::TernaryExpression(cond, trueExpr, falseExpr, {cond->fileRange, RangeAt(pClose)}));
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
        if (hasCachedExpr() || hasCachedSymbol()) {
          if ((!hasCachedExpr()) && hasCachedSymbol()) {
            auto symbol = consumeCachedSymbol();
            setCachedExpr(new ast::Entity(symbol.relative, symbol.name, symbol.fileRange));
          }
          auto* exp = consumeCachedExpr();
          if (isNext(TokenType::identifier, i)) {
            if (isNext(TokenType::parenthesisOpen, i + 1)) {
              auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2, false);
              if (pCloseRes.has_value()) {
                auto pClose = pCloseRes.value();
                auto args   = parseSeparatedExpressions(preCtx, i + 2, pClose);
                setCachedExpr(
                    new ast::MemberFunctionCall(exp, ValueAt(i + 1), args, true, {exp->fileRange, RangeAt(pClose)}));
                i = pClose;
                break;
              } else {
                Error("Expected end for (", RangeAt(i + 2));
              }
            } else {
              Error("Expected ( to start variation function call", RangeSpan(i, i + 1));
            }
          }
        }
        break;
      }
      case TokenType::typeSeparator: {
        if (hasCachedSymbol()) {
          auto  symbol = consumeCachedSymbol();
          auto* typ    = new ast::NamedType(symbol.relative, symbol.name, false, symbol.fileRange);
          if (isNext(TokenType::identifier, i)) {
            auto& subName = ValueAt(i + 1);
            if (isNext(TokenType::parenthesisOpen, i + 1)) {
              auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2, false);
              if (pCloseRes) {
                auto  pClose = pCloseRes.value();
                auto* exp    = parseExpression(preCtx, None, i + 2, pClose).first;
                setCachedExpr(new ast::MixTypeInitialiser(typ, subName, exp, RangeSpan(i, pClose)));
                i = pClose;
              } else {
                Error("Expected end for (", RangeAt(i + 2));
              }
            } else {
              setCachedExpr(new ast::MixTypeInitialiser(typ, subName, None, {symbol.fileRange, RangeAt(i + 1)}));
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
        if (hasCachedExpr() || hasCachedSymbol()) {
          if ((!hasCachedExpr()) && hasCachedSymbol()) {
            auto symbol = consumeCachedSymbol();
            setCachedExpr(new ast::Entity(symbol.relative, symbol.name, symbol.fileRange));
          } else if (hasCachedExpr() && hasCachedSymbol()) {
            Error("Cached expressions are not empty and also found symbol", consumeCachedExpr()->fileRange);
          }
          auto* exp = consumeCachedExpr();
          if (isNext(TokenType::identifier, i)) {
            // TODO - Support generic member function calls
            if (isNext(TokenType::parenthesisOpen, i + 1)) {
              auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2, false);
              if (pCloseRes.has_value()) {
                auto pClose = pCloseRes.value();
                auto args   = parseSeparatedExpressions(preCtx, i + 2, pClose);
                setCachedExpr(new ast::MemberFunctionCall(exp, ValueAt(i + 1), args, false,
                                                          {exp->fileRange, RangeSpan(i, pClose)}));
                i = pClose;
                break;
              } else {
                Error("Expected end for (", RangeAt(i + 2));
              }
            } else {
              setCachedExpr(new ast::MemberAccess(exp, ValueAt(i + 1), {exp->fileRange, RangeSpan(i, i + 1)}));
              i++;
            }
          } else if (isNext(TokenType::end, i)) {
            if (isNext(TokenType::parenthesisOpen, i + 1)) {
              auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2, false);
              if (pCloseRes) {
                setCachedExpr(new ast::MemberFunctionCall(exp, "end", {}, false,
                                                          {exp->fileRange, RangeSpan(i, pCloseRes.value())}));
                i = pCloseRes.value();
                break;
              } else {
                Error("Expected end for (", RangeAt(i + 2));
              }
            } else {
              Error("Expected ( to start the destructor call", RangeAt(i));
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
          SHOW("Value of i is: " << i << " and value of upto is: " << upto.value())
          if ((hasCachedExpr()) && (upto.value() == i)) {
            return {consumeCachedExpr(), i - 1};
          } else if (hasCachedSymbol() && (upto.value() == i)) {
            auto symbol = consumeCachedSymbol();
            return {new ast::Entity(symbol.relative, symbol.name, symbol.fileRange), i - 1};
          } else {
            Error("Encountered an invalid token in the expression", RangeAt(i));
          }
        } else {
          if (hasCachedExpr()) {
            return {consumeCachedExpr(), i - 1};
          } else if (hasCachedSymbol()) {
            auto symbol = consumeCachedSymbol();
            return {new ast::Entity(symbol.relative, symbol.name, symbol.fileRange), i - 1};
          } else {
            Error("No expression found and encountered invalid token", RangeAt(i));
          }
        }
      }
    }
    if (binaryOps) {
      SHOW("Binary ops are not empty")
      if (hasCachedExpr()) {
        SHOW("Found lhs and rhs of binary exp")
        auto* rhs = consumeCachedExpr();
        setCachedExpr(new ast::BinaryExpression(binaryOps->first, binaryOps->second.value, rhs,
                                                FileRange(binaryOps->first->fileRange, rhs->fileRange)));
        binaryOps = None;
      } else if (hasCachedSymbol()) {
        auto  symbol = consumeCachedSymbol();
        auto* rhs    = new ast::Entity(symbol.relative, symbol.name, symbol.fileRange);
        setCachedExpr(new ast::BinaryExpression(binaryOps->first, binaryOps->second.value, rhs,
                                                FileRange(binaryOps->first->fileRange, rhs->fileRange)));
        binaryOps = None;
      }
    }
  }
  if (binaryOps) {
    SHOW("Binary ops are not empty")
    if (hasCachedExpr()) {
      SHOW("Found lhs and rhs of binary exp")
      auto* rhs = consumeCachedExpr();
      setCachedExpr(new ast::BinaryExpression(binaryOps->first, binaryOps->second.value, rhs,
                                              FileRange(binaryOps->first->fileRange, rhs->fileRange)));
      binaryOps = None;
    } else if (hasCachedSymbol()) {
      auto  symbol = consumeCachedSymbol();
      auto* rhs    = new ast::Entity(symbol.relative, symbol.name, symbol.fileRange);
      setCachedExpr(new ast::BinaryExpression(binaryOps->first, binaryOps->second.value, rhs,
                                              FileRange(binaryOps->first->fileRange, rhs->fileRange)));
      binaryOps = None;
    }
  }
  if (!hasCachedExpr()) {
    if (hasCachedSymbol()) {
      auto symbol = consumeCachedSymbol();
      return {new ast::Entity(symbol.relative, symbol.name, symbol.fileRange), i};
    } else {
      Error("No expression found", RangeAt(from));
    }
  } else if (binaryOps) {
    auto* rhs = consumeCachedExpr();
    return {new ast::BinaryExpression(binaryOps->first, binaryOps->second.value, rhs,
                                      {binaryOps->first->fileRange, rhs->fileRange}),
            i};
  } else {
    return {consumeCachedExpr(), i};
  }
} // NOLINT(clang-diagnostic-return-type)

Vec<ast::Expression*> Parser::parseSeparatedExpressions( // NOLINT(misc-no-recursion),
                                                         // NOLINTNEXTLINE(readability-identifier-length)
    ParserContext& preCtx, usize from, usize to) {
  Vec<ast::Expression*> result;
  for (usize i = from + 1; i < to; i++) {
    if (isPrimaryWithin(lexer::TokenType::separator, i - 1, to)) {
      auto endResult = firstPrimaryPosition(lexer::TokenType::separator, i - 1);
      if (!endResult.has_value() || (endResult.value() >= to)) {
        Error("Invalid position for separator `,`", RangeAt(i));
      }
      auto end = endResult.value();
      result.push_back(parseExpression(preCtx, None, i - 1, end).first);
      i = end;
    } else {
      result.push_back(parseExpression(preCtx, None, i - 1, to).first);
      i = to;
    }
  }
  return result;
}

Vec<ast::QatType*> Parser::parseSeparatedTypes(ParserContext& preCtx, usize from, usize upto) {
  Vec<ast::QatType*> result;
  for (usize i = from + 1; i < upto; i++) {
    if (isPrimaryWithin(lexer::TokenType::separator, i - 1, upto)) {
      auto endResult = firstPrimaryPosition(lexer::TokenType::separator, i - 1);
      if (!endResult.has_value() || (endResult.value() >= upto)) {
        Error("Invalid position for separator `,`", RangeAt(i));
      }
      auto end = endResult.value();
      result.push_back(parseType(preCtx, i - 1, end).first);
      i = end;
    } else {
      result.push_back(parseType(preCtx, i - 1, upto).first);
      i = upto;
    }
  }
  return result;
}

Pair<CacheSymbol, usize> Parser::parseSymbol(ParserContext& preCtx, const usize start) {
  using lexer::TokenType;
  Vec<Identifier> name;
  if (isNext(TokenType::identifier, start - 1) || isNext(TokenType::super, start - 1)) {
    u32  relative = isNext(TokenType::super, start - 1);
    auto prev     = tokens->at(start).type;
    auto i        = start; // NOLINT(readability-identifier-length)
    if (isNext(TokenType::super, start - 1)) {
      while (prev == TokenType::super ? isNext(TokenType::colon, i)
                                      : (prev == TokenType::colon ? isNext(TokenType::super, i) : false)) {
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
          name.push_back(Identifier(tok.value, tok.fileRange));
          prev = TokenType::identifier;
        } else {
          return {CacheSymbol(relative, name, start, RangeAt(start)), i - 1};
        }
      } else if (tok.type == TokenType::colon) {
        if (prev == TokenType::identifier) {
          prev = TokenType::colon;
        } else {
          Error("No identifier found before `:`. Anonymous modules are not allowed in `qat`", tok.fileRange);
        }
      } else {
        break;
      }
    }
    return {CacheSymbol(relative, name, start, RangeSpan(start, i - 1)), i - 1};
  } else {
    // TODO - Change this, after verifying that this situation is never
    // encountered.
    Error("No identifier found for parsing the symbol", RangeAt(start));
  }
} // NOLINT(clang-diagnostic-return-type)

Vec<ast::Sentence*> Parser::parseSentences(ParserContext& preCtx, usize from, usize upto, bool onlyOne) {
  using ast::FloatType;
  using ast::IntegerType;
  using IR::FloatTypeKind;
  using lexer::Token;
  using lexer::TokenType;

  auto                ctx = preCtx;
  Vec<ast::Sentence*> result;

  usize i = from + 1; // NOLINT(readability-identifier-length)

  Maybe<CacheSymbol> _cacheSymbol_;
  auto               hasCachedSymbol = [&]() { return _cacheSymbol_.has_value(); };
  auto               setCachedSymbol = [&](const CacheSymbol& symbol) {
    if (_cacheSymbol_.has_value()) {
      Error("Internal error : A cached symbol is already present, and there was an attempt to cache another symbol",
                          RangeAt(i));
    } else {
      _cacheSymbol_ = symbol;
    }
  };
  auto consumeCachedSymbol = [&]() {
    if (_cacheSymbol_.has_value()) {
      auto sym      = _cacheSymbol_.value();
      _cacheSymbol_ = None;
      return sym;
    } else {
      Error("Internal error : No cached symbol found, but was requested here", RangeAt(i));
    }
  };
  auto retrieveCachedSymbol = [&]() {
    if (_cacheSymbol_.has_value()) {
      auto sym      = _cacheSymbol_;
      _cacheSymbol_ = None;
      return sym;
    } else {
      return _cacheSymbol_;
    }
  };

  Maybe<ast::Expression*> _cachedExpression_;
  auto                    hasCachedExpr = [&]() { return _cachedExpression_.has_value(); };
  auto                    setCachedExpr = [&](ast::Expression* value) {
    if (_cachedExpression_.has_value()) {
      Error(
          "Internal error : A cached expression is already present, and there was an attempt to cache another expression",
          RangeAt(i));
    } else {
      _cachedExpression_ = value;
    }
  };
  auto consumeCachedExpr = [&]() {
    if (_cachedExpression_.has_value()) {
      auto* exp          = _cachedExpression_.value();
      _cachedExpression_ = None;
      return exp;
    } else {
      Error("Internal error : No cached expression is found, but was requested here", RangeAt(i));
    }
  };
  auto retrieveCachedExpr = [&]() {
    if (_cachedExpression_.has_value()) {
      auto expr          = _cachedExpression_;
      _cachedExpression_ = None;
      return expr;
    } else {
      return _cachedExpression_;
    }
  };

  for (; i < upto; i++) {
    Token& token = tokens->at(i);
    switch (token.type) {
      case TokenType::super:
      case TokenType::identifier: {
        SHOW("Identifier encountered")
        if (hasCachedSymbol()) {
          Error("Another symbol found when there already is a symbol", RangeAt(i));
        } else {
          auto symRes = parseSymbol(ctx, i);
          setCachedSymbol(symRes.first);
          i = symRes.second;
          if (isNext(TokenType::genericTypeStart, i)) {
            auto expRes = parseExpression(preCtx, retrieveCachedSymbol(), i, None);
            setCachedExpr(expRes.first);
            i = expRes.second;
          }
        }
        break;
      }
      case TokenType::assignment: {
        SHOW("Assignment found")
        /* Assignment */
        if (hasCachedSymbol()) {
          auto end_res = firstPrimaryPosition(TokenType::stop, i);
          if (!end_res.has_value() || (end_res.value() >= upto)) {
            Error("Invalid end for the sentence", token.fileRange);
          }
          auto  end    = end_res.value();
          auto* exp    = parseExpression(ctx, None, i, end).first;
          auto  symbol = consumeCachedSymbol();
          auto* lhs    = new ast::Entity(symbol.relative, symbol.name, symbol.fileRange);
          result.push_back(new ast::Assignment(lhs, exp, FileRange(symbol.fileRange, RangeAt(end))));
          i = end;
        } else if (hasCachedExpr()) {
          SHOW("Assignment has cached expression")
          auto endRes = firstPrimaryPosition(TokenType::stop, i);
          if (endRes.has_value() && (endRes.value() < upto)) {
            auto  end = endRes.value();
            auto* lhs = consumeCachedExpr();
            auto* exp = parseExpression(preCtx, None, i, end).first;
            result.push_back(new ast::Assignment(lhs, exp, {lhs->fileRange, RangeAt(end)}));
            i = end;
          } else {
            Error("Invalid end of sentence", {consumeCachedExpr()->fileRange, token.fileRange});
          }
        } else {
          Error("Expected an expression to assign the expression to", token.fileRange);
        }
        break;
      }
      case TokenType::say: {
        ast::SayType sayTy = ast::SayType::say;
        if (isNext(TokenType::child, i)) {
          if (isNext(TokenType::identifier, i + 1)) {
            if (ValueAt(i + 2) == "dbg") {
              sayTy = ast::SayType::dbg;
            } else if (ValueAt(i + 2) == "only") {
              sayTy = ast::SayType::only;
            } else {
              Error("Unexpected variant of say found", RangeSpan(i, i + 1));
            }
            i += 2;
          } else {
            Error("Expected an identifier for the variant of say", RangeSpan(i, i + 1));
          }
        }
        auto end_res = firstPrimaryPosition(TokenType::stop, i);
        if (!end_res.has_value() || (end_res.value() >= upto)) {
          Error("Say sentence has invalid end", token.fileRange);
        }
        auto end  = end_res.value();
        auto exps = parseSeparatedExpressions(ctx, i, end);
        result.push_back(new ast::SayLike(sayTy, exps, token.fileRange));
        i = end;
        break;
      }
      case TokenType::stop: {
        SHOW("Parsed expression sentence")
        if (hasCachedExpr()) {
          auto* expr = consumeCachedExpr();
          result.push_back(new ast::ExpressionSentence(expr, {expr->fileRange, token.fileRange}));
        } else if (hasCachedSymbol()) {
          auto symbol = consumeCachedSymbol();
          result.push_back(new ast::ExpressionSentence(new ast::Entity(symbol.relative, symbol.name, symbol.fileRange),
                                                       symbol.fileRange));
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
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes) {
            auto* cand = parseExpression(preCtx, None, i + 1, pCloseRes.value()).first;
            i          = pCloseRes.value();
            if (isNext(TokenType::bracketOpen, i)) {
              auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i + 1, false);
              if (bCloseRes) {
                Vec<Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>> chain;
                Maybe<Pair<Vec<ast::Sentence*>, FileRange>>           elseCase;
                parseMatchContents(preCtx, i + 1, bCloseRes.value(), chain, elseCase, isTypeMatch);
                result.push_back(new ast::Match(isTypeMatch, cand, std::move(chain), std::move(elseCase),
                                                RangeSpan(start, bCloseRes.value())));
                i = bCloseRes.value();
              } else {
                Error("Expected end for [", RangeAt(i + 1));
              }
            } else {
              Error("Expected sentences for this case of the match sentence", RangeSpan(start, i));
            }
          } else {
            Error("Expected end for (", RangeAt(i + 1));
          }
        } else {
          Error("Expected expression to match", RangeAt(start));
        }
        break;
      }
      case TokenType::New: {
        const auto start = i;
        SHOW("Found new")
        bool isRef     = false;
        bool isPtr     = false;
        bool isVarDecl = false;
        if (isNext(TokenType::var, i)) {
          isVarDecl = true;
          if (isNext(TokenType::referenceType, i + 1)) {
            isRef = true;
            i += 2;
          } else if (isNext(TokenType::pointerType, i + 1)) {
            isPtr = true;
            i += 2;
          } else {
            i++;
          }
        } else if (isNext(TokenType::referenceType, i)) {
          isRef = true;
          i++;
        } else if (isNext(TokenType::pointerType, i)) {
          isPtr = true;
          i++;
        }
        if (isNext(TokenType::identifier, i)) {
          if (isNext(TokenType::assignment, i + 1)) {
            auto endRes = firstPrimaryPosition(TokenType::stop, i + 2);
            if (endRes.has_value()) {
              auto  end = endRes.value();
              auto* exp = parseExpression(preCtx, None, i + 2, end).first;
              result.push_back(new ast::LocalDeclaration(nullptr, isRef, isPtr, IdentifierAt(i + 1), exp, isVarDecl,
                                                         RangeSpan(start, end)));
              i = end;
            } else {
              Error("Invalid end for sentence", RangeSpan(start, i + 2));
            }
          } else if (isNext(TokenType::colon, i + 1)) {
            auto endRes = firstPrimaryPosition(TokenType::stop, i + 2);
            if (endRes.has_value()) {
              if (isPrimaryWithin(TokenType::assignment, i + 2, endRes.value())) {
                auto  asgnPos = firstPrimaryPosition(TokenType::assignment, i + 2).value();
                auto* typeRes = parseType(preCtx, i + 2, asgnPos).first;
                auto* exp     = parseExpression(preCtx, None, asgnPos, endRes.value()).first;
                result.push_back(new ast::LocalDeclaration(typeRes, isRef, isPtr, IdentifierAt(i + 1), exp, isVarDecl,
                                                           RangeSpan(start, endRes.value())));
              } else {
                // No value for the assignment
                auto* typeRes = parseType(preCtx, i + 2, endRes.value()).first;
                result.push_back(new ast::LocalDeclaration(typeRes, isRef, isPtr, IdentifierAt(i + 1), nullptr,
                                                           isVarDecl, RangeSpan(start, endRes.value())));
              }
              i = endRes.value();
              break;
            } else {
              Error("Expected . to mark the end of this declaration", RangeSpan(start, i + 1));
            }
          } else if (isNext(TokenType::from, i + 1)) {
            Error("Initialisation via constructor or convertor can be used "
                  "only if there is a type provided. This is an inferred "
                  "declaration",
                  RangeSpan(start, i + 2));
          } else if (isNext(TokenType::stop, i + 1)) {
            Error("Only declarations with maybe type can omit the value to be assigned", RangeSpan(start, i + 2));
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
        Vec<Pair<ast::Expression*, Vec<ast::Sentence*>>> chain;
        Maybe<Vec<ast::Sentence*>>                       elseCase;
        FileRange                                        fileRange = token.fileRange;
        while (true) {
          if (isNext(TokenType::parenthesisOpen, i)) {
            auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
            if (pCloseRes) {
              auto* exp = parseExpression(preCtx, None, i + 1, pCloseRes.value()).first;
              if (isNext(TokenType::bracketOpen, pCloseRes.value())) {
                auto bCloseRes =
                    getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, pCloseRes.value() + 1, false);
                if (bCloseRes.has_value()) {
                  fileRange = FileRange(fileRange, RangeAt(bCloseRes.value()));
                  auto snts = parseSentences(preCtx, pCloseRes.value() + 1, bCloseRes.value());
                  chain.push_back(Pair<ast::Expression*, Vec<ast::Sentence*>>(exp, snts));
                  if (isNext(TokenType::Else, bCloseRes.value())) {
                    SHOW("Found else")
                    if (isNext(TokenType::If, bCloseRes.value() + 1)) {
                      i = bCloseRes.value() + 2;
                      continue;
                    } else if (isNext(TokenType::bracketOpen, bCloseRes.value() + 1)) {
                      SHOW("Else case begin")
                      auto bOp    = bCloseRes.value() + 2;
                      auto bClRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, bOp, false);
                      if (bClRes) {
                        fileRange = FileRange(fileRange, RangeAt(bClRes.value()));
                        elseCase  = parseSentences(preCtx, bOp, bClRes.value());
                        i         = bClRes.value();
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
          auto expRes = parseExpression(preCtx, None, i > 0 ? i - 1 : 0, None);
          setCachedExpr(expRes.first);
          i = expRes.second;
          break;
        } else if (isNext(TokenType::colon, i) || isNext(TokenType::bracketOpen, i)) {
          SHOW("Infinite loop")
          auto          pos = i;
          Maybe<String> tag = None;
          if (isNext(TokenType::colon, i)) {
            if (isNext(TokenType::identifier, i + 1)) {
              tag = ValueAt(i + 2);
              pos = i + 2;
            } else {
              Error("Expected identifier after : for the tag for the loop", RangeAt(i + 1));
            }
          }
          if (isNext(TokenType::bracketOpen, pos)) {
            auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, pos + 1, false);
            if (bCloseRes.has_value()) {
              auto bClose    = bCloseRes.value();
              auto sentences = parseSentences(preCtx, pos + 1, bClose);
              result.push_back(new ast::LoopInfinite(sentences, tag, RangeSpan(i, bClose)));
              i = bClose;
            } else {
              Error("Expected end for [", RangeAt(pos + 1));
            }
          }
        } else if (isNext(TokenType::parenthesisOpen, i)) {
          auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1, false);
          if (pCloseRes.has_value()) {
            auto              pClose  = pCloseRes.value();
            Maybe<Identifier> loopTag = None;
            auto*             count   = parseExpression(preCtx, None, i + 1, pClose).first;
            auto              pos     = pClose;
            if (isNext(TokenType::colon, pClose)) {
              if (isNext(TokenType::New, pClose + 1)) {
                if (isNext(TokenType::identifier, pClose + 2)) {
                  loopTag = IdentifierAt(pClose + 3);
                  pos     = pClose + 3;
                } else {
                  Error("Expected an identifier for the tag for the loop after "
                        "alias",
                        RangeAt(pClose + 2));
                }
              } else if (isNext(TokenType::identifier, pClose + 1)) {
                loopTag = IdentifierAt(pClose + 2);
                pos     = pClose + 2;
              } else {
                Error("Expected an identifier for the tag for the loop", RangeAt(pClose + 1));
              }
            }
            if (isNext(TokenType::bracketOpen, pos)) {
              auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, pos + 1, false);
              if (bCloseRes.has_value()) {
                auto snts = parseSentences(preCtx, pos + 1, bCloseRes.value());
                result.push_back(new ast::LoopNTimes(count, parseSentences(preCtx, pos + 1, bCloseRes.value()), loopTag,
                                                     RangeSpan(i, bCloseRes.value())));
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
            auto pCloseRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2, false);
            if (pCloseRes.has_value()) {
              auto  pClose = pCloseRes.value();
              auto* cond   = parseExpression(preCtx, None, i + 2, pClose).first;
              if (isNext(TokenType::bracketOpen, pCloseRes.value())) {
                auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, pClose + 1, false);
                if (bCloseRes.has_value()) {
                  auto snts = parseSentences(preCtx, pClose + 1, bCloseRes.value());
                  result.push_back(new ast::LoopWhile(cond, snts, None, RangeSpan(i, bCloseRes.value())));
                  i = bCloseRes.value();
                  break;
                } else {
                  Error("Expected end for [", RangeAt(pCloseRes.value() + 1));
                }
              } else if (isNext(TokenType::colon, pClose)) {
                if (isNext(TokenType::identifier, pClose + 1)) {
                  if (isNext(TokenType::bracketOpen, pClose + 2)) {
                    auto bCloseRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, pClose + 3, false);
                    if (bCloseRes) {
                      auto snts = parseSentences(preCtx, pClose + 3, bCloseRes.value());
                      result.push_back(
                          new ast::LoopWhile(cond, snts, IdentifierAt(pClose + 2), RangeSpan(i, bCloseRes.value())));
                      i = bCloseRes.value();
                    } else {
                      Error("Expected end for [", RangeAt(pClose + 3));
                    }
                  } else {
                    // FIXME - Implement single statement loop
                  }
                } else {
                  Error("Expected name for the tag for the loop", RangeAt(pClose + 1));
                }
              } else {
                // FIXME - Implement single statement loop
              }
            } else {
              Error("Expected end for (", RangeAt(i + 2));
            }
          }
        } else if (isNext(TokenType::over, i)) {
          // FIXME - Implement
        } else {
          Error("Unexpected token after loop", token.fileRange);
        }
        break;
      }
      case TokenType::give: {
        SHOW("give sentence found")
        if (isNext(TokenType::stop, i)) {
          i++;
          result.push_back(new ast::GiveSentence(None, FileRange(token.fileRange, RangeAt(i + 1))));
        } else {
          auto end = firstPrimaryPosition(TokenType::stop, i);
          if (!end.has_value()) {
            Error("Expected give sentence to end. Please add `.` wherever "
                  "appropriate to mark the end of the statement",
                  token.fileRange);
          }
          auto* exp = parseExpression(ctx, None, i, end.value()).first;
          i         = end.value();
          result.push_back(new ast::GiveSentence(exp, FileRange(token.fileRange, RangeAt(end.value()))));
        }
        break;
      }
      case TokenType::Break: {
        if (isNext(TokenType::child, i)) {
          if (isNext(TokenType::identifier, i + 1)) {
            result.push_back(new ast::Break(IdentifierAt(i + 2), RangeSpan(i, i + 2)));
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
            result.push_back(new ast::Continue(IdentifierAt(i + 2), RangeSpan(i, i + 2)));
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
        auto expRes = parseExpression(preCtx, retrieveCachedSymbol(), i - 1, None, retrieveCachedExpr());
        setCachedExpr(expRes.first);
        i = expRes.second;
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

Pair<Vec<ast::Argument*>, bool> Parser::parseFunctionParameters(ParserContext& preCtx, usize from, usize upto) {
  using ast::FloatType;
  using ast::IntegerType;
  using IR::FloatTypeKind;
  using lexer::TokenType;
  SHOW("Starting parsing function parameters")
  Vec<ast::Argument*> args;

  Maybe<ast::QatType*> typ;
  auto                 hasType = [&typ]() { return typ.has_value(); };
  auto                 useType = [&typ]() {
    if (typ.has_value()) {
      auto* result = typ.value();
      typ          = None;
      return result;
    } else {
      return (ast::QatType*)nullptr;
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
    auto& token = tokens->at(i);
    switch (token.type) { // NOLINT(clang-diagnostic-switch)
      case TokenType::voidType: {
        Error("Arguments cannot have void type", token.fileRange);
        break;
      }
      case TokenType::variadic: {
        if (isNext(TokenType::identifier, i)) {
          args.push_back(ast::Argument::Normal({ValueAt(i + 1), FileRange RangeSpan(i, i + 1)}, nullptr));
          if (isNext(TokenType::parenthesisClose, i + 1) ||
              (isNext(TokenType::separator, i + 1) && isNext(TokenType::parenthesisClose, i + 2))) {
            return {args, true};
          } else {
            Error("Variadic argument should be the last argument of the function", token.fileRange);
          }
        } else {
          Error("Expected name for the variadic argument. Please provide a name", token.fileRange);
        }
        break;
      }
      case TokenType::self: {
        if (isNext(TokenType::identifier, i)) {
          SHOW("Creating member argument: " << ValueAt(i + 1))
          args.push_back(ast::Argument::ForConstructor({ValueAt(i + 1), RangeAt(i + 1)}, nullptr, true));
          if (isNext(TokenType::separator, i + 1) || isNext(TokenType::parenthesisClose, i + 1)) {
            i += 2;
          } else {
            Error("Invalid token found after argument", RangeSpan(i, i + 1));
          }
        } else {
          Error("Expected name of the member to be initialised", token.fileRange);
        }
        break;
      }
      default: {
        auto typeRes = parseType(preCtx, i - 1, None);
        typ          = typeRes.first;
        i            = typeRes.second;
        if (isNext(TokenType::identifier, i)) {
          args.push_back(ast::Argument::Normal({ValueAt(i + 1), typ.value()->fileRange}, typ.value()));
          if (isNext(TokenType::separator, i + 1) || isNext(TokenType::parenthesisClose, i + 1)) {
            i += 2;
          } else {
            Error("Unexpected token found after argument name", RangeAt(i + 1));
          }
        } else {
          Error("Expected a name for the argument, after the type", typ.value()->fileRange);
        }
        break;
      }
    }
  }
  // FIXME - Support variadic function parameters
  return {args, false};
}

Maybe<usize> Parser::getPairEnd(const lexer::TokenType startType, const lexer::TokenType endType, const usize current,
                                const bool respectScope) {
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

bool Parser::areOnlyPresentWithin(const Vec<lexer::TokenType>& kinds, usize from, usize upto) {
  for (usize i = from + 1; i < upto; i++) {
    for (auto const& kind : kinds) {
      if (kind != tokens->at(i).type) {
        return false;
      }
    }
  }
  return true;
}

bool Parser::isPrimaryWithin(lexer::TokenType candidate, usize from, usize upto) {
  auto posRes = firstPrimaryPosition(candidate, from);
  return posRes.has_value() && (posRes.value() < upto);
}

Maybe<usize> Parser::firstPrimaryPosition(const lexer::TokenType candidate, const usize from) {
  using lexer::TokenType;
  for (usize i = from + 1; i < tokens->size(); i++) {
    auto tok = tokens->at(i);
    if (tok.type == TokenType::parenthesisOpen) {
      auto endRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i, false);
      if (endRes.has_value() && (endRes.value() < tokens->size())) {
        i = endRes.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::bracketOpen) {
      auto endRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
      if (endRes.has_value() && (endRes.value() < tokens->size())) {
        i = endRes.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::curlybraceOpen) {
      auto endRes = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i, false);
      if (endRes.has_value() && (endRes.value() < tokens->size())) {
        i = endRes.value();
      } else {
        return None;
      }
    } else if (tok.type == TokenType::genericTypeStart) {
      auto endRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i, false);
      if (endRes.has_value() && (endRes.value() < tokens->size())) {
        i = endRes.value();
      } else {
        return None;
      }
    } else if (tok.type == candidate) {
      return i;
    }
  }
  return None;
}

Vec<usize> Parser::primaryPositionsWithin(lexer::TokenType candidate, usize from, usize upto) {
  Vec<usize> result;
  using lexer::TokenType;
  for (usize i = from + 1; (i < upto && i < tokens->size()); i++) {
    auto tok = tokens->at(i);
    if (tok.type == TokenType::parenthesisOpen) {
      auto endRes = getPairEnd(TokenType::parenthesisOpen, TokenType::parenthesisClose, i, false);
      if (endRes.has_value() && (endRes.value() < upto)) {
        i = endRes.value();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::bracketOpen) {
      auto endRes = getPairEnd(TokenType::bracketOpen, TokenType::bracketClose, i, false);
      if (endRes.has_value() && (endRes.value() < upto)) {
        i = endRes.value();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::curlybraceOpen) {
      auto endRes = getPairEnd(TokenType::curlybraceOpen, TokenType::curlybraceClose, i, false);
      if (endRes.has_value() && (endRes.value() < upto)) {
        i = endRes.value();
      } else {
        return result;
      }
    } else if (tok.type == TokenType::genericTypeStart) {
      auto endRes = getPairEnd(TokenType::genericTypeStart, TokenType::genericTypeEnd, i, false);
      if (endRes.has_value() && (endRes.value() < upto)) {
        i = endRes.value();
      } else {
        return result;
      }
    } else if (tok.type == candidate) {
      result.push_back(i);
    }
  }
  return result;
}

void Parser::Error(const String& message, const FileRange& fileRange) {
  std::cout << colors::highIntensityBackground::red << " parser error " << colors::reset << "▌ " << colors::bold::red
            << message << colors::reset << " | " << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.start.line << ":" << fileRange.start.character << colors::reset << " >> "
            << colors::underline::green << fileRange.file.string() << ":" << fileRange.end.line << ":"
            << fileRange.end.character << colors::reset << "\n";
  tokens->clear();
  ast::Node::clearAll();
  exit(0);
}

void Parser::Warning(const String& message, const FileRange& fileRange) {
  std::cout << colors::highIntensityBackground::yellow << " parser warning " << colors::reset << "▌ "
            << colors::bold::yellow << message << colors::reset << " | " << colors::underline::green
            << fileRange.file.string() << ":" << fileRange.start.line << ":" << fileRange.start.character
            << colors::reset << " >> " << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.end.line << ":" << fileRange.end.character << colors::reset << "\n";
}

} // namespace qat::parser
