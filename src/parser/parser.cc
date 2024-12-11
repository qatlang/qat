#include "./parser.hpp"
#include "../ast/bring_bitwidths.hpp"
#include "../ast/bring_entities.hpp"
#include "../ast/bring_paths.hpp"
#include "../ast/constructor.hpp"
#include "../ast/convertor.hpp"
#include "../ast/define_choice_type.hpp"
#include "../ast/define_core_type.hpp"
#include "../ast/define_mix_type.hpp"
#include "../ast/define_opaque_type.hpp"
#include "../ast/define_region.hpp"
#include "../ast/destructor.hpp"
#include "../ast/do_skill.hpp"
#include "../ast/expressions/address_of.hpp"
#include "../ast/expressions/array_literal.hpp"
#include "../ast/expressions/assembly.hpp"
#include "../ast/expressions/await.hpp"
#include "../ast/expressions/binary_expression.hpp"
#include "../ast/expressions/cast.hpp"
#include "../ast/expressions/constructor_call.hpp"
#include "../ast/expressions/copy.hpp"
#include "../ast/expressions/default.hpp"
#include "../ast/expressions/dereference.hpp"
#include "../ast/expressions/entity.hpp"
#include "../ast/expressions/error.hpp"
#include "../ast/expressions/function_call.hpp"
#include "../ast/expressions/generic_entity.hpp"
#include "../ast/expressions/get_intrinsic.hpp"
#include "../ast/expressions/heap.hpp"
#include "../ast/expressions/index_access.hpp"
#include "../ast/expressions/is.hpp"
#include "../ast/expressions/member_access.hpp"
#include "../ast/expressions/method_call.hpp"
#include "../ast/expressions/mix_choice_initialiser.hpp"
#include "../ast/expressions/move.hpp"
#include "../ast/expressions/negative.hpp"
#include "../ast/expressions/not.hpp"
#include "../ast/expressions/ok.hpp"
#include "../ast/expressions/plain_initialiser.hpp"
#include "../ast/expressions/self_instance.hpp"
#include "../ast/expressions/to_conversion.hpp"
#include "../ast/expressions/tuple_value.hpp"
#include "../ast/function.hpp"
#include "../ast/global_declaration.hpp"
#include "../ast/lib.hpp"
#include "../ast/meta/todo.hpp"
#include "../ast/method.hpp"
#include "../ast/mod_info.hpp"
#include "../ast/operator_function.hpp"
#include "../ast/prerun/array_literal.hpp"
#include "../ast/prerun/binary_op.hpp"
#include "../ast/prerun/bitwise_not.hpp"
#include "../ast/prerun/boolean_literal.hpp"
#include "../ast/prerun/custom_float_literal.hpp"
#include "../ast/prerun/custom_integer_literal.hpp"
#include "../ast/prerun/default.hpp"
#include "../ast/prerun/entity.hpp"
#include "../ast/prerun/float_literal.hpp"
#include "../ast/prerun/function_call.hpp"
#include "../ast/prerun/integer_literal.hpp"
#include "../ast/prerun/member_access.hpp"
#include "../ast/prerun/method_call.hpp"
#include "../ast/prerun/mix_choice_init.hpp"
#include "../ast/prerun/negative.hpp"
#include "../ast/prerun/none.hpp"
#include "../ast/prerun/null_mark.hpp"
#include "../ast/prerun/plain_initialiser.hpp"
#include "../ast/prerun/prerun_global.hpp"
#include "../ast/prerun/to_conversion.hpp"
#include "../ast/prerun/tuple_value.hpp"
#include "../ast/prerun/type_wrap.hpp"
#include "../ast/prerun/unsigned_literal.hpp"
#include "../ast/prerun_sentences/break.hpp"
#include "../ast/prerun_sentences/continue.hpp"
#include "../ast/prerun_sentences/give_sentence.hpp"
#include "../ast/sentences/assignment.hpp"
#include "../ast/sentences/break.hpp"
#include "../ast/sentences/continue.hpp"
#include "../ast/sentences/expression_sentence.hpp"
#include "../ast/sentences/give_sentence.hpp"
#include "../ast/sentences/if_else.hpp"
#include "../ast/sentences/local_declaration.hpp"
#include "../ast/sentences/loop_if.hpp"
#include "../ast/sentences/loop_in.hpp"
#include "../ast/sentences/loop_infinite.hpp"
#include "../ast/sentences/loop_to.hpp"
#include "../ast/sentences/match.hpp"
#include "../ast/sentences/member_initialisation.hpp"
#include "../ast/sentences/say_sentence.hpp"
#include "../ast/type_definition.hpp"
#include "../ast/types/array.hpp"
#include "../ast/types/float.hpp"
#include "../ast/types/function.hpp"
#include "../ast/types/future.hpp"
#include "../ast/types/generic_integer.hpp"
#include "../ast/types/generic_named_type.hpp"
#include "../ast/types/integer.hpp"
#include "../ast/types/linked_generic.hpp"
#include "../ast/types/maybe.hpp"
#include "../ast/types/named.hpp"
#include "../ast/types/native_type.hpp"
#include "../ast/types/pointer.hpp"
#include "../ast/types/prerun_generic.hpp"
#include "../ast/types/reference.hpp"
#include "../ast/types/result.hpp"
#include "../ast/types/self_type.hpp"
#include "../ast/types/string_slice.hpp"
#include "../ast/types/tuple.hpp"
#include "../ast/types/typed_generic.hpp"
#include "../ast/types/unsigned.hpp"
#include "../ast/types/vector.hpp"
#include "../ast/types/void.hpp"
#include "../cli/config.hpp"
#include "../show.hpp"
#include "../utils/utils.hpp"
#include "./cache_symbol.hpp"
#include "./parser_context.hpp"

#include <chrono>
#include <string>
#include <utility>

#define IdentifierAt(ind) Identifier(tokens->at(ind).value, tokens->at(ind).fileRange)
#define ValueAt(ind)      tokens->at(ind).value
#define RangeAt(ind)      tokens->at(ind).fileRange
#define RangeSpan(ind1, ind2)                                                                                          \
	{ tokens->at(ind1).fileRange, tokens->at(ind2).fileRange }

#define ColoredOr(val, rep) (cfg->is_no_color_mode() ? rep : cli::get_color(val))

#define TOKEN_GENERIC_LIST_START ":["
#define TOKEN_GENERIC_LIST_END   "]"

#define TYPE_TRIGGER_TOKENS                                                                                            \
	case TokenType::voidType:                                                                                          \
	case TokenType::nativeType:                                                                                        \
	case TokenType::maybeType:                                                                                         \
	case TokenType::vectorType:                                                                                        \
	case TokenType::stringSliceType:                                                                                   \
	case TokenType::unsignedIntegerType:                                                                               \
	case TokenType::integerType:                                                                                       \
	case TokenType::floatType:                                                                                         \
	case TokenType::futureType:                                                                                        \
	case TokenType::result:

// NOTE - Check if file fileRange values are making use of the new merge
// functionality
namespace qat::parser {

Parser::Parser(ir::Ctx* _irCtx) : irCtx(_irCtx){};

Parser* Parser::get(ir::Ctx* irCtx) { return new Parser(irCtx); }

Parser::~Parser() {
	delete tokens;
	broughtPaths.clear();
	memberPaths.clear();
	comments.clear();
}

u64 Parser::timeInMicroSeconds = 0;
u64 Parser::tokenCount         = 0;

void Parser::set_tokens(Vec<lexer::Token>* allTokens) {
	g_ctx = ParserContext();
	delete tokens;
	tokens = allTokens;
	filter_comments();
}

void Parser::filter_comments() { comments.clear(); }

ast::BringEntities* Parser::parse_bring_entities(ParserContext& ctx, Maybe<ast::VisibilitySpec> visibSpec,
                                                 usize fromMain, usize uptoMain) {
	using lexer::TokenType;
	Vec<ast::BroughtGroup*> rootGroups;

	std::function<void(Maybe<ast::BroughtGroup*>, usize, usize)> handler = [&](Maybe<ast::BroughtGroup*> parent,
	                                                                           usize from, usize upto) {
		for (usize i = from + 1; i < upto; i++) {
			auto token = tokens->at(i);
			switch (token.type) { // NOLINT(clang-diagnostic-switch)
				case TokenType::super: {
					if (parent.has_value()) {
						add_error(color_error("up") + " is not allowed for children of brought entities", RangeAt(i));
					}
				}
				case TokenType::identifier: {
					auto sym_res = do_symbol(ctx, i);
					i            = sym_res.second;
					auto newGroup =
					    ast::BroughtGroup::create(sym_res.first.relative, sym_res.first.name, sym_res.first.fileRange);
					if (is_next(TokenType::curlybraceOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1);
						if (bCloseRes.has_value()) {
							auto        bClose = bCloseRes.value();
							Vec<String> entities;
							if (is_primary_within(TokenType::separator, i + 1, bClose)) {
								auto sepPos = primary_positions_within(TokenType::separator, i + 1, bClose);
								handler(newGroup, i + 1, sepPos.front());
								for (usize k = 0; k < (sepPos.size() - 1); k++) {
									handler(newGroup, sepPos.at(i), sepPos.at(i + 1));
								}
								handler(newGroup, sepPos.back(), bClose);
							} else {
								add_warning(
								    "Expected multiple entities to be brought. Consider removing the curly braces "
								    "since only one child entity is being brought.",
								    FileRange(RangeAt(i + 1), RangeAt(bClose)));
								if (is_next(TokenType::identifier, i + 1)) {
									handler(newGroup, i + 1, bCloseRes.value());
								} else if (is_next(TokenType::super, i + 1)) {
									add_error(".. is not allowed for children of brought entities", RangeAt(i + 2));
								} else {
									add_error("Expected the name of an entity in the bring sentence", RangeAt(i + 2));
								}
							}
						} else {
							add_error("Expected } to close this curly brace", RangeAt(i + 1));
						}
					}
					// FIXME - Support alias for brought entities
					if (parent.has_value()) {
						parent.value()->addMember(newGroup);
					} else {
						rootGroups.push_back(newGroup);
					}
					break;
				}
				case TokenType::separator: {
					if (is_next(TokenType::identifier, i) || is_next(TokenType::super, i)) {
						break;
					} else if (is_next(TokenType::separator, i)) {
						add_error("Two adjacent commas found, please remove duplicate commas", RangeAt(i + 1));
					} else if (is_next(TokenType::stop, i)) {
						break;
					} else {
						add_error("Invalid token found after the comma separator", RangeAt(i + 1));
					}
				}
			}
		}
	};
	handler(None, fromMain, uptoMain);
	return ast::BringEntities::create(rootGroups, visibSpec, RangeSpan(fromMain, uptoMain));
}

Vec<fs::path>& Parser::get_brought_paths() { return broughtPaths; }

void Parser::clear_brought_paths() { broughtPaths.clear(); }

Vec<fs::path>& Parser::get_member_paths() { return memberPaths; }

void Parser::clear_member_paths() { memberPaths.clear(); }

EntityMetadata Parser::do_entity_metadata(ParserContext& parserCtx, usize from, String entityType,
                                          usize genericLength) {
	using lexer::TokenType;
	auto                   i             = from;
	ast::PrerunExpression* defineChecker = nullptr;
	if (is_next(TokenType::define, i) && is_next(TokenType::If, i + 1)) {
		auto start = i + 1;
		if (is_next(TokenType::parenthesisOpen, i + 2)) {
			auto parserCtx = ParserContext();
			auto expRes    = do_prerun_expression(parserCtx, i + 3, None);
			defineChecker  = expRes.first;
			i              = expRes.second;
			if (!is_next(TokenType::parenthesisClose, i)) {
				add_error("Expected ) to end the define condition after this", RangeSpan(start, i));
			}
			i++;
			if (is_next(TokenType::separator, i) && !is_next(TokenType::where, i + 1) &&
			    !is_next(TokenType::meta, i + 1)) {
				add_error("Expected " + color_error("where") + " or " + color_error("meta") + " after " +
				              color_error(",") + " and if you did not mean to include these, remove the " +
				              color_error(",") + " separator",
				          RangeAt(i + 1));
			} else if (is_next(TokenType::separator, i)) {
				i++;
			}
		} else {
			add_error("Expected ( to start the define condition for the " + entityType, RangeSpan(start, i));
		}
	} else if (is_next(TokenType::define, i)) {
		add_error("Expected " + color_error("if") + " after this. Define conditions are of the format " +
		              color_error("define if (condition)"),
		          RangeAt(i + 1));
	}
	ast::PrerunExpression* genericConstraint = nullptr;
	if (is_next(TokenType::where, i)) {
		if (genericLength == 0) {
			add_error("Cannot use generic constraint for a " + entityType + " without any generic parameters",
			          RangeAt(i + 1));
		}
		auto start = i + 1;
		if (is_next(TokenType::parenthesisOpen, i + 1)) {
			i++;
		} else {
			add_error("Expected ( after this to start the generic constraint", RangeAt(start));
		}
		auto expRes       = do_prerun_expression(parserCtx, i + 1, None);
		genericConstraint = expRes.first;
		i                 = expRes.second;
		if (!is_next(TokenType::parenthesisClose, i)) {
			add_error("Expected ) after this to end the generic constraint", RangeSpan(start, i));
		}
		i++;
		if (is_next(TokenType::separator, i) && !is_next(TokenType::meta, i + 1)) {
			add_error("Expected " + color_error("meta") + " after , for the metadata for the " + entityType +
			              ". If you did not mean to include the metadata, remove the " + color_error(",") +
			              " separator",
			          RangeAt(i + 1));
		} else if (is_next(TokenType::separator, i)) {
			i++;
		}
	}
	Maybe<ast::MetaInfo> metaInfo;
	if (is_next(TokenType::meta, i)) {
		if (is_next(TokenType::curlybraceOpen, i + 1)) {
			auto cRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 2);
			if (cRes.has_value()) {
				metaInfo = do_meta_info(i + 2, cRes.value(), RangeAt(i));
				i        = cRes.value();
				if (is_next(TokenType::separator, i)) {
					add_error("Please remove the " + color_error(",") + " separator after this, as it is not necessary",
					          RangeAt(i + 1));
				}
			} else {
				add_error("Expected } to end the meta information", RangeAt(i + 2));
			}
		} else {
			add_error("Expected { after " + color_error("meta") + " to start the meta information", RangeAt(i));
		}
	}
	return EntityMetadata(defineChecker, genericConstraint, std::move(metaInfo), i);
}

ast::MetaInfo Parser::do_meta_info(usize from, usize upto, FileRange fileRange) {
	Vec<Pair<Identifier, ast::PrerunExpression*>> fields;

	bool             isInline = false;
	Maybe<FileRange> inlineRange;
	using lexer::TokenType;
	for (usize i = from + 1; i < upto; i++) {
		auto& token = tokens->at(i);
		switch (token.type) {
			case TokenType::Inline: {
				if (isInline) {
					add_error(color_error("inline") + " has already been provided", RangeAt(i));
				}
				isInline = true;
				if (!is_next(TokenType::associatedAssignment, i)) {
					inlineRange = RangeAt(i);
					if (is_next(TokenType::separator, i)) {
						i++;
					}
					break;
				}
			}
			case TokenType::identifier: {
				if (!is_next(TokenType::associatedAssignment, i)) {
					add_error("Expected := after this", RangeAt(i));
				}
				auto fieldName = Identifier((token.type == TokenType::Inline) ? "inline" : ValueAt(i), RangeAt(i));
				auto sepPos    = first_primary_position(TokenType::separator, i + 1);
				auto preCtx    = parser::ParserContext(); // MetaInfo doesn't share the context of its scope
				if (sepPos.has_value() && (sepPos.value() < upto)) {
					auto expRes = do_prerun_expression(preCtx, i + 1, sepPos.value());
					if (expRes.second + 1 != sepPos.value()) {
						add_error("Expression does not span until ,", RangeSpan(expRes.second, sepPos.value()));
					}
					fields.push_back({IdentifierAt(i), expRes.first});
					i = sepPos.value();
				} else {
					auto expRes = do_prerun_expression(preCtx, i + 1, upto);
					if (expRes.second + 1 != upto) {
						add_error("Expression does not span until }", expRes.first->fileRange);
					}
					fields.push_back({IdentifierAt(i), expRes.first});
					i = upto;
				}
				break;
			}
			default: {
				add_error("Invalid token found here", RangeAt(i));
			}
		}
	}
	return ast::MetaInfo(fields, FileRange{fileRange, RangeAt(upto)});
}

ast::BringPaths* Parser::parse_bring_paths(bool isMember, usize from, usize upto, Maybe<ast::VisibilitySpec> spec,
                                           const FileRange& startRange) {
	using lexer::TokenType;
	Vec<ast::StringLiteral*>        paths;
	Vec<Maybe<ast::StringLiteral*>> names;
	for (usize i = from + 1; (i < upto) && (i < tokens->size()); i++) {
		const auto& token = tokens->at(i);
		switch (token.type) {
			case TokenType::StringLiteral: {
				if (is_next(TokenType::StringLiteral, i)) {
					add_error("Implicit concatenation of adjacent string literals are not "
					          "supported in bring sentences, to avoid ambiguity and confusion. "
					          "If this is supposed to be another file or folder to bring, then "
					          "add a separator between these. Or else fix the sentence.",
					          RangeAt(i + 1));
				} else if (is_next(TokenType::as, i)) {
					if (is_next(TokenType::identifier, i + 1)) {
						names.push_back(ast::StringLiteral::create(ValueAt(i + 2), RangeAt(i + 2)));
						i += 2;
					} else {
						add_error("Expected alias for the file module", RangeSpan(i, i + 1));
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
				paths.push_back(ast::StringLiteral::create(token.value, token.fileRange));
				break;
			}
			case TokenType::separator: {
				if (is_next(TokenType::StringLiteral, i) || is_next(TokenType::stop, i)) {
					continue;
				} else {
					if (is_next(TokenType::separator, i)) {
						// NOTE - This might be too much, or not
						add_error("Multiple adjacent separators found. Please remove this", RangeAt(i + 1));
					} else {
						add_error("Unexpected token in bring sentence", RangeAt(i));
					}
				}
				break;
			}
			default: {
				add_error("Unexpected token in bring sentence", RangeAt(i));
			}
		}
	}
	return ast::BringPaths::create(isMember, std::move(paths), std::move(names), spec, {startRange, RangeAt(upto)});
}

Pair<ast::PrerunExpression*, usize> Parser::do_prerun_expression(ParserContext& preCtx, usize from, Maybe<usize> upto,
                                                                 bool returnOnFirstExp) {
	using lexer::TokenType;

	Maybe<ast::PrerunExpression*> _cacheExp_;
	auto                          hasCachedExp     = [&]() { return _cacheExp_.has_value(); };
	auto                          consumeCachedExp = [&]() {
        auto result = _cacheExp_.value();
        _cacheExp_  = None;
        return result;
	};

#define setCachedPreExp(retVal, retInd)                                                                                \
	if (_cacheExp_.has_value()) {                                                                                      \
		add_error("Internal error: An expression is already parsed and found another one", RangeAt(i));                \
	} else {                                                                                                           \
		if ((retInd + 1 <= tokens->size()) && (tokens->at(retInd + 1).type == TokenType::binaryOperator) &&            \
		    returnOnFirstExp) {                                                                                        \
			return Pair<ast::PrerunExpression*, usize>{retVal, retInd};                                                \
		}                                                                                                              \
		_cacheExp_ = retVal;                                                                                           \
	}

	auto i = 0;
	for (i = from + 1; i < (upto.has_value() ? upto.value() : tokens->size()); i++) {
		auto& token = tokens->at(i);
		switch (token.type) {
			case TokenType::comment: {
				break;
			}
				TYPE_TRIGGER_TOKENS {
					auto typeRes = do_type(preCtx, i - 1, None);
					i            = typeRes.second;
					setCachedPreExp(ast::TypeWrap::create(typeRes.first, false, typeRes.first->fileRange), i);
					break;
				}
			case TokenType::super:
			case TokenType::identifier: {
				const auto start  = i;
				auto       symRes = do_symbol(preCtx, i);
				auto       name   = symRes.first.name;
				i                 = symRes.second;
				if (is_next(TokenType::genericTypeStart, i)) {
					// FIXME - Implement
					add_error("This syntax not supported yet", RangeAt(i));
				}
				if (name.size() == 1) {
					if (preCtx.has_typed_generic(name[0].value)) {
						setCachedPreExp(ast::TypeWrap::create(ast::LinkedGeneric::create(
						                                          preCtx.get_typed_generic(name[0].value), RangeAt(i)),
						                                      false, RangeAt(i)),
						                i);
						break;
					}
					SHOW("No generic found: " << name[0].value)
				}
				setCachedPreExp(ast::PrerunEntity::create(symRes.first.relative, name, RangeSpan(start, i)),
				                symRes.second);
				break;
			}
			case TokenType::typeSeparator: {
				auto                          start = i;
				Maybe<ast::PrerunExpression*> typeExp;
				if (hasCachedExp()) {
					typeExp = consumeCachedExp();
				}
				if (is_next(TokenType::identifier, i)) {
					auto                          name = IdentifierAt(i + 1);
					Maybe<ast::PrerunExpression*> valueExp;
					i++;
					if (is_next(TokenType::parenthesisOpen, i)) {
						auto expRes = do_prerun_expression(preCtx, i + 1, None);
						valueExp    = expRes.first;
						i           = expRes.second;
						if (is_next(TokenType::parenthesisClose, i)) {
							i++;
						} else {
							add_error("The expression ended here, and expected ) to indicate the end of the expression",
							          (typeExp.has_value() ? FileRange{typeExp.value()->fileRange, RangeSpan(start, i)}
							                               : FileRange RangeSpan(start, i)));
						}
					}
					setCachedPreExp(ast::PrerunMixOrChoiceInit::create(typeExp, name, valueExp, RangeSpan(start, i)),
					                i);
				} else {
					add_error(
					    "Expected an identifier after this to mention the variant of the mix or choice type to init",
					    (typeExp.has_value() ? FileRange{typeExp.value()->fileRange, RangeAt(i)} : RangeAt(i)));
				}
				break;
			}
			case TokenType::Type: {
				if (is_next(TokenType::parenthesisOpen, i)) {
					auto pClose = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
					if (pClose.has_value()) {
						auto typeRes = do_type(preCtx, i + 1, pClose);
						if (typeRes.second + 1 != pClose.value()) {
							add_error("Type ended before the detected end of the type wrap",
							          RangeSpan(typeRes.second + 1, pClose.value()));
						}
						setCachedPreExp(ast::TypeWrap::create(typeRes.first, true, RangeSpan(i, pClose.value())),
						                pClose.value());
						i = pClose.value();
					} else {
						add_error("Expected end for (", RangeAt(i + 1));
					}
				} else {
					add_error("Expected (", RangeAt(i));
				}
				break;
			}
			case TokenType::FALSE:
			case TokenType::TRUE: {
				setCachedPreExp(ast::BooleanLiteral::create(token.type == TokenType::TRUE, RangeAt(i)), i);
				break;
			}
			case TokenType::unaryOperator: {
				const auto start = i;
				if (ValueAt(i) != "~") {
					add_error("An unexpected unary operator found here", RangeAt(i));
				}
				auto expRes = do_prerun_expression(preCtx, i, upto, true);
				i           = expRes.second;
				setCachedPreExp(ast::PrerunBitwiseNot::create(expRes.first, {RangeAt(start), expRes.first->fileRange}),
				                i);
				break;
			}
			case TokenType::binaryOperator: {
				if (!hasCachedExp() && token.value == "-") {
					SHOW("Parser: Found unary -")
					auto expRes = do_prerun_expression(preCtx, i, upto, true);
					SHOW("Negative op for nodetype: " << (int)(expRes.first->nodeType()))
					setCachedPreExp(ast::PrerunNegative::create(expRes.first, RangeSpan(i, expRes.second)),
					                expRes.second);
					i = expRes.second;
				} else if (hasCachedExp()) {
					auto opr    = ast::operator_from_string(ValueAt(i));
					auto rhsRes = do_prerun_expression(preCtx, i, upto, true);
					if (is_next(TokenType::binaryOperator, rhsRes.second) && !returnOnFirstExp) {
						Deque<ast::Op>                operators{opr};
						Deque<ast::PrerunExpression*> expressions{consumeCachedExp(), rhsRes.first};
						i = rhsRes.second;
						while (is_next(TokenType::binaryOperator, i)) {
							auto newOp  = ast::operator_from_string(ValueAt(i + 1));
							auto newRhs = do_prerun_expression(preCtx, i + 1, upto, true);
							operators.push_back(newOp);
							expressions.push_back(newRhs.first);
							i = newRhs.second;
						}
						auto findLowestPrecedence = [&]() {
							usize lowestPrec = 1000;
							usize result     = 0;
							for (usize j = 0; j < operators.size(); j++) {
								if (ast::get_precedence_of(operators[j]) < lowestPrec) {
									lowestPrec = ast::get_precedence_of(operators[j]);
									result     = j;
								}
							}
							return result;
						};
						while (operators.size() > 0) {
							auto index  = findLowestPrecedence();
							auto newExp = ast::PrerunBinaryOp::create(
							    expressions[index], operators[index], expressions[index + 1],
							    {expressions[index]->fileRange, expressions[index + 1]->fileRange});
							operators.erase(operators.begin() + index);
							expressions.erase(expressions.begin() + index + 1);
							expressions.at(index) = newExp;
						}
						setCachedPreExp(expressions.front(), i);
					} else {
						auto lhs = consumeCachedExp();
						i        = rhsRes.second;
						setCachedPreExp(ast::PrerunBinaryOp::create(lhs, opr, rhsRes.first,
						                                            {lhs->fileRange, rhsRes.first->fileRange}),
						                i);
					}
				} else {
					add_error("No expression found on the left hand side of the binary operator " +
					              color_error(token.value),
					          RangeAt(i));
				}
				break;
			}
			case TokenType::integerLiteral: {
				SHOW("INTEGER_LITERAL FOUND IN PARSER: " << token.value)
				usize     radPos = 0;
				Maybe<u8> radix;
				if (token.value.find("0b") == 0) {
					radix  = 2;
					radPos = 2;
				} else if (token.value.find("0c") == 0) {
					radix  = 8;
					radPos = 2;
				} else if (token.value.find("0x") == 0) {
					radix  = 16;
					radPos = 2;
				} else if (token.value.find("0r") == 0) {
					if (token.value.find("_") != String::npos) {
						// Check for being integer is not required here as it is done within the lexer itself
						auto radVal = std::stoul(token.value.substr(2, token.value.find("_") - 2));
						if ((radVal > 36u) || (radVal < 2)) {
							add_error("Radix value has to be a value from 2 to 36", token.fileRange);
						}
						radix  = (u8)radVal;
						radPos = token.value.find("_") + 1;
					} else {
						add_error("Expected _ to be found to separate the radix specification from the number value",
						          token.fileRange);
					}
				}
				usize lastUnderscorePos = token.value.find_last_of('_');
				if (lastUnderscorePos != String::npos && (lastUnderscorePos + 1 < token.value.length()) &&
				    !utils::is_integer(String(1, token.value.at(lastUnderscorePos + 1)))) {
					String            bitStr = ((lastUnderscorePos + 1 < token.value.length()) &&
                                     (token.value.at(lastUnderscorePos + 1) == 'i' ||
                                      token.value.at(lastUnderscorePos + 1) == 'u') &&
                                     (lastUnderscorePos + 2 < token.value.length()))
					                               ? token.value.substr(lastUnderscorePos + 2)
					                               : "";
					Maybe<Identifier> suffix;
					if (!utils::is_integer(bitStr) && (lastUnderscorePos + 1 < token.value.length())) {
						suffix = {token.value.substr(lastUnderscorePos + 1),
						          FileRange(token.fileRange.file,
						                    FilePos{token.fileRange.start.line,
						                            token.fileRange.start.character + lastUnderscorePos + 1},
						                    token.fileRange.end)};
					}
					Maybe<u64> bits =
					    (!bitStr.empty() && utils::is_integer(bitStr)) ? Maybe<u64>(std::stoul(bitStr)) : None;
					Maybe<bool> isUnsigned;
					if (suffix.has_value() && suffix->value.length() == 1) {
						if (suffix.value().value[0] == 'u') {
							isUnsigned = true;
							suffix     = None;
						} else if (suffix.value().value[0] == 'i') {
							isUnsigned = false;
							suffix     = None;
						}
					} else if (!suffix.has_value() && !radix.has_value()) {
						isUnsigned = (bits.has_value() && (lastUnderscorePos + 1 < token.value.length()) &&
						              (token.value.at(lastUnderscorePos + 1) == 'u'));
					}
					if (radix.has_value() || suffix.has_value()) {
						String number(token.value.substr(radPos, lastUnderscorePos - radPos));
						SHOW("CUSTOM_INTEGER_LITERAL: " << number << " bits: " << (bits.has_value() ? bits.value() : 0))
						setCachedPreExp(
						    ast::CustomIntegerLiteral::create(number, isUnsigned, bits, radix, suffix, token.fileRange),
						    i);
					} else {
						SHOW("INTEGER_LITERAL: " << token.value << " isUnsigned: " << (isUnsigned ? "true" : "false")
						                         << " bits: " << (bits.has_value() ? bits.value() : 0u))
						setCachedPreExp(
						    isUnsigned.has_value() && isUnsigned.value()
						        ? (ast::PrerunExpression*)(ast::UnsignedLiteral::create(
						              token.value.substr(0, lastUnderscorePos),
						              bits.has_value()
						                  ? Maybe<Pair<u64, FileRange>>(
						                        {bits.value(), FileRange(token.fileRange.file,
						                                                 FilePos{token.fileRange.start.line,
						                                                         token.fileRange.start.character +
						                                                             lastUnderscorePos + 1},
						                                                 token.fileRange.end)})
						                  : None,
						              token.fileRange))
						        : (ast::PrerunExpression*)(ast::IntegerLiteral::create(
						              token.value.substr(0, lastUnderscorePos),
						              bits.has_value()
						                  ? Maybe<Pair<u64, FileRange>>(
						                        {bits.value(), FileRange(token.fileRange.file,
						                                                 FilePos{token.fileRange.start.line,
						                                                         token.fileRange.start.character +
						                                                             lastUnderscorePos + 1},
						                                                 token.fileRange.end)})
						                  : None,
						              token.fileRange)),
						    i);
						SHOW("Set integer literal. i = " << i << "; from = " << from
						                                 << "; upto = " << (upto.has_value() ? upto.value() : 0))
					}
				} else if (radix.has_value()) {
					SHOW("CUSTOM_INTEGER_LITERAL: " << token.value.substr(radPos))
					setCachedPreExp(ast::CustomIntegerLiteral::create(token.value.substr(radPos), None, None, radix,
					                                                  None, token.fileRange),
					                i);
				} else {
					bool nonDigitChar = false;
					for (auto dig : token.value) {
						if (String("0123456789_").find(dig) == String::npos) {
							nonDigitChar = true;
							break;
						}
					}
					if (nonDigitChar) {
						SHOW("CUSTOM_INTEGER_LITERAL: " << token.value)
						setCachedPreExp(
						    ast::CustomIntegerLiteral::create(token.value, None, None, None, None, token.fileRange), i);
					} else {
						SHOW("INTEGER_LITERAL: " << token.value)
						setCachedPreExp(ast::IntegerLiteral::create(token.value, None, token.fileRange), i);
					}
				}
				break;
			}
			case TokenType::floatLiteral: {
				auto number         = token.value;
				auto expPos         = number.find('e');
				auto lastUnderscore = number.find_last_of('_');
				if ((lastUnderscore != String::npos) || (expPos != String::npos)) {
					SHOW("Found custom float literal: " << number.substr(lastUnderscore + 1))
					setCachedPreExp(ast::CustomFloatLiteral::create(
					                    number.substr(0, lastUnderscore),
					                    (lastUnderscore != String::npos) ? number.substr(lastUnderscore + 1) : "",
					                    token.fileRange),
					                i);
				} else {
					SHOW("Found float literal")
					setCachedPreExp(ast::FloatLiteral::create(token.value, token.fileRange), i);
				}
				break;
			}
			case TokenType::StringLiteral: {
				setCachedPreExp(ast::StringLiteral::create(token.value, token.fileRange), i);
				break;
			}
			case TokenType::Default: {
				auto start = i;
				if (is_next(TokenType::genericTypeStart, i)) {
					auto gEndRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i);
					if (gEndRes.has_value()) {
						auto typeres = do_type(preCtx, i, gEndRes);
						if (typeres.second > gEndRes.value()) {
							add_error(
							    "Type parsing exceeded the recognised end of type specification of the default expression",
							    RangeSpan(start, typeres.second));
						}
						setCachedPreExp(ast::PrerunDefault::create(typeres.first, RangeSpan(start, gEndRes.value())),
						                gEndRes.value());
						i = gEndRes.value();
						break;
					} else {
						add_error("Expected " TOKEN_GENERIC_LIST_END
						          " to end the type specification for the default expression",
						          RangeAt(i));
					}
				}
				setCachedPreExp(ast::PrerunDefault::create(None, RangeAt(i)), i);
				break;
			}
			case TokenType::from:
			case TokenType::colon: {
				const auto isIsolatedFrom = tokens->at(i).type == TokenType::from;
				if (isIsolatedFrom || is_next(TokenType::from, i)) {
					auto start = i;
					if (not isIsolatedFrom) {
						i++;
					}
					if (is_next(TokenType::curlybraceOpen, i)) {
						auto cEndRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1);
						if (not cEndRes.has_value()) {
							add_error("Expected end for {", RangeAt(i));
						}
						const auto cEnd = cEndRes.value();
						if (not hasCachedExp()) {
							add_error("Expected a type or expression before as the type for plain initialisation",
							          RangeAt(i));
						}
						Vec<Identifier>             fields;
						Vec<ast::PrerunExpression*> fieldValues;
						if (is_primary_within(TokenType::associatedAssignment, i + 1, cEnd)) {
							for (usize j = i + 2; j < cEnd; j++) {
								if (not is_next(TokenType::identifier, j - 1)) {
									add_error("Expected an identifier for the name of the member of the struct type",
									          RangeAt(j));
								}
								fields.push_back({ValueAt(j), RangeAt(j)});
								if (not is_next(TokenType::associatedAssignment, j)) {
									add_error("Expected := after the name of the member", RangeAt(j));
								}
								if (is_primary_within(TokenType::separator, j + 1, cEnd)) {
									auto sepRes = first_primary_position(TokenType::separator, j + 1);
									if (not sepRes.has_value()) {
										add_error("Expected ,", RangeSpan(j + 1, cEnd));
									}
									auto sep = sepRes.value();
									if (sep == j + 2) {
										add_error("No expression for the member found", RangeSpan(j + 1, j + 2));
									}
									fieldValues.push_back(do_prerun_expression(preCtx, j + 1, sep).first);
									j = sep;
									continue;
								} else {
									if (cEnd == j + 2) {
										add_error("No expression for the member found", RangeSpan(j + 1, j + 2));
									}
									fieldValues.push_back(do_prerun_expression(preCtx, j + 1, cEnd).first);
									j = cEnd;
								}
							}
						} else {
							fieldValues = do_separated_prerun_expressions(preCtx, i + 1, cEnd);
						}
						auto typExp = consumeCachedExp();
						setCachedPreExp(ast::PrerunPlainInit::create(typExp, fields, fieldValues,
						                                             {typExp->fileRange, RangeSpan(start, cEnd)}),
						                cEnd);
						i = cEnd;
					} else {
						// FIXME - Constructor call
						add_error("Not supported yet", RangeSpan(i, i + 1));
					}
				} else if (not isIsolatedFrom) {
					if (is_next(TokenType::identifier, i)) {
						add_error("This syntax is not supported", RangeSpan(i, i + 1));
					} else {
						add_error("Found : here without an identifier or " + color_error("from") +
						              " following it. This is invalid syntax",
						          RangeAt(i));
					}
				}
				break;
			}
			case TokenType::parenthesisOpen: {
				if (hasCachedExp()) {
					// Function call
					auto                        funcExp = consumeCachedExp();
					Vec<ast::PrerunExpression*> arguments;
					if (is_next(TokenType::parenthesisClose, i)) {
						i++;
					} else {
						auto firstRes = do_prerun_expression(preCtx, i, None);
						arguments.push_back(firstRes.first);
						i = firstRes.second;
						while (is_next(TokenType::separator, i)) {
							if (is_next(TokenType::parenthesisClose, i + 1)) {
								i++;
								break;
							} else if (is_next(TokenType::separator, i + 1)) {
								add_error("Repeating separator " + color_error(",") + " found here", RangeAt(i + 2));
							} else {
								auto expRes = do_prerun_expression(preCtx, i + 1, None);
								arguments.push_back(expRes.first);
								i = expRes.second;
							}
						}
						if (not is_next(TokenType::parenthesisClose, i)) {
							add_error("Expected ) after this to end the list of arguments for the function call",
							          FileRange{funcExp->fileRange, RangeAt(i)});
						}
						i++;
					}
					setCachedPreExp(ast::PrerunFunctionCall::create(funcExp, std::move(arguments),
					                                                {funcExp->fileRange, RangeAt(i)}),
					                i);
				} else {
					const auto start  = i;
					auto       expRes = do_prerun_expression(preCtx, i, None);
					if (is_next(TokenType::semiColon, expRes.second)) {
						// Prerun Tuple Value
						Vec<ast::PrerunExpression*> members{expRes.first};
						i = expRes.second;
						while (is_next(TokenType::semiColon, i)) {
							expRes = do_prerun_expression(preCtx, i + 1, None);
							members.push_back(expRes.first);
							i = expRes.second;
						}
						if (is_next(TokenType::parenthesisClose, i)) {
							add_error("Expected ) after this to end the tuple value", RangeSpan(start, i));
						}
						i++;
						setCachedPreExp(ast::PrerunTupleValue::create(members, RangeSpan(start, i)), i);
					} else if (is_next(TokenType::parenthesisClose, expRes.second)) {
						i = expRes.second + 1;
						setCachedPreExp(expRes.first, expRes.second + 1);
					} else {
						add_error("Expected ) to end the enclosed expression", RangeAt(i));
					}
				}
				break;
			}
			case TokenType::bracketOpen: {
				auto cEndRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
				if (not cEndRes.has_value()) {
					add_error("Expected end for [", RangeAt(i));
				}
				auto cEnd = cEndRes.value();
				setCachedPreExp(ast::PrerunArrayLiteral::create(do_separated_prerun_expressions(preCtx, i, cEnd),
				                                                RangeSpan(i, cEnd)),
				                cEnd);
				i = cEnd;
				break;
			}
			case TokenType::child: {
				if (hasCachedExp()) {
					if (is_next(TokenType::identifier, i)) {
						if (is_next(TokenType::parenthesisOpen, i + 1)) {
							auto pCloseRes =
							    get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2);
							if (pCloseRes.has_value()) {
								auto exp     = consumeCachedExp();
								auto argVals = do_separated_prerun_expressions(preCtx, i + 2, pCloseRes.value());
								setCachedPreExp(
								    ast::PrerunMemberFnCall::create(exp, IdentifierAt(i + 1), argVals,
								                                    {exp->fileRange, RangeAt(pCloseRes.value())}),
								    pCloseRes.value());
								i = pCloseRes.value();
							} else {
								add_error("Expected end for (", RangeAt(i + 2));
							}
						} else {
							auto exp = consumeCachedExp();
							setCachedPreExp(ast::PrerunMemberAccess::create(exp, IdentifierAt(i + 1),
							                                                FileRange{exp->fileRange, RangeAt(i + 1)}),
							                i + 1);
							i = i + 1;
						}
					} else if (is_next(TokenType::to, i)) {
						auto start = i;
						if (is_next(TokenType::parenthesisOpen, i + 1)) {
							auto typeRes = do_type(preCtx, i + 2, None);
							auto exp     = consumeCachedExp();
							i            = typeRes.second;
							if (not is_next(TokenType::parenthesisClose, i)) {
								add_error("Expected ) after this to end the type specification for the " +
								              color_error("to") + " conversion",
								          RangeSpan(start, i));
							}
							i++;
							setCachedPreExp(ast::PrerunTo::create(exp, typeRes.first, {exp->fileRange, RangeAt(i)}), i);
						} else {
							// TODO - Allow type inference in this case???
							add_error("Expected a type to be provided here like " + color_error("'to(TargetType)") +
							              " for the conversion",
							          RangeAt(i + 1));
						}
					} else {
						add_error("Expected an identifier after ' for member access or member function call",
						          RangeAt(i));
					}
				} else {
					add_error("No expression found before '", RangeAt(i));
				}
				break;
			}
			default: {
				if (hasCachedExp()) {
					return {consumeCachedExp(), i - 1};
				} else {
					add_error("No expression parsed and invalid token found here", RangeAt(i));
				}
			}
		}
	}
	if (hasCachedExp()) {
		return {consumeCachedExp(), i - 1};
	} else {
		add_error("No expression found", RangeAt(from));
	}
	return {nullptr, i - 1};
}

Vec<ast::FillGeneric*> Parser::do_generic_fill(ParserContext& preCtx, usize from, usize upto) {
	using lexer::TokenType;
	Vec<ast::FillGeneric*> result;
	for (usize i = from + 1; i < upto; i++) {
		auto& token = tokens->at(i);
		switch (token.type) {
			TYPE_TRIGGER_TOKENS {
				auto subRes = do_type(preCtx, i - 1, upto);
				result.push_back(ast::FillGeneric::create(subRes.first));
				i = subRes.second;
				break;
			}
			case TokenType::super:
			case TokenType::identifier: {
				auto start  = i;
				auto symRes = do_symbol(preCtx, i);
				auto name   = symRes.first.name;
				i           = symRes.second;
				if (is_next(TokenType::genericTypeStart, i)) {
					auto subRes = do_type(preCtx, start, None);
					result.push_back(ast::FillGeneric::create(subRes.first));
					i = subRes.second;
				} else {
					result.push_back(ast::FillGeneric::create(
					    ast::PrerunEntity::create(symRes.first.relative, name, RangeSpan(start, i))));
				}
				break;
			}
			case TokenType::parenthesisOpen: {
				auto start        = i;
				auto pCloseResult = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i);
				if (!pCloseResult.has_value()) {
					add_error("Expected )", token.fileRange);
				}
				if (pCloseResult.value() > upto) {
					add_error("Invalid position for )", RangeAt(pCloseResult.value()));
				}
				if (is_primary_within(TokenType::semiColon, i, pCloseResult.value())) {
					auto            pClose = pCloseResult.value();
					Vec<ast::Type*> subTypes;
					for (usize j = i; j < pClose; j++) {
						if (is_primary_within(TokenType::semiColon, j, pClose)) {
							auto semiPosResult = first_primary_position(TokenType::semiColon, j);
							if (!semiPosResult.has_value()) {
								add_error("Invalid position of ; separator", token.fileRange);
							}
							auto semiPos = semiPosResult.value();
							subTypes.push_back(do_type(preCtx, j, semiPos).first);
							j = semiPos - 1;
						} else if (j != (pClose - 1)) {
							subTypes.push_back(do_type(preCtx, j, pClose).first);
							j = pClose;
						}
					}
					i = pClose;
					// FIXME - Tuple packing logic
					result.push_back(
					    ast::FillGeneric::create(ast::TupleType::create(subTypes, false, token.fileRange)));
				} else {
					auto constRes = do_prerun_expression(preCtx, i, pCloseResult.value());
					result.push_back(ast::FillGeneric::create(constRes.first));
					if (constRes.second >= pCloseResult.value()) {
						add_error("Parsing the constant expression wrapped inside ( and ) exceeded the position of )",
						          RangeSpan(start, constRes.second));
					}
					i = pCloseResult.value();
				}
				break;
			}
			default: {
				auto constRes = do_prerun_expression(preCtx, i - 1, None);
				result.push_back(ast::FillGeneric::create(constRes.first));
				i = constRes.second;
			}
		}
		if (is_next(TokenType::separator, i)) {
			i++;
		} else if (!(i == (upto - 1))) {
			add_error("Unexpected token found here", RangeAt(i + 1));
		}
	}
	return result;
}

Pair<ast::Type*, usize> Parser::do_type(ParserContext& preCtx, usize from, Maybe<usize> upto) {
	using lexer::Token;
	using lexer::TokenType;

	auto              ctx = ParserContext(preCtx);
	usize             i   = 0; // NOLINT(readability-identifier-length)
	Maybe<ast::Type*> cacheTy;

	for (i = from + 1; i < (upto.has_value() ? upto.value() : tokens->size()); i++) {
		Token& token = tokens->at(i);
		switch (token.type) {
			case TokenType::selfInstance:
			case TokenType::selfWord: {
				cacheTy = ast::SelfType::create(token.type == TokenType::selfWord, RangeAt(i));
				break;
			}
			case TokenType::nativeType: {
				auto start = i;
				if (ValueAt(i) == "cPtr") {
					if (is_next(TokenType::genericTypeStart, i)) {
						auto gEnd = first_primary_position(TokenType::genericTypeEnd, i + 1);
						if (gEnd.has_value()) {
							bool isPtrSubtyVar = false;
							if (is_next(TokenType::var, i + 1)) {
								isPtrSubtyVar = true;
								i++;
							}
							auto* subTy = do_type(preCtx, i + 1, gEnd).first;
							cacheTy     = ast::NativeType::create(subTy, isPtrSubtyVar, RangeSpan(start, gEnd.value()));
							i           = gEnd.value();
						} else {
							add_error("Expected " TOKEN_GENERIC_LIST_END " to end the generic type specification",
							          RangeAt(i + 1));
						}
					} else {
						add_error(
						    "Expected subtype for cPtr. Did you forget to provide the subtype like cPtr" TOKEN_GENERIC_LIST_START
						    "subtype" TOKEN_GENERIC_LIST_END " ?",
						    RangeAt(i));
					}
				} else if (((ValueAt(i) == "int") || (ValueAt(i) == "uint")) &&
				           is_next(TokenType::genericTypeStart, i)) {
					auto bitVal = do_prerun_expression(preCtx, i + 1, None);
					i           = bitVal.second;
					if (is_next(TokenType::genericTypeEnd, i)) {
						cacheTy = ast::GenericIntegerType::create_specific(bitVal.first, ValueAt(start) == "uint",
						                                                   RangeSpan(start, i + 1));
					} else {
						add_error("Expected ] after this to end the type", RangeSpan(start, i));
					}
				} else {
					auto nativeKind = ir::native_type_kind_from_string(ValueAt(i));
					if (nativeKind.has_value()) {
						cacheTy = ast::NativeType::create(nativeKind.value(), RangeAt(i));
					} else {
						add_error("Invalid native type", RangeAt(i));
					}
				}
				break;
			}
			case TokenType::vectorType: {
				auto start = i;
				if (is_next(TokenType::genericTypeStart, i)) {
					Maybe<FileRange> scalable;
					if (is_next(TokenType::questionMark, i + 1)) {
						scalable = RangeAt(i + 2);
						if (is_next(TokenType::separator, i + 2)) {
							i = i + 3;
						} else {
							add_error(
							    "Expected , after this to precede the number of minimum number of elements of the vectorized type",
							    RangeSpan(i, i + 2));
						}
					} else {
						i = i + 1;
					}
					auto countRes = do_prerun_expression(preCtx, i, None);
					i             = countRes.second;
					if (is_next(TokenType::separator, i)) {
						i            = i + 1;
						auto typeRes = do_type(preCtx, i, None);
						if (is_next(TokenType::genericTypeEnd, typeRes.second)) {
							i = typeRes.second + 1;
							cacheTy =
							    ast::VectorType::create(typeRes.first, countRes.first, scalable, RangeSpan(start, i));
							break;
						} else {
							add_error("Expected ] after this to end specification of the vectorized type",
							          RangeSpan(start, typeRes.second));
						}
					} else {
						add_error("Expected , after this to precede the element type of the vectorized type",
						          RangeAt(i));
					}
				} else {
					add_error("Expected :[] to enclose the count and element type of the vectorized type", RangeAt(i));
				}
				break;
			}
			case TokenType::result: {
				if (is_next(TokenType::genericTypeStart, i)) {
					auto start    = i;
					bool isPacked = false;
					if (is_next(TokenType::packed, i + 1)) {
						isPacked = true;
						if (!is_next(TokenType::separator, i + 2)) {
							add_error("Expected , after " + color_error("pack"), RangeAt(i + 2));
						}
						i += 2;
					}
					auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (endRes.has_value()) {
						if (is_primary_within(TokenType::separator, i + 1, endRes.value())) {
							auto sepPos  = first_primary_position(TokenType::separator, i + 1).value();
							auto validTy = do_type(preCtx, i + 1, sepPos);
							auto errorTy = do_type(preCtx, sepPos, endRes.value());
							cacheTy      = ast::ResultType::create(validTy.first, errorTy.first, isPacked,
							                                       RangeSpan(start, endRes.value()));
							i            = endRes.value();
						} else {
							add_error("Expected , to separate the valid value type and error value type in " +
							              color_error("result") + " type",
							          RangeSpan(i + 1, endRes.value()));
						}
					} else {
						add_error("Expected end of the generic type specification for " + color_error("result") +
						              " type",
						          RangeAt(start + 1));
					}
				} else {
					add_error(
					    "Expected a type for the valid value and another type for the error value in the result type",
					    RangeAt(i));
				}
				break;
			}
			case TokenType::futureType: {
				if (is_next(TokenType::genericTypeStart, i)) {
					auto start    = i;
					bool isPacked = false;
					if (is_next(TokenType::packed, i + 1)) {
						isPacked = true;
						if (!is_next(TokenType::separator, i + 2)) {
							add_error("Expected , after " + color_error("pack"), RangeAt(i + 2));
						}
						i += 2;
					}
					auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (endRes.has_value()) {
						auto subTyRes = do_type(preCtx, i + 1, endRes);
						cacheTy = ast::FutureType::create(isPacked, subTyRes.first, RangeSpan(start, endRes.value()));
						i       = endRes.value();
					} else {
						add_error("Expected end of the generic type specification for " + color_error("future") +
						              " type",
						          RangeAt(start + 1));
					}
				} else {
					add_error(
					    "Expected sub-type for maybe. Did you forget to provide the sub-type like future:[subtype] ?",
					    RangeAt(i));
				}
				break;
			}
			case TokenType::maybeType: {
				if (is_next(TokenType::genericTypeStart, i)) {
					auto start    = i;
					bool isPacked = false;
					if (is_next(TokenType::packed, i + 1)) {
						isPacked = true;
						if (!is_next(TokenType::separator, i + 2)) {
							add_error("Expected , after " + color_error("pack"), RangeAt(i + 2));
						}
						i += 2;
					}
					auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (endRes.has_value()) {
						auto subTyRes = do_type(preCtx, i + 1, endRes);
						cacheTy = ast::MaybeType::create(isPacked, subTyRes.first, RangeSpan(start, endRes.value()));
						i       = endRes.value();
					} else {
						add_error("Expected end of the generic type specification for " + color_error("maybe") +
						              " type",
						          RangeAt(start + 1));
					}
				} else {
					add_error(
					    "Expected sub-type for maybe. Did you forget to provide the sub-type like maybe:[subtype] ?",
					    RangeAt(i));
				}
				break;
			}
			case TokenType::parenthesisOpen: {
				auto start = i;
				if (cacheTy.has_value()) {
					auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i);
					if (pCloseRes.has_value()) {
						auto hasPrimary = is_primary_within(TokenType::semiColon, i, pCloseRes.value());
						if (hasPrimary) {
							add_error("Tuple type found after another type",
							          FileRange(token.fileRange, RangeAt(pCloseRes.value())));
						} else {
							return {cacheTy.value(), i - 1};
						}
					} else {
						add_error("Expected )", token.fileRange);
					}
				}
				auto pCloseResult = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i);
				if (!pCloseResult.has_value()) {
					add_error("Expected )", token.fileRange);
				}
				if (upto.has_value() && (pCloseResult.value() > upto.value())) {
					add_error("Invalid position for )", RangeAt(pCloseResult.value()));
				}
				auto pClose = pCloseResult.value();
				if (is_next(TokenType::givenTypeSeparator, pClose)) {
					Vec<ast::Type*> argTypes;
					bool            isVariadic = false;
					while (i < pClose) {
						if (is_next(TokenType::variadic, i)) {
							isVariadic = true;
							i++;
							if (is_next(TokenType::separator, i)) {
								i++;
							}
							if (is_next(TokenType::separator, i)) {
								add_error("Repeating separator " + color_error(",") + " found here", RangeAt(i + 1));
							}
							if (i + 1 == pClose) {
								break;
							} else {
								add_error(
								    "Expected the list of argument types to end here, but it did not. Variadic argument should always be the last argument",
								    RangeSpan(i, pClose));
							}
						} else if (is_next(TokenType::selfInstance, i)) {
							if (is_next(TokenType::identifier, i + 1)) {
								add_error("Member arguments are not allowed in function types",
								          RangeSpan(i + 1, i + 2));
							} else {
								add_error(
								    "It seems like you were trying to provide member argument here. Note that the name is missing after this. And member arguments are not allowed in function types",
								    RangeAt(i + 1));
							}
						} else if (is_next(TokenType::separator, i)) {
							if (tokens->at(i).type == TokenType::separator) {
								add_error("Repeating separator " + color_error(",") + " found here", RangeAt(i));
							} else {
								add_error("Found separator " + color_error(",") + " here which is not allowed",
								          RangeAt(i + 1));
							}
						} else if (i != (pClose - 1)) {
							auto typRes = do_type(preCtx, i, None);
							argTypes.push_back(typRes.first);
							i = typRes.second;
							if (is_next(TokenType::separator, i)) {
								i++;
								if (is_next(TokenType::separator, i)) {
									add_error("Repeating separator " + color_error(",") + " found here",
									          RangeAt(i + 1));
								}
							} else if (i != (pClose - 1)) {
								add_error("Invalid syntax found for function type. Expected either separator " +
								              color_error(",") +
								              " or for the list of argument types to end, but found more symbols here",
								          RangeSpan(i, pClose));
							}
						} else {
							break;
						}
					}
					i             = pClose + 1;
					auto retTyRes = do_type(preCtx, pClose + 1, None);
					cacheTy       = ast::FunctionType::create(retTyRes.first, argTypes, isVariadic,
					                                          RangeSpan(start, retTyRes.second));
					i             = retTyRes.second;
				} else {
					if (is_primary_within(TokenType::separator, start, pClose)) {
						add_error(
						    "Expected a type here. Expected ; to separate member types of a tuple type, but found , instead",
						    RangeSpan(start, pClose));
					} else if (is_primary_within(TokenType::semiColon, start, pClose)) {
						Vec<ast::Type*> subTypes;
						for (usize j = i; j < pClose; j++) {
							if (is_primary_within(TokenType::semiColon, j, pClose)) {
								auto semiPosResult = first_primary_position(TokenType::semiColon, j);
								if (!semiPosResult.has_value()) {
									add_error("Invalid position of ; separator", token.fileRange);
								}
								auto semiPos = semiPosResult.value();
								subTypes.push_back(do_type(ctx, j, semiPos).first);
								j = semiPos - 1;
							} else if (j != (pClose - 1)) {
								subTypes.push_back(do_type(ctx, j, pClose).first);
								j = pClose;
							}
						}
						cacheTy = ast::TupleType::create(subTypes, false, RangeSpan(start, pClose));
						i       = pClose;
					} else {
						add_error(
						    "This is not a valid type, could not find ; inside to indicate a tuple type. If you meant to provide a function type, please provide the return type as well like " +
						        color_error("(argTypes...) -> returnType"),
						    RangeSpan(start, pClose));
					}
				}
				break;
			}
			case TokenType::voidType: {
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				cacheTy = ast::VoidType::create(token.fileRange);
				break;
			}
			case TokenType::stringSliceType: {
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				cacheTy = ast::StringSliceType::create(token.fileRange);
				break;
			}
			case TokenType::unsignedIntegerType: {
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				cacheTy = ast::UnsignedType::create(((token.value == "bool") ? 1u : std::stoul(token.value)),
				                                    token.value == "bool", token.fileRange);
				break;
			}
			case TokenType::integerType: {
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				cacheTy = ast::IntegerType::create(std::stoul(token.value), token.fileRange);
				break;
			}
			case TokenType::genericIntegerType: {
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				auto start = i;
				if (is_next(TokenType::genericTypeStart, i)) {
					auto isUnsigned = do_prerun_expression(preCtx, i + 1, None);
					i               = isUnsigned.second;
					if (is_next(TokenType::separator, i)) {
						auto bitVal = do_prerun_expression(preCtx, i + 1, None);
						i           = bitVal.second;
						if (is_next(TokenType::genericTypeEnd, i)) {
							i++;
							cacheTy =
							    ast::GenericIntegerType::create(bitVal.first, isUnsigned.first, RangeSpan(start, i));
						} else {
							add_error("Expected ] after this to end the type", RangeSpan(start, i));
						}
					} else {
						add_error("Expected , after this expression", isUnsigned.first->fileRange);
					}
				} else {
					add_error("Expected :[ after " + color_error("integer"), RangeAt(i));
				}
				break;
			}
			case TokenType::floatType: {
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				if (token.value == "fbrain") {
					cacheTy = ast::FloatType::create(ir::FloatTypeKind::_brain, token.fileRange);
				} else if (token.value == "f16") {
					cacheTy = ast::FloatType::create(ir::FloatTypeKind::_16, token.fileRange);
				} else if (token.value == "f32") {
					cacheTy = ast::FloatType::create(ir::FloatTypeKind::_32, token.fileRange);
				} else if (token.value == "f64") {
					cacheTy = ast::FloatType::create(ir::FloatTypeKind::_64, token.fileRange);
				} else if (token.value == "f80") {
					cacheTy = ast::FloatType::create(ir::FloatTypeKind::_80, token.fileRange);
				} else if (token.value == "f128") {
					cacheTy = ast::FloatType::create(ir::FloatTypeKind::_128, token.fileRange);
				} else if (token.value == "f128ppc") {
					cacheTy = ast::FloatType::create(ir::FloatTypeKind::_128PPC, token.fileRange);
				} else {
					add_error("Invalid float type: " + token.value, RangeAt(i));
				}
				break;
			}
			case TokenType::super:
			case TokenType::identifier: {
				auto start = i;
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				auto symRes = do_symbol(ctx, i);
				auto name   = symRes.first.name;
				i           = symRes.second;
				if (is_next(TokenType::genericTypeStart, i)) {
					auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (endRes.has_value()) {
						auto end = endRes.value();
						if (endRes.value() == i + 2) {
							cacheTy = ast::GenericNamedType::create(symRes.first.relative, symRes.first.name, {},
							                                        RangeSpan(start, end));
						} else {
							auto types = do_generic_fill(preCtx, i + 1, end);
							cacheTy    = ast::GenericNamedType::create(symRes.first.relative, symRes.first.name, types,
							                                           RangeSpan(start, end));
							i          = end;
						}
						break;
					} else {
						add_error("Expected end for generic type specification", RangeAt(i + 1));
					}
				} else if ((name.size() == 1) && (preCtx.has_typed_generic(name.front().value) ||
				                                  preCtx.has_prerun_generic(name.front().value))) {
					ast::GenericAbstractType* genericParameter;
					if (preCtx.has_typed_generic(name.front().value)) {
						genericParameter = preCtx.get_typed_generic(name.front().value);
					} else {
						genericParameter = preCtx.get_prerun_generic(name.front().value);
					}
					cacheTy = ast::LinkedGeneric::create(genericParameter, name.front().range);
					i       = symRes.second;
					break;
				} else {
					cacheTy = ast::NamedType::create(symRes.first.relative, name, symRes.first.fileRange);
					i       = symRes.second;
					break;
				}
				break;
			}
			case TokenType::referenceType: {
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				const auto start    = i;
				bool       isRefVar = false;
				if (is_next(TokenType::var, i)) {
					isRefVar = true;
					i++;
				}
				auto subRes = do_type(ctx, i, None);
				i           = subRes.second;
				cacheTy     = ast::ReferenceType::create(subRes.first, isRefVar, RangeSpan(start, i));
				break;
			}
			case TokenType::sliceType:
			case TokenType::markType: {
				const bool isSlice = token.type == TokenType::sliceType;
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				if (is_next(TokenType::genericTypeStart, i) || is_next(TokenType::exclamation, i)) {
					bool      isSubtypeVar  = false;
					bool      isNonNullable = false;
					TokenType startTy       = TokenType::genericTypeStart;
					TokenType endTy         = TokenType::genericTypeEnd;
					if (is_next(TokenType::exclamation, i)) {
						if (is_next(TokenType::bracketOpen, i + 1)) {
							isNonNullable = true;
							startTy       = TokenType::bracketOpen;
							endTy         = TokenType::bracketClose;
							i++;
						} else {
							add_error("Expected [ to start the subtype of the mark type", RangeSpan(i, i + 1));
						}
					}
					if (is_next(TokenType::var, i + 1)) {
						isSubtypeVar = true;
						i++;
					}
					auto bCloseRes = get_pair_end(startTy, endTy, i + 1);
					if (bCloseRes.has_value() && (!upto.has_value() || (bCloseRes.value() < upto.value()))) {
						auto bClose = bCloseRes.value();
						if (is_primary_within(TokenType::separator, i + 1, bClose)) {
							auto sepPos     = first_primary_position(TokenType::separator, i + 1).value();
							auto subTypeRes = do_type(ctx, i + 1, sepPos);
							if (is_next(TokenType::own, sepPos)) {
								if (sepPos + 2 != bClose) {
									add_error("Ownership did not span till ]", RangeSpan(sepPos + 2, bClose));
								}
								cacheTy = ast::MarkType::create(subTypeRes.first, isSubtypeVar,
								                                ast::MarkOwnType::function, isNonNullable, None,
								                                isSlice, {token.fileRange, RangeAt(bClose)});
							} else if (is_next(TokenType::heap, sepPos)) {
								if (sepPos + 2 != bClose) {
									add_error("Ownership did not span till ]", RangeSpan(sepPos + 2, bClose));
								}
								cacheTy = ast::MarkType::create(subTypeRes.first, isSubtypeVar, ast::MarkOwnType::heap,
								                                isNonNullable, None, isSlice,
								                                {token.fileRange, RangeAt(bClose)});
							} else if (is_next(TokenType::Type, sepPos)) {
								if (is_next(TokenType::parenthesisOpen, sepPos + 1)) {
									auto pCloseRes = get_pair_end(TokenType::parenthesisOpen,
									                              TokenType::parenthesisClose, sepPos + 2);
									if (pCloseRes.has_value()) {
										if (pCloseRes.value() + 1 != bClose) {
											add_error("Ownership did not span till ]",
											          RangeSpan(pCloseRes.value() + 1, bClose));
										}
										auto ownTy = do_type(preCtx, sepPos + 2, pCloseRes.value());
										if (ownTy.second + 1 != pCloseRes.value()) {
											add_error("Owner type did not span till )",
											          RangeSpan(ownTy.second + 1, pCloseRes.value()));
										}
										cacheTy = ast::MarkType::create(
										    subTypeRes.first, isSubtypeVar, ast::MarkOwnType::type, isNonNullable,
										    ownTy.first, isSlice, {token.fileRange, RangeAt(bClose)});
									} else {
										add_error("Expected end for (", RangeAt(sepPos + 2));
									}
								} else {
									add_error("Expected a type to be provided to be the owner of this " +
									              String(isSlice ? "slice" : "mark") + " type like " +
									              color_error(String(isSlice ? "slice:[" : "mark:[") +
									                          subTypeRes.first->to_string() + ", type(OwnerType)]"),
									          RangeSpan(sepPos, bClose));
								}
							} else if (is_next(TokenType::region, sepPos)) {
								if (is_next(TokenType::parenthesisOpen, sepPos + 1)) {
									auto pCloseRes = get_pair_end(TokenType::parenthesisOpen,
									                              TokenType::parenthesisClose, sepPos + 2);
									if (pCloseRes.has_value()) {
										if (pCloseRes.value() + 1 != bClose) {
											add_error("Ownership did not span till ]",
											          RangeSpan(pCloseRes.value() + 1, bClose));
										}
										auto regTy = do_type(preCtx, sepPos + 1, pCloseRes.value());
										if (regTy.second + 1 != pCloseRes.value()) {
											add_error("Owner region specified did not span till )",
											          RangeSpan(regTy.second + 1, pCloseRes.value()));
										}
										cacheTy = ast::MarkType::create(
										    subTypeRes.first, isSubtypeVar, ast::MarkOwnType::region, isNonNullable,
										    regTy.first, isSlice, {token.fileRange, RangeAt(bClose)});
									} else {
										add_error("Expected end for (", RangeAt(sepPos + 2));
									}
								} else {
									cacheTy = ast::MarkType::create(subTypeRes.first, isSubtypeVar,
									                                ast::MarkOwnType::anyRegion, isNonNullable, nullptr,
									                                isSlice, {token.fileRange, RangeAt(bClose)});
								}
							} else if (is_next(TokenType::selfInstance, sepPos)) {
								if (sepPos + 2 != bClose) {
									add_error("Ownership did not span till ]", RangeSpan(sepPos + 2, bClose));
								}
								cacheTy = ast::MarkType::create(subTypeRes.first, isSubtypeVar,
								                                ast::MarkOwnType::typeParent, isNonNullable, None,
								                                isSlice, {token.fileRange, RangeAt(bClose)});
							} else {
								add_error("Invalid ownership of the " + color_error(isSlice ? "slice" : "mark"),
								          {token.fileRange, RangeAt(sepPos)});
							}
						} else {
							auto subTypeRes = do_type(ctx, i + 1, bClose);
							if (subTypeRes.second + 1 != bClose) {
								add_error("Subtype of the pointer did not span till ]",
								          RangeSpan(subTypeRes.second + 1, bClose));
							}
							cacheTy =
							    ast::MarkType::create(subTypeRes.first, isSubtypeVar, ast::MarkOwnType::anonymous,
							                          isNonNullable, None, isSlice, {token.fileRange, RangeAt(bClose)});
						}
						i = bClose;
						break;
					} else {
						add_error("Could not find " TOKEN_GENERIC_LIST_END " for the end of the mark subtype",
						          RangeAt(i + 1));
					}
				} else {
					if (is_next(TokenType::bracketOpen, i)) {
						add_error("Found [ after " + color_error(isSlice ? "slice" : "mark") +
						              " which is not the correct syntax. The syntax for " +
						              color_error(isSlice ? "slice" : "mark") + " type is " +
						              color_error("mark:[subtype]"),
						          RangeSpan(i, i + 1));
					} else {
						add_error("Subtype of the " + color_error(isSlice ? "slice" : "mark") +
						              " type is not specified. The syntax for a " +
						              color_error(isSlice ? "slice" : "mark") + " type is " +
						              color_error(isSlice ? "slice:[subtype]" : "mark:[subtype]") + " for nullable " +
						              (isSlice ? "slice" : "mark") + " and " +
						              color_error(isSlice ? "slice![subtype]" : "mark![subtype]") +
						              " for non-nullable " + (isSlice ? "slice" : "mark"),
						          token.fileRange);
					}
				}
				break;
			}
			case TokenType::bracketOpen: {
				if (cacheTy.has_value()) {
					return {cacheTy.value(), i - 1};
				}
				auto bClose = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i);
				if (bClose.has_value()) {
					auto lengthExp = do_prerun_expression(preCtx, i, bClose);
					if (lengthExp.second + 1 > bClose.value()) {
						add_error("Array length did not span till ]", RangeSpan(lengthExp.second + 1, bClose.value()));
					}
					auto arrSubTy = do_type(ctx, bClose.value(), None);
					cacheTy =
					    ast::ArrayType::create(arrSubTy.first, lengthExp.first, {RangeAt(i), RangeAt(arrSubTy.second)});
					i = arrSubTy.second;
				} else {
					add_error("Expected end for [", RangeAt(i));
				}
				break;
			}
			default: {
				if (!cacheTy.has_value()) {
					add_error("No type found", RangeAt(from));
				}
				return {cacheTy.value(), i - 1};
			}
		}
	}
	if (!cacheTy.has_value()) {
		add_error("No type found", RangeAt(from));
	}
	return {cacheTy.value(), i - 1};
}

Vec<ast::GenericAbstractType*> Parser::do_generic_abstracts(ParserContext& preCtx, usize from, usize upto) {
	using lexer::TokenType;

	Vec<ast::GenericAbstractType*> result;
	for (usize i = from + 1; i < upto; i++) {
		auto token = tokens->at(i);
		if (token.type == TokenType::identifier) {
			if (is_next(TokenType::assignment, i)) {
				auto typRes = do_type(preCtx, i + 1, None);
				result.push_back(ast::TypedGenericAbstract::create(result.size(), IdentifierAt(i), typRes.first,
				                                                   RangeSpan(i, typRes.second)));
				i = typRes.second;
			} else {
				result.push_back(
				    ast::TypedGenericAbstract::create(result.size(), IdentifierAt(i), None, token.fileRange));
			}
			if (is_next(TokenType::separator, i)) {
				i++;
				continue;
			} else if (is_next(TokenType::genericTypeEnd, i)) {
				break;
			} else {
				add_error("Unexpected token after identifier in generic parameter specification", token.fileRange);
			}
		} else if (token.type == TokenType::pre) {
			auto start = i;
			if (is_next(TokenType::identifier, i)) {
				auto constGenName = IdentifierAt(i + 1);
				if (is_next(TokenType::typeSeparator, i + 1)) {
					if (is_next(TokenType::separator, i + 2) || is_next(TokenType::genericTypeEnd, i + 2)) {
						add_error("Expected type for the prerun expression generic parameter", RangeSpan(i, i + 3));
					}
					auto typeRes = do_type(preCtx, i + 2, None);
					if (typeRes.second >= upto) {
						add_error(
						    "Parsing type for the prerun expression generic parameter surpassed the end of the recognised generic type specification",
						    RangeSpan(i, typeRes.second));
					}
					i = typeRes.second;
					Maybe<ast::PrerunExpression*> defaultValue;
					if (is_next(TokenType::assignment, i)) {
						if (is_next(TokenType::separator, i + 1) || is_next(TokenType::genericTypeEnd, i + 1)) {
							add_error(
							    "Expected a pre-run expression for the default value of the pre-run generic parameter. Found nothing.",
							    RangeSpan(start, i + 2));
						}
						auto constRes = do_prerun_expression(preCtx, i + 1, None);
						if (constRes.second >= upto) {
							add_error(
							    "Parsing pre-run expression for the default value for the pre-run generic parameter surpassed the end of the recognised generic type specification",
							    RangeSpan(start, constRes.second));
						}
						defaultValue = constRes.first;
						i            = constRes.second;
					}
					result.push_back(ast::PrerunGenericAbstract::get(result.size(), constGenName, typeRes.first,
					                                                 defaultValue, RangeSpan(start, i)));
					if (is_next(TokenType::separator, i)) {
						i++;
					} else if (is_next(TokenType::genericTypeEnd, i)) {
						break;
					}
				} else {
					add_error("Expected :: after the name of the prerun generic parameter", RangeSpan(i, i + 1));
				}
			} else {
				add_error("Expected an identifier found after `const` in generic parameter specification", RangeAt(i));
			}
		} else {
			add_error("Unexpected token found in the generic parameter specification", token.fileRange);
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

	Maybe<std::tuple<usize, String, FileRange>> totalComment;
	auto                                        addComment = [&](std::tuple<usize, String, FileRange> commValue) {
        if (totalComment.has_value() && (std::get<0>(totalComment.value()) + 1 == std::get<0>(commValue))) {
            std::get<0>(totalComment.value()) = std::get<0>(commValue);
            std::get<1>(totalComment.value()).append(std::get<1>(commValue));
            std::get<2>(totalComment.value()) = std::get<2>(totalComment.value()).spanTo(std::get<2>(commValue));
        } else {
            totalComment = commValue;
        }
	};

	Vec<ast::Node*> resultNodes;
	auto            addNode = [&](ast::Node* node) {
        if (node->isCommentable() && totalComment.has_value()) {
            node->asCommentable()->commentValue =
                Pair<String, FileRange>{std::get<1>(totalComment.value()), std::get<2>(totalComment.value())};
        }
        resultNodes.push_back(node);
	};

	using lexer::Token;
	using lexer::TokenType;

	if (upto == -1) {
		upto = tokens->size();
	}

	ParserContext thisCtx(preCtx);

	Maybe<CacheSymbol> c_sym           = None;
	auto               setCachedSymbol = [&c_sym](const CacheSymbol& sym) { c_sym = sym; };
	auto               getCachedSymbol = [&c_sym]() {
        auto result = c_sym.value();
        c_sym       = None;
        return result;
	};
	auto hasCachedSymbol = [&c_sym]() { return c_sym.has_value(); };

	std::deque<Token>      cacheT;
	std::deque<ast::Type*> cacheTy;

	Maybe<ast::VisibilitySpec> visibility;
	auto                       setVisibility  = [&](ast::VisibilitySpec kind) { visibility = kind; };
	auto                       get_visibility = [&]() {
        auto res   = visibility;
        visibility = None;
        return res;
	};

	for (usize i = (from + 1); i < upto; i++) {
		Token& token = tokens->at(i);
		switch (token.type) {
			case TokenType::startOfFile:
			case TokenType::endOfFile: {
				break;
			}
			case TokenType::comment: {
				addComment({i, token.value, token.fileRange});
				break;
			}
			case TokenType::Public: {
				auto kindRes = do_visibility_kind(i);
				setVisibility(kindRes.first);
				i = kindRes.second;
				break;
			}
			case TokenType::meta: {
				if (is_next(TokenType::curlybraceOpen, i)) {
					auto bCloseRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1);
					if (bCloseRes.has_value()) {
						addNode(ast::ModInfo::create(do_meta_info(i + 1, bCloseRes.value(), RangeAt(i)),
						                             RangeSpan(i, bCloseRes.value())));
						i = bCloseRes.value();
					} else {
						add_error("Expected end for {", RangeAt(i + 1));
					}
				} else {
					add_error("Expected { to start the module meta information", RangeAt(i));
				}
				break;
			}
			case TokenType::let: {
				auto start = i;
				if (is_next(TokenType::pre, i)) {
					if (not is_next(TokenType::identifier, i + 1)) {
						add_error("Expected an identifier after this for the name of the prerun global value",
						          RangeSpan(i, i + 1));
					}
					auto              name = IdentifierAt(i + 2);
					Maybe<ast::Type*> providedTy;
					if (is_next(TokenType::typeSeparator, i)) {
						auto typRes = do_type(preCtx, i + 1, None);
						providedTy  = typRes.first;
						i           = typRes.second;
					}
					auto entityMeta = do_entity_metadata(preCtx, i, "prerun global value", 0u);
					if (not is_next(TokenType::assignment, i)) {
						add_error(
						    "Expected = after this to provide a value for this prerun global. Prerun global entities always require a value",
						    RangeSpan(start, i));
					}
					i++;
					auto expRes = do_prerun_expression(preCtx, i, None);
					i           = expRes.second;
					if (not is_next(TokenType::stop, i)) {
						add_error("Expected " + color_error(".") +
						              " after this to end the declaration of the prerun global value",
						          RangeSpan(start, i));
					}
					i++;
					auto visibSpec = get_visibility();
					resultNodes.push_back(
					    ast::PrerunGlobal::create(name, providedTy, expRes.first, visibSpec, RangeSpan(start, i)));
					break;
				} else {
					bool isVar = false;
					if (is_next(TokenType::var, i)) {
						isVar = true;
						i++;
					}
					if (is_next(TokenType::identifier, i)) {
						Maybe<ast::Expression*> exp;
						ast::Type*              typ    = nullptr;
						auto                    endRes = first_primary_position(TokenType::stop, i + 1);
						if (!endRes.has_value()) {
							add_error("Expected end for the global declaration", RangeSpan(start, i + 1));
						}
						if (is_next(TokenType::typeSeparator, i + 1)) {
							if (is_primary_within(TokenType::assignment, i + 2, endRes.value())) {
								auto assignPos = first_primary_position(TokenType::assignment, i + 2).value();
								typ            = do_type(preCtx, i + 2, assignPos).first;
								exp            = do_expression(preCtx, None, assignPos, endRes.value()).first;
							} else {
								typ = do_type(preCtx, i + 2, endRes.value()).first;
							}
						} else {
							add_error("Expected :: and the type of the global declaration here",
							          RangeSpan(start, i + 1));
						}
						// FIXME - Support meta info for globals as well
						addNode(ast::GlobalDeclaration::create(IdentifierAt(i + 1), typ, exp, isVar, get_visibility(),
						                                       None, RangeSpan(start, endRes.value())));
						i = endRes.value();
					} else {
						add_error("Expected name for the global declaration",
						          isVar ? FileRange(tokens->at(start).fileRange, tokens->at(start + 1).fileRange)
						                : RangeAt(start));
					}
				}
				break;
			}
			case TokenType::bring: {
				if (is_next(TokenType::StringLiteral, i) ||
				    (is_next(TokenType::colon, i) && is_next(TokenType::identifier, i + 1) &&
				     (ValueAt(i + 2) == "member"))) {
					bool isMember = !is_next(TokenType::StringLiteral, i);
					auto endRes   = first_primary_position(TokenType::stop, i);
					if (endRes.has_value()) {
						addNode(parse_bring_paths(isMember, isMember ? i + 2 : i, endRes.value(), get_visibility(),
						                          RangeAt(i)));
						i = endRes.value();
					} else {
						add_error("Expected end of bring sentence", token.fileRange);
					}
				} else if (is_next(TokenType::identifier, i) || is_next(TokenType::super, i)) {
					if (ValueAt(i + 1) == "std") {
						SHOW("Stdlib request found in parser")
						irCtx->stdLibRequired = true;
					}
					auto endRes = first_primary_position(TokenType::stop, i);
					if (endRes.has_value()) {
						addNode(parse_bring_entities(thisCtx, get_visibility(), i, endRes.value()));
						i = endRes.value();
					} else {
						add_error("Expected . to end the bring sentence", token.fileRange);
					}
				} else if (is_next(TokenType::integerType, i) || is_next(TokenType::unsignedIntegerType, i)) {
					auto start  = i;
					auto endRes = first_primary_position(TokenType::stop, i);
					if (endRes.has_value()) {
						Vec<ast::Type*> types;
						if (is_primary_within(TokenType::separator, i, endRes.value())) {
							while (is_primary_within(TokenType::separator, i, endRes.value())) {
								auto sepPos = first_primary_position(TokenType::separator, i).value();
								auto typRes = do_type(preCtx, i, sepPos);
								if (typRes.second + 1 != sepPos) {
									add_error("Brought type did not span till the ,",
									          RangeSpan(typRes.second + 1, sepPos));
								}
								types.push_back(typRes.first);
								i = sepPos;
							}
							if (i + 1 != endRes.value()) {
								auto typRes = do_type(preCtx, i, endRes.value());
								if (typRes.second + 1 != endRes.value()) {
									add_error("Brought type did not span till the .",
									          RangeSpan(typRes.second + 1, endRes.value()));
								}
								types.push_back(typRes.first);
							}
							i = endRes.value();
						} else {
							auto typRes = do_type(preCtx, i, endRes.value());
							if (typRes.second + 1 != endRes.value()) {
								add_error("Brought type did not span till the .",
								          RangeSpan(typRes.second + 1, endRes.value()));
							}
							types.push_back(typRes.first);
							i = endRes.value();
						}
						addNode(ast::BringBitwidths::create(types, RangeSpan(start, i)));
					} else {
						add_error("Expected . to end the bring sentence", token.fileRange);
					}
				}
				break;
			}
			case TokenType::pre: {
				auto start = i;
				if (is_next(TokenType::identifier, i)) {
						i++;
					}
						}
				} else {
				}
				break;
			}
			case TokenType::Do: {
				if (visibility.has_value()) {
					add_error("Implementations are always public. Please remove the visibility info before this",
					          get_visibility().value().range);
				}
				auto start = i;
				if (is_next(TokenType::Type, i)) {
					auto typeRes = do_type(preCtx, i + 1, None);
					i            = typeRes.second;
					if (is_next(TokenType::curlybraceOpen, i)) {
						auto cClose = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1);
						auto doneSk = ast::DoSkill::create(true, None, typeRes.first, RangeSpan(start, cClose.value()));
						do_type_contents(preCtx, i + 1, cClose.value(), doneSk);
						resultNodes.push_back(doneSk);
						i = cClose.value();
					} else {
						add_error("Expected { after this to start the body of the implementation", RangeSpan(start, i));
					}
				} else {
					add_error("Expected " + color_error("type") + " after this", RangeAt(i));
				}
				break;
			}
			case TokenType::mix: {
				if (is_next(TokenType::identifier, i)) {
					auto mixCtx = ParserContext();
					auto entityMeta =
					    do_entity_metadata(mixCtx, i, "mix type", 0 /** Change this when generics is supported */);
					i = entityMeta.lastIndex;
					if (is_next(TokenType::curlybraceOpen, i + 1)) {
						auto bCloseRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 2);
						if (bCloseRes.has_value()) {
							auto                                     bClose = bCloseRes.value();
							Vec<Pair<Identifier, Maybe<ast::Type*>>> subTypes;
							Vec<FileRange>                           fileRanges;
							Maybe<usize>                             defaultVal;
							parse_mix_type(preCtx, i + 2, bClose, subTypes, fileRanges, defaultVal);
							// FIXME - Support packing
							addNode(ast::DefineMixType::create(IdentifierAt(i + 1), entityMeta.defineChecker,
							                                   entityMeta.genericConstraint, std::move(subTypes),
							                                   std::move(fileRanges), defaultVal, false,
							                                   get_visibility(), RangeSpan(i, bClose)));
							i = bClose;
						} else {
							add_error("Expected end for {", RangeAt(i + 2));
						}
					} else {
						add_error("Expected { to start the definition of the mix type", RangeSpan(i, i + 1));
					}
				} else {
					add_error("Expected an identifier for the name of the mix type", RangeAt(i));
				}
				break;
			}
			case TokenType::choice: {
				auto start = i;
				if (is_next(TokenType::identifier, i)) {
					auto              typeName = IdentifierAt(i + 1);
					Maybe<ast::Type*> providedTy;
					if (is_next(TokenType::typeSeparator, i + 1)) {
						auto curPos = first_primary_position(TokenType::curlybraceOpen, i + 2);
						if (curPos.has_value()) {
							auto typRes = do_type(preCtx, i + 2, curPos);
							providedTy  = typRes.first;
							i           = curPos.value() - 1;
						} else {
							add_error("Expected { after this to start the definition of choice type", RangeAt(i + 2));
						}
					} else {
						i++;
					}
					if (is_next(TokenType::curlybraceOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1);
						if (bCloseRes.has_value()) {
							auto                                                 bClose = bCloseRes.value();
							Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>> fields;
							Maybe<usize>                                         defaultVal;
							do_choice_type(i + 1, bClose, fields, defaultVal);
							addNode(ast::DefineChoiceType::create(typeName, std::move(fields), providedTy, defaultVal,
							                                      get_visibility(), RangeSpan(start, bClose)));
							i = bClose;
						} else {
							add_error("Expected end for {", RangeAt(i + 1));
						}
					} else {
						add_error("Expected { to start the definition of the choice type", RangeSpan(start, i));
					}
				} else {
					add_error("Expected an identifier after this for the name of the choice type", RangeAt(i));
				}
				break;
			}
			case TokenType::region: {
				if (is_next(TokenType::identifier, i)) {
					if (is_next(TokenType::stop, i + 1)) {
						addNode(ast::DefineRegion::create(IdentifierAt(i + 1), get_visibility(), RangeSpan(i, i + 2)));
						i += 2;
					} else {
						add_error("Expected . to end the region declaration", RangeSpan(i, i + 1));
					}
				} else {
					add_error("Expected an identifier for the name of the region", RangeAt(i));
				}
				break;
			}
			case TokenType::define: {
				auto start = i;
				if (is_next(TokenType::identifier, i)) {
					auto name = IdentifierAt(i + 1);

					Vec<ast::GenericAbstractType*> genericList;
					i++;
					auto typeCtx = ParserContext();
					if (is_next(TokenType::genericTypeStart, i)) {
						auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
						if (endRes.has_value()) {
							auto end    = endRes.value();
							genericList = do_generic_abstracts(typeCtx, i + 1, end);
							for (auto* temp : genericList) {
								typeCtx.add_abstract_generic(temp);
							}
							i = end;
						} else {
							add_error("Expected ] to end the generic parameter list", RangeAt(i + 1));
						}
					}
					auto typeMetaData = do_entity_metadata(typeCtx, i, "type definition", genericList.size());
					i                 = typeMetaData.lastIndex;
					if (is_next(TokenType::assignment, i)) {
						auto tyDefStartFrom = i + 1;
						auto endRes         = first_primary_position(TokenType::stop, tyDefStartFrom);
						if (endRes.has_value()) {
							auto* typ = do_type(typeCtx, tyDefStartFrom, endRes.value()).first;
							addNode(ast::TypeDefinition::create(name, typeMetaData.defineChecker, genericList,
							                                    typeMetaData.genericConstraint, typ,
							                                    RangeSpan(start, endRes.value()), get_visibility()));
							i = endRes.value();
							break;
						} else {
							add_error("Expected . to end the type definition", RangeSpan(i, i + 2));
						}
					} else {
						add_error("Expected = after this to start the type definition", RangeSpan(start, i));
					}
				} else {
					add_error("Expected name for the type definition", RangeAt(i));
				}
				break;
			}
			case TokenType::Type: {
				auto start = i;
				if (is_next(TokenType::identifier, i)) {
					auto                           name = IdentifierAt(i + 1);
					Vec<ast::GenericAbstractType*> genericList;
					i++;
					auto typeCtx = ParserContext();
					if (is_next(TokenType::genericTypeStart, i)) {
						auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
						if (endRes.has_value()) {
							auto end    = endRes.value();
							genericList = do_generic_abstracts(typeCtx, i + 1, end);
							for (auto* temp : genericList) {
								typeCtx.add_abstract_generic(temp);
							}
							i = end;
						} else {
							add_error("Expected ] to end the generic parameter list", RangeAt(i + 1));
						}
					}
					auto typeMetaData = do_entity_metadata(typeCtx, i, "struct type", genericList.size());
					i                 = typeMetaData.lastIndex;
					if (is_next(TokenType::curlybraceOpen, i)) {
						auto bClose = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1);
						if (bClose.has_value()) {
							// FIXME - Implement packing
							auto* tRes =
							    ast::DefineCoreType::create(name, typeMetaData.defineChecker, get_visibility(),
							                                {token.fileRange, RangeAt(bClose.value())}, genericList,
							                                typeMetaData.genericConstraint, typeMetaData.metaInfo);
							do_type_contents(typeCtx, i + 1, bClose.value(), tRes);
							addNode(tRes);
							i = bClose.value();
							break;
						} else {
							add_error("Expected } to end the body of the struct type", RangeAt(i + 2));
						}
					} else {
						add_error("Expected { to start the body of the struct type", RangeSpan(start, i));
					}
				} else {
					add_error("Expected name for the struct type", token.fileRange);
				}
				break;
			}
			case TokenType::opaque: {
				auto start = i;
				if (is_next(TokenType::identifier, i)) {
					auto                          visibility = get_visibility();
					auto                          typeName   = IdentifierAt(i + 1);
					Maybe<ast::PrerunExpression*> condition;
					Maybe<ast::MetaInfo>          metaInfo;

					auto entityMeta = do_entity_metadata(preCtx, i + 1, "opaque type", 0);
					i               = entityMeta.lastIndex;
					if (is_next(TokenType::stop, i)) {
						addNode(ast::DefineOpaqueType::create(typeName, condition, visibility, metaInfo,
						                                      RangeSpan(start, i + 1)));
						i++;
						break;
					} else {
						add_error("Expected . to end the declaration of the opaque type", RangeSpan(start, i));
					}
				} else {
					add_error("Expected an identifier for the name of the opaque type", RangeAt(i));
				}
				break;
			}
			case TokenType::lib: {
				if (is_next(TokenType::identifier, i)) {
					if (is_next(TokenType::curlybraceOpen, i + 1)) {
						auto bClose = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 2);
						if (bClose.has_value()) {
							auto contents = parse(thisCtx, i + 2, bClose.value());
							addNode(ast::Lib::create(IdentifierAt(i + 1), contents, get_visibility(),
							                         FileRange(token.fileRange, RangeAt(bClose.value()))));
							i = bClose.value();
						} else {
							add_error("Expected } to close the lib", RangeAt(i + 2));
						}
					} else {
						add_error("Expected { after name for the lib", RangeAt(i + 1));
					}
				} else {
					add_error("Expected name for the lib", token.fileRange);
				}
				break;
			}
			case TokenType::identifier: {
				if (hasCachedSymbol()) {
					auto cachSym = getCachedSymbol();
					add_error("Another symbol " + color_error(cachSym.to_string()) + " found before this",
					          cachSym.fileRange.spanTo(RangeAt(i)));
				}
				auto start   = i;
				auto sym_res = do_symbol(thisCtx, i);
				i            = sym_res.second;
				Vec<ast::GenericAbstractType*> genericList;
				auto                           fnCtx = ParserContext();
				if (is_next(TokenType::genericTypeStart, i)) {
					auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (endRes.has_value()) {
						auto end    = endRes.value();
						genericList = do_generic_abstracts(preCtx, i + 1, end);
						for (auto* temp : genericList) {
							fnCtx.add_abstract_generic(temp);
						}
						if (is_next(TokenType::givenTypeSeparator, end) || is_next(TokenType::parenthesisOpen, end)) {
							i = end;
						} else {
							add_error(
							    "This is assumed to be part of a function. Expected the return type or the argument list of the function",
							    RangeSpan(start, end));
						}
					} else {
						add_error("Expected end for generic type specification", RangeAt(i + 1));
					}
				}
				if (is_next(TokenType::parenthesisOpen, i) || is_next(TokenType::givenTypeSeparator, i)) {
					Maybe<ast::Type*> retType;
					if (is_next(TokenType::givenTypeSeparator, i)) {
						auto typRes = do_type(fnCtx, i + 1, None);
						retType     = typRes.first;
						i           = typRes.second;
					}
					Pair<Vec<ast::Argument*>, bool> argResult = {{}, false};
					if (is_next(TokenType::parenthesisOpen, i)) {
						auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
						if (pCloseRes.has_value()) {
							auto pClose = pCloseRes.value();
							argResult   = do_function_parameters(fnCtx, i + 1, pClose);
							SHOW("Parsed arguments")
							i = pClose;
						} else {
							add_error("Expected end for (", RangeAt(i + 1));
						}
					}
					auto protoEnd   = i;
					auto entityMeta = do_entity_metadata(
					    preCtx, i, genericList.empty() ? "function" : "generic function", genericList.size());
					i = entityMeta.lastIndex;
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
						if (bCloseRes.has_value()) {
							auto bClose    = bCloseRes.value();
							auto sentences = do_sentences(fnCtx, i + 1, bClose);
							addNode(ast::FunctionPrototype::create(
							    IdentifierAt(start), argResult.first, argResult.second, retType,
							    entityMeta.defineChecker, entityMeta.genericConstraint, entityMeta.metaInfo,
							    get_visibility(),
							    RangeSpan((is_previous(TokenType::identifier, start) ? start - 1 : start), protoEnd),
							    genericList, Pair<Vec<ast::Sentence*>, FileRange>(sentences, RangeSpan(i, bClose))));
							i = bClose;
							break;
						} else {
							// NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
							add_error("Expected end for [", RangeAt(i + 1));
						}
					} else if (is_next(TokenType::stop, i)) {
						addNode(ast::FunctionPrototype::create(
						    IdentifierAt(start), argResult.first, argResult.second, retType, entityMeta.defineChecker,
						    entityMeta.genericConstraint, entityMeta.metaInfo, get_visibility(),
						    RangeSpan((is_previous(TokenType::identifier, start) ? start - 1 : start), protoEnd),
						    genericList, None));
						i++;
						break;
					} else {
						add_error(
						    "Expected either [ to start the definition of the function or . to end the declaration",
						    RangeAt(i));
					}
				} else {
					setCachedSymbol(sym_res.first);
				}
				break;
			}
			case TokenType::givenTypeSeparator: {
				if (!hasCachedSymbol()) {
					add_error("Function name not provided", token.fileRange);
				}
				auto  retTypeRes = do_type(thisCtx, i, None);
				auto* retType    = retTypeRes.first;
				i                = retTypeRes.second;
				if (is_next(TokenType::parenthesisOpen, i)) {
					auto pCloseResult = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
					if (!pCloseResult.has_value()) {
						add_error("Expected )", RangeAt(i + 1));
					}
					auto pClose    = pCloseResult.value();
					auto argResult = do_function_parameters(thisCtx, i + 1, pClose);
					i              = pClose;
					auto meta      = do_entity_metadata(preCtx, i, "function", 0);
					auto cacheSym  = getCachedSymbol();
					if (cacheSym.name.size() > 1) {
						add_error("Function name should be just one identifier", cacheSym.fileRange);
					}
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseResult = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, pClose + 1);
						if (!bCloseResult.has_value() || (bCloseResult.value() >= tokens->size())) {
							add_error("Expected ] to end the function definition", RangeAt(pClose + 1));
						}
						auto bClose    = bCloseResult.value();
						auto sentences = do_sentences(thisCtx, pClose + 1, bClose);
						addNode(ast::FunctionPrototype::create(
						    cacheSym.name.front(), argResult.first, argResult.second, retType, meta.defineChecker,
						    nullptr, meta.metaInfo, get_visibility(),
						    FileRange{RangeAt(cacheSym.tokenIndex), token.fileRange}, {},
						    Pair<Vec<ast::Sentence*>, FileRange>(sentences, RangeSpan(i, bClose))));
						i = bClose;
						continue;
					} else if (is_next(TokenType::stop, i)) {
						addNode(ast::FunctionPrototype::create(
						    cacheSym.name.front(), argResult.first, argResult.second, retType, meta.defineChecker,
						    nullptr, meta.metaInfo, get_visibility(),
						    FileRange{RangeAt(cacheSym.tokenIndex), token.fileRange}, {}, None));
						i++;
					} else {
						add_error(
						    "Expected either [ to start the definition of the function or . to end the declaration",
						    RangeAt(i));
					}
				} else {
					add_error("Expected (", token.fileRange);
				}
				break;
			}
			default: {
				add_error("Unexpected token", token.fileRange);
			}
		}
	}
	parseRecurseCount--;
	if (parseRecurseCount == 0) {
		auto parseEndTime = std::chrono::high_resolution_clock::now();
		SHOW("Parse end time: " << parseEndTime.time_since_epoch().count())
		timeInMicroSeconds +=
		    std::chrono::duration_cast<std::chrono::microseconds>(parseEndTime - latestStartTime).count();
		tokenCount += tokens->size();
	}
	return resultNodes;
}

Pair<ast::VisibilitySpec, usize> Parser::do_visibility_kind(usize from) {
	using lexer::TokenType;
	if (is_next(TokenType::colon, from)) {
		if (is_next(TokenType::Type, from + 1)) {
			return {{VisibilityKind::type, RangeSpan(from, from + 2)}, from + 2};
		} else if (is_next(TokenType::skill, from + 1)) {
			return {{VisibilityKind::skill, RangeSpan(from, from + 2)}, from + 2};
		} else if (is_next(TokenType::lib, from + 1)) {
			return {{VisibilityKind::lib, RangeSpan(from, from + 2)}, from + 2};
		} else if (is_next(TokenType::identifier, from + 1)) {
			auto val = ValueAt(from + 2);
			if (val == "file") {
				return {{VisibilityKind::file, RangeSpan(from, from + 2)}, from + 2};
			} else if (val == "folder") {
				return {{VisibilityKind::folder, RangeSpan(from, from + 2)}, from + 2};
			} else {
				add_error("Invalid identifier found after " + color_error("pub:") +
				              " for visibility. Expected either " + color_error("pub:file") + " or " +
				              color_error("pub:folder"),
				          RangeAt(from + 2));
			}
		} else {
			add_error("Invalid token found after " + color_error("pub:") + " for visibility",
			          RangeSpan(from, from + 1));
		}
	} else {
		return {{VisibilityKind::pub, RangeAt(from)}, from};
	}
} // NOLINT(clang-diagnostic-return-type)

// FIXME - Finish functionality for parsing type contents
void Parser::do_type_contents(ParserContext& preCtx, usize from, usize upto, ast::MemberParentLike* memParent) {
	using lexer::Token;
	using lexer::TokenType;

	bool finishedMemberList    = false;
	bool foundFirstMember      = false;
	bool haveNonMemberEntities = false;

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

	Maybe<Pair<ast::VisibilitySpec, FileRange>> visibility;
	auto setVisibility = [&](Pair<ast::VisibilitySpec, FileRange> kind) { visibility = kind; };
	auto hasVisibility = [&]() { return visibility.has_value(); };
	auto getVisibility = [&]() {
		auto res   = visibility;
		visibility = None;
		return res;
	};
	auto getVisibSpec = [](Maybe<Pair<ast::VisibilitySpec, FileRange>> visibValue) -> Maybe<ast::VisibilitySpec> {
		if (visibValue.has_value()) {
			return visibValue->first;
		} else {
			return None;
		}
	};
	auto fromVisibRange = [](Maybe<Pair<ast::VisibilitySpec, FileRange>> visibValue, FileRange otherRange) {
		if (visibValue.has_value()) {
			return FileRange{visibValue->second, otherRange};
		} else {
			return otherRange;
		}
	};

	for (usize i = from + 1; i < upto; i++) {
		Token& token = tokens->at(i);
		switch (token.type) {
			case TokenType::Public: {
				if (visibility.has_value()) {
					add_error("There is already a visibility specifier before this, that is unused", RangeAt(i));
				}
				auto kindRes = do_visibility_kind(i);
				setVisibility({kindRes.first, RangeSpan(i, kindRes.second)});
				i = kindRes.second;
				break;
			}
				//   case TokenType::constant: {
				//     setConst();
				//     tokens->at(i);
				//     break;
				//   }
			case TokenType::Static: {
				if (!is_next(TokenType::identifier, i)) {
					add_error("Unexpected token after static", RangeAt(i));
				}
				setStatic();
				break;
			}
			case TokenType::var: {
				if (is_next(TokenType::colon, i)) {
					if (!is_next(TokenType::identifier, i + 1) && !is_next(TokenType::Operator, i + 1)) {
						add_error("Expected " + color_error("operator") +
						              " or the identifier for the name of the method after " + color_error("var:"),
						          RangeAt(i));
					}
					setVariation();
					i++;
				} else {
					add_error("Expected : after " + color_error("var"), RangeAt(i));
				}
				break;
			}
			case TokenType::selfWord: {
				auto start = i;
				if (is_next(TokenType::colon, i)) {
					if (is_next(TokenType::identifier, i + 1)) {
						auto fnName = IdentifierAt(i + 2);
						i           = i + 2;
						if (is_next(TokenType::givenTypeSeparator, i) || is_next(TokenType::parenthesisOpen, i)) {
							if (foundFirstMember && !finishedMemberList) {
								add_error("The list of member fields in this type has not been finalised. Please add " +
								              color_error(".") + " after the last member field to terminate the list",
								          RangeSpan(start, i + 1));
							}
							haveNonMemberEntities = true;
							Maybe<ast::Type*> retTy;
							if (is_next(TokenType::givenTypeSeparator, i)) {
								auto typeRes = do_type(preCtx, i + 1, None);
								retTy        = typeRes.first;
								i            = typeRes.second;
							}
							Pair<Vec<ast::Argument*>, bool> argsRes = {{}, false};
							if (is_next(TokenType::parenthesisOpen, i)) {
								auto pCloseRes =
								    get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
								if (pCloseRes.has_value()) {
									auto pClose = pCloseRes.value();
									argsRes     = do_function_parameters(preCtx, i + 1, pClose);
									i           = pClose;
								} else {
									add_error("Expected end for (", RangeAt(i + 1));
								}
							}
							auto entityMeta = do_entity_metadata(preCtx, i, "method",
							                                     0 /** TODO: Change to support generic methods */);
							i               = entityMeta.lastIndex;
							if (is_next(TokenType::bracketOpen, i)) {
								auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
								if (bCloseRes.has_value()) {
									auto bClose = bCloseRes.value();
									auto snts   = do_sentences(preCtx, i + 1, bClose);
									SHOW("Creating member function prototype")
									auto memVisib = getVisibility();
									memParent->add_method_definition(ast::MethodDefinition::create(
									    ast::MethodPrototype::Value(fnName, entityMeta.defineChecker, argsRes.first,
									                                argsRes.second, retTy, entityMeta.metaInfo,
									                                getVisibSpec(memVisib),
									                                fromVisibRange(memVisib, RangeSpan(start, i))),
									    snts, RangeSpan(start, bClose)));
									i = bClose;
								} else {
									add_error("Expected ] to end the function body", RangeAt(i + 1));
								}
							} else {
								add_error("Expected [ to start the function body", RangeSpan(start, i));
							}
						} else {
							add_error(
							    "Expected either -> before the given type or ( to start the argument list of the value method",
							    RangeSpan(i, i + 2));
						}
					} else {
						add_error("Expected identifier after " + color_error("self:"), RangeSpan(i, i + 1));
					}
				} else {
					add_error("Expected self:function_name here", RangeAt(i));
				}
				break;
			}
			case TokenType::identifier: {
				SHOW("Identifier inside core type: " << token.value)
				auto start = i;
				if (is_next(TokenType::givenTypeSeparator, i) || is_next(TokenType::parenthesisOpen, i)) {
					if (foundFirstMember && !finishedMemberList) {
						add_error("The list of member fields in this type has not been finalised. Please add " +
						              color_error(".") + " after the last member field to terminate the list",
						          RangeSpan(start, i + 1));
					}
					haveNonMemberEntities = true;
					SHOW("Member function start")
					Maybe<ast::Type*> retTy;
					if (is_next(TokenType::givenTypeSeparator, i)) {
						auto typeRes = do_type(preCtx, i + 1, None);
						retTy        = typeRes.first;
						i            = typeRes.second;
					}
					Pair<Vec<ast::Argument*>, bool> argsRes = {{}, false};
					if (is_next(TokenType::parenthesisOpen, i)) {
						auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
						if (pCloseRes.has_value()) {
							auto pClose = pCloseRes.value();
							argsRes     = do_function_parameters(preCtx, i + 1, pClose);
							i           = pClose;
						} else {
							add_error("Expected end for (", RangeAt(i + 1));
						}
					}
					auto meta = do_entity_metadata(preCtx, i, "method", 0 /** TODO - Generic methods */);
					i         = meta.lastIndex;
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
						if (bCloseRes.has_value()) {
							auto bClose = bCloseRes.value();
							auto snts   = do_sentences(preCtx, i + 1, bClose);
							SHOW("Creating member function prototype")
							auto memVisib = getVisibility();
							memParent->add_method_definition(ast::MethodDefinition::create(
							    getStatic()
							        ? ast::MethodPrototype::Static(IdentifierAt(start), meta.defineChecker,
							                                       argsRes.first, argsRes.second, retTy,
							                                       std::move(meta.metaInfo), getVisibSpec(memVisib),
							                                       fromVisibRange(memVisib, RangeSpan(start, i)))
							        : ast::MethodPrototype::Normal(
							              getVariation(), IdentifierAt(start), meta.defineChecker, argsRes.first,
							              argsRes.second, retTy, std::move(meta.metaInfo), getVisibSpec(memVisib),
							              fromVisibRange(memVisib, RangeSpan(start, i))),
							    snts, RangeSpan(start, bClose)));
							i = bClose;
						} else {
							add_error("Expected ] to end the function body", RangeAt(i + 1));
						}
					} else {
						add_error("Expected [ to start the function body", RangeSpan(start, i));
					}
				} else if (is_next(TokenType::typeSeparator, i)) {
					if (!memParent->is_define_core_type()) {
						add_error("Member fields are only allowed for struct types", RangeSpan(start, i + 1));
					}
					if (finishedMemberList) {
						add_error(
						    "The member list has already been finalised. Cannot add a member field after terminating the list. Did you add a " +
						        color_error(".") + " accidentally after the previous member field?",
						    RangeSpan(i, i + 1));
					}
					if (haveNonMemberEntities) {
						add_error(
						    "The member fields of the type is expected to be provided at the beginning of the type to improve clarity",
						    RangeSpan(i, i + 1));
					}
					auto                    memName  = IdentifierAt(i);
					auto                    memVisib = getVisibility();
					auto                    memVar   = not getConst();
					ast::Type*              memType  = nullptr;
					Maybe<ast::Expression*> memValue;
					auto                    typeRes = do_type(preCtx, i + 1, None);
					memType                         = typeRes.first;
					if (is_next(TokenType::assignment, typeRes.second)) {
						auto expRes = do_prerun_expression(preCtx, typeRes.second + 1, None);
						memValue    = expRes.first;
						i           = expRes.second;
					} else {
						i = typeRes.second;
					}
					if (is_next(TokenType::stop, i)) {
						finishedMemberList = true;
						i++;
					} else if (is_next(TokenType::separator, i)) {
						i++;
					} else {
						add_error("Expected either " + color_error(",") + " to end this member field or " +
						              color_error(".") + " to end the list of member fields",
						          RangeSpan(start, i));
					}
					SHOW("Adding member")
					memParent->as_define_core_type()->addMember(
					    ast::DefineCoreType::Member::create(memType, memName, memVar, getVisibSpec(memVisib), memValue,
					                                        fromVisibRange(memVisib, RangeSpan(start, i))));
				} else {
					add_error("Unexpected identifier found inside core type", RangeAt(i));
				}
				break;
			}
			case TokenType::Default: {
				if (foundFirstMember && !finishedMemberList) {
					add_error("The list of member fields in this type has not been finalised. Please add " +
					              color_error(".") + " after the last member field to terminate the list",
					          RangeAt(i));
				}
				haveNonMemberEntities = true;
				auto start            = i;
				if (memParent->has_default_constructor()) {
					add_error("A default destructor is already defined for the core type", RangeAt(i));
				}
				auto entMeta = do_entity_metadata(preCtx, i, "default constructor", 0);
				i            = entMeta.lastIndex;
				if (is_next(TokenType::bracketOpen, i)) {
					auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
					if (bCloseRes.has_value()) {
						auto bClose   = bCloseRes.value();
						auto snts     = do_sentences(preCtx, i + 1, bClose);
						auto defVisib = getVisibility();
						memParent->set_default_constructor(ast::ConstructorDefinition::create(
						    ast::ConstructorPrototype::Default(getVisibSpec(defVisib), RangeAt(start),
						                                       fromVisibRange(defVisib, RangeAt(i)),
						                                       entMeta.defineChecker, std::move(entMeta.metaInfo)),
						    std::move(snts), RangeSpan(i + 1, bClose)));
						i = bClose;
					} else {
						add_error("Expected end for [", RangeAt(i + 1));
					}
				} else {
					add_error("Expected [ to start the definition of the default constructor", RangeAt(i));
				}
				break;
			}
			case TokenType::copy: {
				if (foundFirstMember && !finishedMemberList) {
					add_error("The list of member fields in this type has not been finalised. Please add " +
					              color_error(".") + " after the last member field to terminate the list",
					          RangeAt(i));
				}
				haveNonMemberEntities = true;
				auto start            = i;
				if (is_next(TokenType::assignment, i)) {
					if (memParent->has_copy_assignment()) {
						add_error("The copy assignment operator is already defined for this struct type",
						          RangeSpan(i, i + 1));
					}
					if (is_next(TokenType::identifier, i + 1)) {
						auto argName = IdentifierAt(i + 2);
						i            = i + 2;
						auto entMeta = do_entity_metadata(preCtx, i, "copy assignment operator", 0);
						if (is_next(TokenType::bracketOpen, i)) {
							auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
							if (bCloseRes.has_value()) {
								auto bClose  = bCloseRes.value();
								auto snts    = do_sentences(preCtx, i + 1, bClose);
								auto opVisib = getVisibility();
								memParent->set_copy_assignment(ast::OperatorDefinition::create(
								    ast::OperatorPrototype::create(
								        true, ast::Op::copyAssignment, RangeSpan(start, start + 1), {}, nullptr,
								        getVisibSpec(opVisib), fromVisibRange(opVisib, RangeSpan(start, i)),
								        std::move(argName), entMeta.defineChecker, std::move(entMeta.metaInfo)),
								    std::move(snts), RangeSpan(start, bClose)));
								i = bClose;
							} else {
								add_error("Expected ] to end the function body, but could not find any",
								          RangeAt(i + 1));
							}
						} else {
							add_error("Expected [ to start the body of the copy assignment operator after this",
							          RangeSpan(start, i));
						}
					} else {
						add_error(
						    "Expected an identifier after this for the argument name of the copy assignment operator",
						    RangeSpan(start, i + 1));
					}
				} else if (is_next(TokenType::identifier, i)) {
					if (memParent->has_copy_constructor()) {
						add_error("A copy constructor is already defined for the struct type", RangeAt(i));
					}
					auto argName = IdentifierAt(i + 1);
					i++;
					auto meta = do_entity_metadata(preCtx, i, "copy constructor", 0);
					i         = meta.lastIndex;
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
						if (bCloseRes.has_value()) {
							auto bClose   = bCloseRes.value();
							auto snts     = do_sentences(preCtx, i + 1, bClose);
							auto memVisib = getVisibility();
							memParent->set_copy_constructor(ast::ConstructorDefinition::create(
							    ast::ConstructorPrototype::Copy(getVisibSpec(memVisib), RangeAt(start),
							                                    fromVisibRange(memVisib, RangeAt(i)), argName,
							                                    meta.defineChecker, std::move(meta.metaInfo)),
							    std::move(snts), RangeSpan(i + 1, bClose)));
							i = bClose;
						} else {
							add_error("Expected end for [", RangeAt(i + 2));
						}
					} else {
						add_error("Expected [ to start the definition of the copy constructor", RangeSpan(i, i + 1));
					}
				} else {
					add_error("Expected " + color_error("copy other") + " for the copy constructor or " +
					              color_error("copy = other") + " for the copy assignment, but could not find both",
					          RangeAt(i));
				}
				break;
			}
			case TokenType::move: {
				auto start = i;
				if (is_next(TokenType::assignment, i)) {
					if (memParent->has_move_assignment()) {
						add_error("The move assignment operator is already defined for this struct type",
						          RangeSpan(i, i + 1));
					}
					if (is_next(TokenType::identifier, i + 1)) {
						auto argName = IdentifierAt(i + 2);
						i            = i + 2;
						auto entMeta = do_entity_metadata(preCtx, i, "move assignment operator", 0);
						if (is_next(TokenType::bracketOpen, i)) {
							auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
							if (bCloseRes.has_value()) {
								auto bClose  = bCloseRes.value();
								auto snts    = do_sentences(preCtx, i + 1, bClose);
								auto opVisib = getVisibility();
								memParent->set_move_assignment(ast::OperatorDefinition::create(
								    ast::OperatorPrototype::create(
								        true, ast::Op::moveAssignment, RangeSpan(start, start + 1), {}, nullptr,
								        getVisibSpec(opVisib), fromVisibRange(opVisib, RangeSpan(start, i)),
								        std::move(argName), entMeta.defineChecker, std::move(entMeta.metaInfo)),
								    std::move(snts), RangeSpan(start, bClose)));
								i = bClose;
							} else {
								add_error("Expected ] to end the function body, but could not find any",
								          RangeAt(i + 1));
							}
						} else {
							add_error("Expected [ to start the body of the move assignment operator after this",
							          RangeSpan(start, i));
						}
					} else {
						add_error(
						    "Expected an identifier after this for the argument name of the move assignment operator",
						    RangeSpan(start, i + 1));
					}
				} else if (is_next(TokenType::identifier, i)) {
					if (memParent->has_move_constructor()) {
						add_error("A move constructor is already defined for the struct type", RangeAt(i));
					}
					auto argName = IdentifierAt(i + 1);
					i++;
					auto meta = do_entity_metadata(preCtx, i, "move constructor", 0);
					i         = meta.lastIndex;
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 2);
						if (bCloseRes.has_value()) {
							auto bClose    = bCloseRes.value();
							auto snts      = do_sentences(preCtx, i + 2, bClose);
							auto moveVisib = getVisibility();
							memParent->set_move_constructor(ast::ConstructorDefinition::create(
							    ast::ConstructorPrototype::Move(getVisibSpec(moveVisib), RangeAt(start),
							                                    fromVisibRange(moveVisib, RangeAt(i)), argName,
							                                    meta.defineChecker, std::move(meta.metaInfo)),
							    std::move(snts), RangeSpan(i + 1, bClose)));
							i = bClose;
						} else {
							add_error("Expected end for [", RangeAt(i + 2));
						}
					} else {
						add_error("Expected [ to start the definition of the move constructor", RangeSpan(i, i + 1));
					}
				} else {
					add_error("Expected name for the argument, which represents the existing "
					          "instance of the type, after move keyword",
					          RangeAt(i));
				}
				break;
			}
			case TokenType::from: {
				if (foundFirstMember && !finishedMemberList) {
					add_error("The list of member fields in this type has not been finalised. Please add " +
					              color_error(".") + " after the last member field to terminate the list",
					          RangeAt(i));
				}
				haveNonMemberEntities = true;
				auto start            = i;
				if (is_next(TokenType::parenthesisOpen, i)) {
					auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
					if (pCloseRes.has_value()) {
						auto pClose  = pCloseRes.value();
						auto argsRes = do_function_parameters(preCtx, i + 1, pClose);
						auto meta    = do_entity_metadata(preCtx, pClose,
                                                       (argsRes.first.size() == 1) ? "convertor" : "constructor",
						                                  0 /** TODO - Change this? */);
						if (is_next(TokenType::bracketOpen, pClose)) {
							auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, pClose + 1);
							if (bCloseRes.has_value()) {
								auto bClose   = bCloseRes.value();
								auto snts     = do_sentences(preCtx, pClose + 1, bClose);
								auto memVisib = getVisibility();
								if (argsRes.first.size() == 1) {
									// NOTE - Convertor
									memParent->add_convertor_definition(ast::ConvertorDefinition::create(
									    ast::ConvertorPrototype::create_from(
									        RangeAt(start), argsRes.first.front()->get_name(),
									        argsRes.first.front()->get_type(), argsRes.first.front()->is_member_arg(),
									        getVisibSpec(memVisib), fromVisibRange(memVisib, RangeSpan(start, pClose)),
									        meta.defineChecker, std::move(meta.metaInfo)),
									    snts, RangeSpan(start, bClose)));
								} else {
									// NOTE = Constructor
									memParent->add_constructor_definition(ast::ConstructorDefinition::create(
									    ast::ConstructorPrototype::Normal(
									        RangeAt(start), std::move(argsRes.first), getVisibSpec(memVisib),
									        fromVisibRange(memVisib, RangeSpan(start, pClose)), meta.defineChecker,
									        std::move(meta.metaInfo)),
									    snts, RangeSpan(pClose + 1, bClose)));
								}
								i = bClose;
							} else {
								add_error("Expected end for [", RangeAt(pClose + 1));
							}
						} else {
							add_error("Expected definition for constructor", RangeSpan(start, pClose));
						}
					} else {
						add_error("Expected end for (", RangeAt(i + 1));
					}
				}
				break;
			}
			case TokenType::Operator: {
				if (foundFirstMember && !finishedMemberList) {
					add_error("The list of member fields in this type has not been finalised. Please add " +
					              color_error(".") + " after the last member field to terminate the list",
					          RangeAt(i));
				}
				haveNonMemberEntities     = true;
				auto                start = i;
				String              opr;
				bool                isUnary  = false;
				ast::Type*          returnTy = ast::VoidType::create(FileRange{"", {0u, 0u}, {0u, 0u}});
				Vec<ast::Argument*> args;
				if (is_next(TokenType::binaryOperator, i)) {
					SHOW("Binary operator for core type: " << ValueAt(i + 1))
					if (ValueAt(i + 1) == "-") {
						if (is_next(TokenType::parenthesisOpen, i) && is_next(TokenType::parenthesisClose, i + 1)) {
							isUnary = true;
						} else if (is_next(TokenType::givenTypeSeparator, i)) {
							auto typRes = do_type(preCtx, i + 1, None);
							returnTy    = typRes.first;
							i           = typRes.second;
							if (is_next(TokenType::parenthesisOpen, i) && is_next(TokenType::parenthesisClose, i + 1)) {
								isUnary = true;
							}
						}
					}
					opr = ValueAt(i + 1);
					i++;
				} else if (is_next(TokenType::unaryOperator, i)) {
					isUnary = true;
					opr     = ValueAt(i + 1);
					i++;
				} else if (is_next(TokenType::referenceType, i)) {
					isUnary = true;
					opr     = "@";
					i++;
				} else if (is_next(TokenType::assignment, i)) {
					opr = "=";
					i++;
				} else if (is_next(TokenType::bracketOpen, i)) {
					if (is_next(TokenType::bracketClose, i + 1)) {
						opr = "[]";
						i += 2;
					} else {
						add_error("[ is an invalid operator. Did you mean []", RangeSpan(start, start + 1));
					}
				} else {
					add_error("Invalid operator", RangeAt(i));
				}
				if (is_next(TokenType::givenTypeSeparator, i)) {
					auto typRes = do_type(preCtx, i + 1, None);
					returnTy    = typRes.first;
					i           = typRes.second;
				}
				if (is_next(TokenType::parenthesisOpen, i)) {
					auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
					if (pCloseRes.has_value()) {
						auto pClose    = pCloseRes.value();
						auto fnArgsRes = do_function_parameters(preCtx, i + 1, pClose);
						if (fnArgsRes.second) {
							add_error("Operator functions cannot have variadic arguments", RangeSpan(start, pClose));
						}
						args = fnArgsRes.first;
						i    = pClose;
					} else {
						add_error("Expected end for (", RangeAt(i + 1));
					}
				}
				auto meta       = do_entity_metadata(preCtx, i, "operator function", 0 /** TODO - Generic method? */);
				i               = meta.lastIndex;
				auto  memVisib  = getVisibility();
				auto* prototype = ast::OperatorPrototype::create(
				    getVariation(),
				    isUnary ? (opr == "-" ? ast::Op::minus : ast::operator_from_string(opr))
				            : ast::operator_from_string(opr),
				    RangeAt(start), args, returnTy, getVisibSpec(memVisib),
				    fromVisibRange(memVisib, RangeSpan(start, i)), None, meta.defineChecker, std::move(meta.metaInfo));
				if (is_next(TokenType::bracketOpen, i)) {
					auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
					if (bCloseRes.has_value()) {
						auto bClose = bCloseRes.value();
						auto snts   = do_sentences(preCtx, i + 1, bClose);
						memParent->add_operator_definition(
						    ast::OperatorDefinition::create(prototype, snts, RangeSpan(start, bClose)));
						i = bClose;
					} else {
						add_error("Expected end for [", RangeAt(i + 1));
					}
				} else {
					add_error("Expected definition for the operator", RangeSpan(start, i));
				}
				break;
			}
			case TokenType::to: {
				if (foundFirstMember && !finishedMemberList) {
					add_error("The list of member fields in this type has not been finalised. Please add " +
					              color_error(".") + " after the last member field to terminate the list",
					          RangeAt(i));
				}
				haveNonMemberEntities = true;
				auto start            = i;
				auto typRes           = do_type(preCtx, i, None);
				i                     = typRes.second;
				auto meta             = do_entity_metadata(preCtx, i, "to convertor", 0);
				i                     = meta.lastIndex;
				if (is_next(TokenType::bracketOpen, i)) {
					auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
					if (bCloseRes.has_value()) {
						auto bClose   = bCloseRes.value();
						auto snts     = do_sentences(preCtx, i + 1, bClose);
						auto memVisib = getVisibility();
						memParent->add_convertor_definition(ast::ConvertorDefinition::create(
						    ast::ConvertorPrototype::create_to(RangeAt(start), typRes.first, getVisibSpec(memVisib),
						                                       RangeSpan(start, i), meta.defineChecker,
						                                       std::move(meta.metaInfo)),
						    std::move(snts), fromVisibRange(memVisib, RangeSpan(start, bClose))));
						i = bClose;
						break;
					} else {
						add_error("Expected ] to end body of the convertor", RangeAt(i + 1));
					}
				} else {
					add_error("Expected [ after this to start the definition of the convertor", RangeSpan(start, i));
				}
				break;
			}
			case TokenType::end: {
				if (foundFirstMember && !finishedMemberList) {
					add_error("The list of member fields in this type has not been finalised. Please add " +
					              color_error(".") + " after the last member field to terminate the list",
					          RangeAt(i));
				}
				haveNonMemberEntities = true;
				auto start            = i;
				if (memParent->has_destructor()) {
					add_error("A destructor is already defined for the struct type", RangeAt(i));
				}
				auto meta = do_entity_metadata(preCtx, i, "destructor", 0);
				i         = meta.lastIndex;
				if (is_next(TokenType::bracketOpen, i)) {
					auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
					if (bCloseRes) {
						auto snts     = do_sentences(preCtx, i + 1, bCloseRes.value());
						auto memVisib = getVisibility();
						memParent->set_destructor_definition(ast::DestructorDefinition::create(
						    RangeAt(start), meta.defineChecker, std::move(meta.metaInfo), snts,
						    fromVisibRange(memVisib, RangeSpan(i, bCloseRes.value()))));
						i = bCloseRes.value();
					} else {
						add_error("Expected end for [", RangeAt(i + 1));
					}
				} else {
					add_error("Expected definition of destructor", RangeAt(i));
				}
				break;
			}
			case TokenType::comment: {
				break;
			}
			default: {
				add_error("Unexpected token found here", RangeAt(i));
			}
		}
	}
}

void Parser::parse_mix_type(ParserContext& preCtx, usize from, usize upto,
                            Vec<Pair<Identifier, Maybe<ast::Type*>>>& uRef, Vec<FileRange>& fileRanges,
                            Maybe<usize>& defaultVal) {
	using lexer::TokenType;

	for (auto i = from + 1; i < upto; i++) {
		auto& token = tokens->at(i);
		switch (token.type) {
			case TokenType::Default: {
				if (!defaultVal.has_value()) {
					if (is_next(TokenType::identifier, i)) {
						defaultVal = uRef.size();
						break;
					} else {
						add_error("Invalid token found after default", RangeAt(i));
					}
				} else {
					add_error("Default value for mix type already provided", RangeAt(i));
				}
			}
			case TokenType::identifier: {
				auto start = i;
				if (is_next(TokenType::separator, i) || (is_next(TokenType::curlybraceClose, i) && (i + 1 == upto))) {
					uRef.push_back(Pair<Identifier, Maybe<ast::Type*>>(IdentifierAt(start), None));
					fileRanges.push_back(is_previous(TokenType::Default, start)
					                         ? FileRange(tokens->at(start - 1).fileRange, tokens->at(start).fileRange)
					                         : RangeAt(start));
					i++;
				} else if (is_next(TokenType::typeSeparator, i)) {
					ast::Type* typ;
					if (is_primary_within(TokenType::separator, i + 1, upto)) {
						auto sepPos = first_primary_position(TokenType::separator, i + 1).value();
						typ         = do_type(preCtx, i + 1, sepPos).first;
						i           = sepPos;
					} else {
						typ = do_type(preCtx, i + 1, upto).first;
						i   = upto;
					}
					uRef.push_back(Pair<Identifier, Maybe<ast::Type*>>(IdentifierAt(start), typ));
					fileRanges.push_back(is_previous(TokenType::Default, start)
					                         ? FileRange(tokens->at(start - 1).fileRange, tokens->at(start).fileRange)
					                         : RangeAt(start));
				} else {
					add_error("Invalid token found after identifier in mix type definition", RangeAt(i));
				}
				break;
			}
			default: {
				add_error("Invalid token found after identifier in mix type definition", RangeAt(i));
			}
		}
	}
}

void Parser::do_choice_type(usize from, usize upto, Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>>& fields,
                            Maybe<usize>& defaultVal) {
	using lexer::TokenType;

	for (usize i = from + 1; i < upto; i++) {
		auto& token = tokens->at(i);
		switch (token.type) {
			case TokenType::Default: {
				if (!defaultVal.has_value()) {
					if (is_next(TokenType::identifier, i)) {
						defaultVal = fields.size();
						break;
					} else {
						add_error("Invalid token found after default", RangeAt(i));
					}
				} else {
					add_error("Default value for choice type already provided", RangeAt(i));
				}
			}
			case TokenType::identifier: {
				auto start = i;
				if (is_next(TokenType::separator, i) || (i + 1 == upto)) {
					fields.push_back(Pair<Identifier, Maybe<ast::PrerunExpression*>>(
					    {ValueAt(i), is_previous(TokenType::Default, i)
					                     ? FileRange(tokens->at(i - 1).fileRange, tokens->at(i).fileRange)
					                     : RangeAt(i)},
					    None));
					i++;
				} else if (is_next(TokenType::assignment, i)) {
					auto fieldName = ValueAt(i);

					Maybe<ast::PrerunExpression*> val;

					auto preCtx = ParserContext();
					auto valRes = do_prerun_expression(preCtx, i + 1, None);
					val         = valRes.first;
					i           = valRes.second;
					if (is_next(TokenType::separator, i) ||
					    (is_next(TokenType::curlybraceClose, i) && (i + 1 == upto))) {
						fields.push_back(Pair<Identifier, Maybe<ast::PrerunExpression*>>(
						    Identifier{ValueAt(start),
						               is_previous(TokenType::Default, start)
						                   ? FileRange(tokens->at(start - 1).fileRange, tokens->at(start).fileRange)
						                   : RangeAt(start)},
						    val));
						i++;
					} else {
						add_error("Invalid token found after integer value for the choice "
						          "variant",
						          RangeAt(start));
					}
				}
				break;
			}
			case TokenType::comment: {
				// FIXME - Support comment association with fields
				break;
			}
			default: {
				add_error("Invalid token found inside choice type definition", RangeAt(i));
			}
		}
	}
}

void Parser::parse_match_contents(ParserContext& preCtx, usize from, usize upto,
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
				if (is_next(TokenType::identifier, i)) {
					auto              fieldName = IdentifierAt(i + 1);
					Maybe<Identifier> valueName;
					bool              isVar = false;
					if (is_next(TokenType::parenthesisOpen, i + 1)) {
						isValueRequested = true;
						auto pCloseRes   = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2);
						if (pCloseRes) {
							if (is_next(TokenType::var, i + 2)) {
								isVar = true;
								i += 3;
							} else {
								i += 2;
							}
							if (is_next(TokenType::identifier, i)) {
								SHOW("Value name is: " << ValueAt(i + 1))
								matchVals.push_back(
								    ast::MixOrChoiceMatchValue::create(fieldName, IdentifierAt(i + 1), isVar));
								if (is_next(TokenType::parenthesisClose, i + 1)) {
									i = pCloseRes.value();
								} else {
									add_error("Unexpected token found after identifier in match case "
									          "value name specification",
									          RangeAt(i + 1));
								}
							} else {
								add_error("Expected value name for the mix subfield", RangeSpan(start, i));
							}
						} else {
							add_error("Expected end for (", RangeAt(i + 2));
						}
					} else {
						matchVals.push_back(ast::MixOrChoiceMatchValue::create(IdentifierAt(i + 1), None, false));
						i++;
					}
					auto j = i;
					while (is_next(TokenType::separator, j)) {
						if (isValueRequested) {
							add_error(
							    "Matched value is requested once and hence a list of variants to match cannot be provided",
							    RangeSpan(start, j));
						}
						if (is_next(TokenType::typeSeparator, j + 1)) {
							if (is_next(TokenType::identifier, j + 2)) {
								if (is_next(TokenType::parenthesisOpen, j + 3)) {
									add_error(
									    "Cannot destructure value for the matched variant, since a list of values to match are provided",
									    RangeAt(j + 4));
								}
								matchVals.push_back(
								    ast::MixOrChoiceMatchValue::create(IdentifierAt(j + 3), None, false));
								if (is_next(TokenType::altArrow, j + 3)) {
									i = j + 3;
									break;
								} else {
									j += 3;
								}
							} else {
								add_error("Expected name for the variant of the mix or choice type to match",
								          RangeAt(j + 2));
							}
						} else {
							add_error("Using normal expressions as cases for matching mix types is not supported yet",
							          RangeAt(j + 1));
						}
					}
					if (is_next(TokenType::altArrow, i)) {
						if (!is_next(TokenType::bracketOpen, i + 1)) {
							add_error(
							    "Expected [ after => to start the case block that contains the sentences to be executed for this case",
							    RangeAt(i + 1));
						}
						i++;
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
						if (bCloseRes) {
							chain.push_back(Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>(
							    matchVals, do_sentences(preCtx, i + 1, bCloseRes.value())));
							i = bCloseRes.value();
						} else {
							add_error("Expected end of [", RangeAt(i + 1));
						}
					} else {
						add_error(
						    "Expected => to before the block that contains sentences to be executed for this case",
						    RangeSpan(i, i + 1));
					}
				} else {
					add_error("Expected name for the variant of the mix or choice type to match", RangeAt(i));
				}
				break;
			}
			case TokenType::Else: {
				auto start = i;
				if (elseCase.has_value()) {
					add_error("Else case for match sentence is already provided. Please check "
					          "logic and make neceassary changes",
					          RangeAt(i));
				}
				if (is_next(TokenType::bracketOpen, i)) {
					auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
					if (bCloseRes.has_value()) {
						auto snts = do_sentences(preCtx, i + 1, bCloseRes.value());
						elseCase  = Pair<Vec<ast::Sentence*>, FileRange>{std::move(snts),
						                                                 FileRange RangeSpan(start, bCloseRes.value())};
						i         = bCloseRes.value();
						if (i + 1 != upto) {
							add_warning(
							    "Expected match sentence to end after the else case. Make sure that the else case is the last branch in a match sentence",
							    RangeSpan(i + 1, bCloseRes.value()));
						}
					} else {
						add_error("Expected end for [", RangeAt(i + 1));
					}
				} else if (is_next(TokenType::altArrow, i)) {
					add_error("Else case for match block doesn't need => before the block", RangeAt(i));
				} else {
					add_error("Expected sentences for the else case in match sentence", RangeAt(i));
				}
				break;
			}
			case TokenType::comment: {
				break;
			}
			default: {
				if (is_primary_within(TokenType::altArrow, i, upto)) {
					auto                  start       = i;
					auto                  matchValEnd = first_primary_position(TokenType::altArrow, i).value();
					Vec<ast::MatchValue*> matchVals;
					if (is_primary_within(TokenType::separator, i, matchValEnd)) {
						auto sepPoss = primary_positions_within(TokenType::separator, i, matchValEnd);
						matchVals.push_back(ast::ExpressionMatchValue::create(
						    do_expression(preCtx, None, i - 1, sepPoss.front()).first));
						for (usize j = 1; j < sepPoss.size(); j++) {
							if (is_next(TokenType::typeSeparator, sepPoss.at(j - 1))) {
								if (is_next(TokenType::identifier, sepPoss.at(j - 1) + 1)) {
									if (is_next(TokenType::separator, sepPoss.at(j - 1) + 2)) {
										matchVals.push_back(ast::MixOrChoiceMatchValue::create(
										    IdentifierAt(sepPoss.at(j - 1) + 2), None, false));
									} else if (is_next(TokenType::parenthesisOpen, sepPoss.at(j - 1) + 2)) {
										add_error(
										    "Multiple values are provided to be matched for this case, so the matched value of the mix type subfield variant cannot be retrieved for use",
										    RangeSpan(sepPoss.at(j - 1) + 1, sepPoss.at(j - 1) + 2));
									} else {
										add_error("Unexpected token found after mix subfield name",
										          RangeSpan(sepPoss.at(j - 1) + 1, sepPoss.at(j - 1) + 2));
									}
								} else {
									add_error("Expected name for the subfield of the mix type to match",
									          RangeAt(sepPoss.at(j - 1) + 1));
								}
							} else {
								matchVals.push_back(ast::ExpressionMatchValue::create(
								    do_expression(preCtx, None, sepPoss.at(j - 1), sepPoss.at(j)).first));
							}
						}
						if (is_next(TokenType::typeSeparator, sepPoss.back())) {
							if (is_next(TokenType::identifier, sepPoss.back() + 1)) {
								if (is_next(TokenType::altArrow, sepPoss.back() + 2)) {
									matchVals.push_back(ast::MixOrChoiceMatchValue::create(
									    IdentifierAt(sepPoss.back() + 2), None, false));
								} else {
									add_error("Unexpected token found after the match case",
									          RangeSpan(sepPoss.back(), sepPoss.back() + 1));
								}
							} else {
								add_error("Expected name for the variant of the mix or choice type to match",
								          RangeAt(sepPoss.back() + 1));
							}
						} else {
							matchVals.push_back(ast::ExpressionMatchValue::create(
							    do_expression(preCtx, None, sepPoss.back(), matchValEnd).first));
						}
					} else {
						matchVals.push_back(
						    ast::ExpressionMatchValue::create(do_expression(preCtx, None, i - 1, matchValEnd).first));
					}
					i = matchValEnd;
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
						if (bCloseRes.has_value()) {
							chain.push_back(Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>(
							    matchVals, do_sentences(preCtx, i + 1, bCloseRes.value())));
							i = bCloseRes.value();
						} else {
							add_error("Expected end for [", RangeAt(i + 1));
						}
					} else {
						add_error(
						    "Expected [ to start the case block that contains sentences to be executed for this case block",
						    RangeSpan(start, i));
					}
					break;
				} else {
					add_error(
					    "You forgot to add => at the end of the expression/list of expressions to match to, which is expected if expressions are used or if there are multiple values to match to",
					    token.fileRange);
				}
			}
		}
	}
}

ast::PlainInitialiser* Parser::do_plain_initialiser(ParserContext& preCtx, Maybe<ast::Type*> type, usize from,
                                                    usize upto) {
	using lexer::TokenType;
	if (is_primary_within(TokenType::associatedAssignment, from, upto)) {
		Vec<Pair<String, FileRange>> fields;
		Vec<ast::Expression*>        fieldValues;
		for (usize j = from + 1; j < upto; j++) {
			if (is_next(TokenType::identifier, j - 1)) {
				fields.push_back(Pair<String, FileRange>(ValueAt(j), RangeAt(j)));
				if (is_next(TokenType::associatedAssignment, j)) {
					if (is_primary_within(TokenType::separator, j + 1, upto)) {
						auto sepRes = first_primary_position(TokenType::separator, j + 1);
						if (sepRes) {
							auto sep = sepRes.value();
							if (sep == j + 2) {
								add_error("No expression for the member found", RangeSpan(j + 1, j + 2));
							}
							fieldValues.push_back(do_expression(preCtx, None, j + 1, sep).first);
							j = sep;
							continue;
						} else {
							add_error("Expected ,", RangeSpan(j + 1, upto));
						}
					} else {
						if (upto == j + 2) {
							add_error("No expression for the member found", RangeSpan(j + 1, j + 2));
						}
						fieldValues.push_back(do_expression(preCtx, None, j + 1, upto).first);
						j = upto;
					}
				} else {
					add_error("Expected := after the name of the member", RangeAt(j));
				}
			} else {
				add_error("Expected an identifier for the name of the member "
				          "of the core type",
				          RangeAt(j));
			}
		}
		return ast::PlainInitialiser::create(type, fields, fieldValues,
		                                     type.has_value() ? FileRange{type.value()->fileRange, RangeAt(upto)}
		                                                      : FileRange RangeSpan(from, upto));
	} else {
		SHOW("PlainInit from " << from << " upto " << upto)
		auto exps = do_separated_expressions(preCtx, from, upto);
		return ast::PlainInitialiser::create(type, {}, exps,
		                                     type.has_value() ? FileRange{type.value()->fileRange, RangeAt(upto)}
		                                                      : FileRange RangeSpan(from, upto));
	}
}

Pair<ast::Expression*, usize> Parser::do_expression(ParserContext&            preCtx, // NOLINT(misc-no-recursion)
                                                    const Maybe<CacheSymbol>& _symbol, usize from, Maybe<usize> upto,
                                                    Maybe<ast::Expression*> _cachedExps, bool returnAtFirstExp) {
	using ast::Expression;
	using lexer::Token;
	using lexer::TokenType;

	SHOW("\n	CachedSymbol : " << (_symbol.has_value() ? "true" : "false"))
	if (_symbol.has_value()) {
		SHOW("			" << Identifier::fullName(_symbol.value().name).value)
	}
	SHOW("	CachedExpression : " << (_cachedExps.has_value() ? "true" : "false"))
	if (_cachedExps.has_value()) {
		SHOW("			" << ((_cachedExps.value())->to_json()["nodeType"].asString()))
	}
	SHOW("	FROM : " << from << " range: " << RangeAt(from))
	SHOW("	UPTO : " << (upto.has_value() ? "true" : "false")
	                 << (upto.has_value() ? "\n		" + std::to_string(upto.value()) +
	                                            " range: " + RangeAt(upto.value()).start_to_string()
	                                      : ""))

	usize i = from + 1; // NOLINT(readability-identifier-length)

	Maybe<CacheSymbol> _cachedSymbol_(_symbol);
	auto               hasCachedSymbol     = [&]() { return _cachedSymbol_.has_value(); };
	auto               consumeCachedSymbol = [&]() {
        if (_cachedSymbol_.has_value()) {
            auto res       = _cachedSymbol_.value();
            _cachedSymbol_ = None;
            return res;
        } else {
            add_error("Internal error : No cached symbol found, but it is requested here", RangeAt(i));
        }
	};
	auto setSymbol = [&](const CacheSymbol& other) {
		if (_cachedSymbol_.has_value()) {
			add_error(
			    "Internal error : A cached symbol is already present, but an attempt to set another symbol was made",
			    RangeAt(i));
		} else {
			_cachedSymbol_ = other;
		}
	};

	Maybe<ast::Type*> _cachedType_;
	auto              hasCachedType = [&]() { return _cachedType_.has_value(); };
	auto              setCachedType = [&](ast::Type* _type) {
        if (_cachedType_.has_value()) {
            add_error(
                "Internal error : A cached type is already present, but an attempt to set another symbol was made",
                RangeAt(i));
        } else {
            _cachedType_ = _type;
        }
	};
	auto consumeCachedType = [&]() {
		if (_cachedType_.has_value()) {
			auto* res    = _cachedType_.value();
			_cachedType_ = None;
			return res;
		} else {
			add_error("Internal error: No cached type found, but it is requested here", RangeAt(i));
		}
	};

	Maybe<Expression*> _cachedExpressions_(_cachedExps);
	auto               hasCachedExpr = [&]() { return _cachedExpressions_.has_value() || _cachedType_.has_value(); };
	auto               consumeCachedExpr = [&]() -> ast::Expression* {
        if (_cachedExpressions_.has_value()) {
            auto* res           = _cachedExpressions_.value();
            _cachedExpressions_ = None;
            return res;
        } else if (_cachedType_.has_value()) {
            auto* res = consumeCachedType();
            return ast::TypeWrap::create(res, false, res->fileRange);
        } else {
            add_error("Internal error : No cached expression found, but it is requested here", RangeAt(i));
        }
	};
#define setCachedExpr(other, returnIndexValue)                                                                             \
	if (_cachedExpressions_.has_value()) {                                                                                 \
		add_error(                                                                                                         \
		    "Internal error : A cached expression is already present, but there was an attempt to set another expression", \
		    RangeAt(i));                                                                                                   \
	} else {                                                                                                               \
		if ((returnIndexValue + 1 <= tokens->size()) &&                                                                    \
		    (tokens->at(returnIndexValue + 1).type == TokenType::binaryOperator) && returnAtFirstExp) {                    \
			return Pair<ast::Expression*, usize>{other, returnIndexValue};                                                 \
		}                                                                                                                  \
		_cachedExpressions_ = other;                                                                                       \
	}

	for (; upto.has_value() ? (i < upto.value()) : (i < tokens->size()); i++) {
		Token& token = tokens->at(i);
		switch (token.type) {
			case TokenType::comment: {
				break;
			}
			case TokenType::Type:
			case TokenType::TRUE:
			case TokenType::FALSE:
			case TokenType::floatLiteral:
			case TokenType::integerLiteral: {
				auto preRes = do_prerun_expression(preCtx, i - 1, None, true);
				setCachedExpr(preRes.first, preRes.second);
				i = preRes.second;
				break;
			}
			case TokenType::StringLiteral: {
				setCachedExpr(ast::StringLiteral::create(token.value, token.fileRange), i);
				break;
			}
			case TokenType::none: {
				ast::Type*       noneType = nullptr;
				auto             range    = RangeAt(i);
				auto             start    = i;
				Maybe<FileRange> isPacked = None;
				if (is_next(TokenType::genericTypeStart, i)) {
					if (is_next(TokenType::packed, i + 1)) {
						isPacked = RangeAt(i + 2);
						i++;
					}
					auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (endRes.has_value()) {
						auto typeRes = do_type(preCtx, i + 1, endRes.value());
						if (typeRes.second + 1 != endRes.value()) {
							add_error("Unexpected end for the type of the none expression",
							          RangeSpan(start, typeRes.second));
						} else {
							noneType = typeRes.first;
							range    = RangeSpan(i, endRes.value());
							i        = endRes.value();
						}
					} else {
						add_error("Expected end for generic type specification", RangeAt(i + 1));
					}
				}
				setCachedExpr(ast::NoneExpression::create(isPacked, noneType, range), i);
				break;
			}
			case TokenType::null: {
				if (is_next(TokenType::genericTypeStart, i)) {
					auto gClose = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (gClose.has_value()) {
						auto typRes = do_type(preCtx, i + 1, gClose);
						if (typRes.second + 1 != gClose.value()) {
							add_error("Provided type for the null mark did not span till the ]",
							          RangeSpan(typRes.second + 1, gClose.value()));
						}
						setCachedExpr(ast::NullMark::create(typRes.first, token.fileRange), gClose.value());
						i = gClose.value();
					} else {
						add_error("No ] to end the type associated with the null-mark expression", RangeSpan(i, i + 1));
					}
				} else {
					setCachedExpr(ast::NullMark::create(None, token.fileRange), i);
				}
				break;
			}
			case TokenType::super:
			case TokenType::identifier: {
				auto symbolRes = do_symbol(preCtx, i);
				setSymbol(symbolRes.first);
				i = symbolRes.second;
				break;
			}
			case TokenType::Default: {
				if (is_next(TokenType::genericTypeStart, i)) {
					auto gClose = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (gClose.has_value()) {
						auto typRes = do_type(preCtx, i + 1, gClose);
						setCachedExpr(ast::Default::create(typRes.first, RangeSpan(i, gClose.value())), gClose.value());
						i = gClose.value();
					} else {
						add_error("No ] to end the type associated with the default expression, found",
						          RangeSpan(i, i + 1));
					}
				} else {
					setCachedExpr(ast::Default::create(None, RangeAt(i)), i);
				}
				break;
			}
			case TokenType::selfWord: {
				auto typeRes = do_type(preCtx, i - 1, None);
				setCachedType(typeRes.first);
				i = typeRes.second;
				break;
			}
			case TokenType::meta: {
				auto start = i;
				if (is_next(TokenType::colon, i) && is_next(TokenType::identifier, i + 1) &&
				    (ValueAt(i + 2) == "intrinsic")) {
					i = i + 2;
					if (is_next(TokenType::genericTypeStart, i)) {
						auto gEnd = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
						if (!gEnd.has_value()) {
							add_error("Expected ] to end the parameter list", RangeAt(i + 1));
						}
						auto params = do_separated_prerun_expressions(preCtx, i + 1, gEnd.value());
						setCachedExpr(ast::GetIntrinsic::create(params, RangeSpan(start, gEnd.value())), gEnd.value());
						i = gEnd.value();
					} else {
						add_error("Expected :[ here to start the parameters required to get intrinsics",
						          RangeSpan(start, i));
					}
				} else {
					add_error("Invalid token found here", RangeAt(i));
				}
				break;
			}
			case TokenType::as: {
				if (hasCachedSymbol()) {
					auto symbol = consumeCachedSymbol();
					setCachedExpr(ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange),
					              symbol.tokenIndex);
				} else if (not hasCachedExpr()) {
					add_error("Could not find an expression for casting before this", RangeAt(i));
				}
				auto typRes = do_type(preCtx, i, None);
				auto expRes = consumeCachedExpr();
				setCachedExpr(ast::Cast::create(expRes, typRes.first, {expRes->fileRange, RangeAt(typRes.second)}),
				              typRes.second);
				i = typRes.second;
				break;
			}
			case TokenType::from:
			case TokenType::colon: {
				const auto isIsolatedFrom = (tokens->at(i).type == TokenType::from);
				if (isIsolatedFrom || is_next(TokenType::from, i)) {
					auto start = i;
					if (not isIsolatedFrom) {
						i++;
						if (hasCachedSymbol()) {
							auto symbol = consumeCachedSymbol();
							setCachedType(ast::NamedType::create(symbol.relative, symbol.name, symbol.fileRange));
						}
					} else if (hasCachedSymbol()) {
						add_error("Invalid syntax for constructor calls. The valid syntax is " +
						              color_error("MyType:from(arg1, arg2)") + " or " +
						              color_error("from(arg1, arg2)") +
						              " if you want the type to be inferred from scope",
						          {consumeCachedSymbol().fileRange, RangeAt(start)});
					}
					if (is_next(TokenType::parenthesisOpen, i)) {
						auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
						if (pCloseRes.has_value()) {
							auto pClose = pCloseRes.value();
							auto exps   = do_separated_expressions(preCtx, i + 1, pClose);
							auto candTy = hasCachedType() ? Maybe<ast::Type*>(consumeCachedType()) : None;
							setCachedExpr(ast::ConstructorCall::create(
							                  candTy, exps,
							                  candTy.has_value() ? FileRange{candTy.value()->fileRange, RangeAt(pClose)}
							                                     : FileRange{RangeAt(start), RangeAt(pClose)}),
							              pClose);
							i = pClose;
						} else {
							add_error("Expected end for (", RangeAt(i + 1));
						}
					} else if (is_next(TokenType::curlybraceOpen, i)) {
						if (hasCachedSymbol()) {
							auto symbol = consumeCachedSymbol();
							setCachedType(ast::NamedType::create(symbol.relative, symbol.name, symbol.fileRange));
						}
						auto cCloseRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i + 1);
						if (cCloseRes.has_value()) {
							setCachedExpr(do_plain_initialiser(
							                  preCtx, hasCachedType() ? Maybe<ast::Type*>(consumeCachedType()) : None,
							                  i + 1, cCloseRes.value()),
							              cCloseRes.value());
							i = cCloseRes.value();
						} else {
							add_error("Expected end for {", token.fileRange);
						}
					}
				} else if (not isIsolatedFrom) {
					if (is_next(TokenType::identifier, i)) {
						// FIXME - Support static members and functions for generic types
						add_error("This syntax is not yet supported", RangeSpan(i, i + 1));
					} else {
						add_error("Found : here without an identifier or " + color_error("from") +
						              " following it. This is invalid syntax",
						          RangeAt(i));
					}
				}
				break;
			}
			case TokenType::genericTypeStart: {
				SHOW("Generic type start: " << i)
				if (hasCachedSymbol()) {
					auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i);
					if (endRes.has_value()) {
						auto end = endRes.value();
						SHOW("Generic type end: " << end)
						auto types = do_generic_fill(preCtx, i, end);
						if (is_next(TokenType::colon, end) && is_next(TokenType::from, end + 1)) {
							auto symbol = consumeCachedSymbol();
							setCachedType(
							    ast::GenericNamedType::create(symbol.relative, symbol.name, types, symbol.fileRange));
							i = end;
						} else {
							auto symbol = consumeCachedSymbol();
							setCachedExpr(ast::GenericEntity::create(symbol.relative, symbol.name, types,
							                                         {symbol.fileRange, RangeAt(end)}),
							              end);
							i = end;
						}
					} else {
						add_error("Expected end for generic type specification", RangeAt(i));
					}
				} else {
					add_error("Expected identifier or symbol before generic type specification", RangeAt(i));
				}
				break;
			}
			case TokenType::selfInstance: {
				if (is_next(TokenType::copy, i)) {
					setCachedExpr(ast::Copy::create(ast::SelfInstance::create(RangeAt(i)), true, RangeSpan(i, i + 1)),
					              i + 1);
					i++;
				} else if (is_next(TokenType::move, i)) {
					setCachedExpr(ast::Move::create(ast::SelfInstance::create(RangeAt(i)), true, RangeSpan(i, i + 1)),
					              i + 1);
					i++;
				} else if (((is_next(TokenType::var, i) || is_next(TokenType::constant, i)) &&
				            is_next(TokenType::colon, i + 1) && is_next(TokenType::identifier, i + 2)) ||
				           is_next(TokenType::identifier, i)) {
					auto        start = i;
					Maybe<bool> callNature;
					if (is_next(TokenType::var, i)) {
						callNature = true;
						i += 2;
					} else if (is_next(TokenType::constant, i)) {
						callNature = false;
						i += 2;
					}
					// FIXME - Support generic member function calls
					if (is_next(TokenType::parenthesisOpen, i + 1)) {
						auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2);
						if (pCloseRes.has_value()) {
							auto pClose = pCloseRes.value();
							auto args   = do_separated_expressions(preCtx, i + 2, pClose);
							setCachedExpr(ast::MethodCall::create(ast::SelfInstance::create(RangeAt(start)), true,
							                                      IdentifierAt(i + 1), args, callNature,
							                                      RangeSpan(start, pClose)),
							              pClose);
							i = pClose;
							break;
						} else {
							add_error("Expected end for (", RangeAt(i + 2));
						}
					} else {
						setCachedExpr(ast::MemberAccess::create(ast::SelfInstance::create(RangeAt(start)), true,
						                                        callNature, IdentifierAt(i + 1),
						                                        RangeSpan(start, i + 1)),
						              i + 1);
						i++;
						break;
					}
				} else {
					SHOW("Creating self in AST")
					setCachedExpr(ast::SelfInstance::create(RangeAt(i)), i);
				}
				break;
			}
			case TokenType::heap: {
				if (is_next(TokenType::colon, i)) {
					if (is_next(TokenType::identifier, i + 1)) {
						if (ValueAt(i + 2) == "get") {
							if (is_next(TokenType::genericTypeStart, i + 2)) {
								auto tEndRes =
								    get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 3);
								if (tEndRes.has_value()) {
									auto  tEnd    = tEndRes.value();
									auto  typeRes = do_type(preCtx, i + 3, tEnd);
									auto* type    = typeRes.first;
									if (typeRes.second != tEnd - 1) {
										add_error("Invalid type for heap'get", RangeSpan(i, typeRes.second));
									}
									if (is_next(TokenType::parenthesisOpen, tEnd)) {
										auto pCloseRes = get_pair_end(TokenType::parenthesisOpen,
										                              TokenType::parenthesisClose, tEnd + 1);
										if (pCloseRes.has_value()) {
											auto  pClose = pCloseRes.value();
											auto* exp    = do_expression(preCtx, None, tEnd + 1, pClose).first;
											setCachedExpr(ast::HeapGet::create(type, exp, RangeAt(i)), i);
											i = pClose;
											break;
										} else {
											add_error("Expected end for (", RangeAt(tEnd + 1));
										}
									} else {
										setCachedExpr(ast::HeapGet::create(type, nullptr, RangeSpan(i, tEnd)), tEnd);
										i = tEnd;
										break;
									}
								} else {
									add_error("Expected end for generic type specification", RangeAt(i + 3));
								}
							} else {
								add_error("Expected generic type specification for the type to allocate the memory for",
								          RangeSpan(i, i + 2));
							}
						} else if (ValueAt(i + 2) == "put") {
							if (is_next(TokenType::parenthesisOpen, i + 2)) {
								auto pCloseRes =
								    get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 3);
								if (pCloseRes.has_value()) {
									auto* exp = do_expression(preCtx, None, i + 3, pCloseRes.value()).first;
									setCachedExpr(ast::HeapPut::create(exp, RangeSpan(i, pCloseRes.value())),
									              pCloseRes.value());
									i = pCloseRes.value();
									break;
								} else {
									add_error("Expected end for (", RangeAt(i + 3));
								}
							} else {
								add_error("Invalid expression. Expected a pointer expression to use "
								          "for heap'put",
								          RangeSpan(i, i + 2));
							}
						} else if (ValueAt(i + 2) == "grow") {
							// FIXME - Maybe provided type is not necessary
							if (is_next(TokenType::genericTypeStart, i + 2)) {
								auto tEndRes =
								    get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 3);
								if (tEndRes.has_value()) {
									auto  tEnd = tEndRes.value();
									auto* type = do_type(preCtx, i + 3, tEnd).first;
									if (is_next(TokenType::parenthesisOpen, tEnd)) {
										auto pCloseRes = get_pair_end(TokenType::parenthesisOpen,
										                              TokenType::parenthesisClose, tEnd + 1);
										if (pCloseRes.has_value()) {
											auto pClose = pCloseRes.value();
											if (is_primary_within(TokenType::separator, tEnd + 1, pClose)) {
												auto split =
												    first_primary_position(TokenType::separator, tEnd + 1).value();
												auto* exp   = do_expression(preCtx, None, tEnd + 1, split).first;
												auto* count = do_expression(preCtx, None, split, pClose).first;
												setCachedExpr(
												    ast::HeapGrow::create(type, exp, count, RangeSpan(i, pClose)),
												    pClose);
												i = pClose;
											} else {
												add_error("Expected 2 argument values for heap'grow",
												          RangeSpan(tEnd + 1, pClose));
											}
											break;
										} else {
											add_error("Expected end for (", RangeAt(tEnd + 1));
										}
									} else {
										add_error("Expected arguments for heap'grow", RangeSpan(i, tEnd));
									}
								} else {
									add_error("Expected end for the generic type specification", RangeAt(i + 3));
								}
							}
						} else {
							add_error("Invalid identifier found after heap'", RangeAt(i + 2));
						}
					} else {
						add_error("Expected an identifier after heap' to represent the heap operation to perform",
						          RangeSpan(i, i + 1));
					}
				} else {
					add_error("Invalid expression", RangeAt(i));
				}
				break;
			}
			case TokenType::bracketOpen: {
				SHOW("Found [")
				auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i);
				if (bCloseRes.has_value()) {
					//   if (isNotPartOfExpression(i, bCloseRes.value())) {
					//     if (hasCachedExpr()) {
					//       return {consumeCachedExpr(), i - 1};
					//     } else {
					//       if (hasCachedSymbol()) {
					//         auto symbol = consumeCachedSymbol();
					//         return {ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange), i - 1};
					//       } else {
					//         Error("[ ...... ] found that is not part of an expression and no expression found
					//         before that",
					//               RangeSpan(i, bCloseRes.value()));
					//       }
					//     }
					//   }
					if (hasCachedSymbol()) {
						auto symbol = consumeCachedSymbol();
						setCachedExpr(ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange),
						              symbol.tokenIndex);
					}
					if (hasCachedExpr()) {
						auto* exp = do_expression(preCtx, None, i, bCloseRes.value()).first;
						auto* ent = consumeCachedExpr();
						setCachedExpr(ast::IndexAccess::create(ent, exp, {ent->fileRange, exp->fileRange}),
						              bCloseRes.value());
						i = bCloseRes.value();
					} else {
						auto bClose = bCloseRes.value();
						if (bClose == i + 1) {
							// Empty array literal
							setCachedExpr(ast::ArrayLiteral::create({}, RangeSpan(i, bClose)), bClose);
							i = bClose;
						} else {
							auto vals = do_separated_expressions(preCtx, i, bClose);
							setCachedExpr(ast::ArrayLiteral::create(vals, RangeSpan(i, bClose)), bClose);
							i = bClose;
						}
					}
				} else {
					add_error("Expected end for [", token.fileRange);
				}
				break;
			}
			case TokenType::parenthesisOpen: {
				SHOW("Found paranthesis")
				if (hasCachedExpr() || hasCachedSymbol()) {
					// This parenthesis is supposed to indicate a function call
					auto p_close = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i);
					if (p_close.has_value()) {
						SHOW("Found end of paranthesis")
						if (upto.has_value() && (p_close.value() >= upto)) {
							add_error("Invalid position of )", token.fileRange);
						} else {
							SHOW("About to parse arguments")
							Vec<ast::Expression*> args;
							if (is_primary_within(TokenType::separator, i, p_close.value())) {
								args = do_separated_expressions(preCtx, i, p_close.value());
							} else if (i < (p_close.value() - 1)) {
								args.push_back(do_expression(preCtx, None, i, p_close.value()).first);
							}
							if (!hasCachedExpr()) {
								SHOW("Expressions cache is empty")
								if (hasCachedSymbol()) {
									auto  symbol = consumeCachedSymbol();
									auto* ent    = ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange);
									setCachedExpr(ast::FunctionCall::create(
									                  ent, args, symbol.extend_fileRange(RangeAt(p_close.value()))),
									              p_close.value());
								} else {
									add_error("No expression found to be passed to the function call. "
									          "And no function name found for "
									          "the static function call",
									          {token.fileRange, RangeAt(p_close.value())});
								}
							} else {
								auto* expression = consumeCachedExpr();
								setCachedExpr(
								    ast::FunctionCall::create(
								        expression, args, FileRange(expression->fileRange, RangeAt(p_close.value()))),
								    p_close.value());
							}
							i = p_close.value();
						}
					} else {
						add_error("Expected ) to close the scope started by this opening "
						          "paranthesis",
						          token.fileRange);
					}
				} else {
					auto p_close_res = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i);
					if (!p_close_res.has_value()) {
						add_error("Expected )", token.fileRange);
					}
					if (upto.has_value() && (p_close_res.value() >= upto)) {
						add_error("Invalid position of )", token.fileRange);
					}
					auto p_close = p_close_res.value();
					if (is_primary_within(TokenType::semiColon, i, p_close)) {
						Vec<ast::Expression*> values;
						auto                  separations = primary_positions_within(TokenType::semiColon, i, p_close);
						values.push_back(do_expression(preCtx, None, i, separations.front()).first);
						bool shouldContinue = true;
						if (separations.size() == 1) {
							SHOW("Only 1 separation")
							shouldContinue = (separations.at(0) != (p_close - 1));
							SHOW("Found condition")
						}
						if (shouldContinue) {
							for (usize j = 0; j < (separations.size() - 1); j += 1) {
								values.push_back(
								    do_expression(preCtx, None, separations.at(j), separations.at(j + 1)).first);
							}
							values.push_back(do_expression(preCtx, None, separations.back(), p_close).first);
						}
						setCachedExpr(ast::TupleValue::create(values, FileRange(token.fileRange, RangeAt(p_close))),
						              p_close);
						i = p_close;
					} else {
						SHOW("Parsing enclosed expression")
						auto expRes = do_expression(preCtx, None, i, p_close);
						SHOW("Done parsing enclosed expression")
						setCachedExpr(expRes.first, expRes.second);
						if (expRes.second + 1 != p_close) {
							add_error("Expression did not span till )", RangeSpan(expRes.second + 1, p_close));
						}
						i = p_close;
					}
				}
				break;
			}
			case TokenType::ok: {
				const auto                                     start = i;
				Maybe<Pair<FileRange, ast::PrerunExpression*>> isPacked;
				Maybe<Pair<ast::Type*, ast::Type*>>            providedType;
				if (is_next(TokenType::genericTypeStart, i)) {
					auto gClose = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (!gClose.has_value()) {
						add_error("Expected ] to end the type specification for the " + color_error("ok") +
						              " expression",
						          RangeAt(i + 1));
					}
					if (is_next(TokenType::packed, i + 1)) {
						if (is_next(TokenType::associatedAssignment, i + 2)) {
							auto packExpRes = do_prerun_expression(preCtx, i + 3, None);
							isPacked = Pair<FileRange, ast::PrerunExpression*>{RangeSpan(i + 2, packExpRes.second + 1),
							                                                   packExpRes.first};
							i        = packExpRes.second;
						} else {
							isPacked = Pair<FileRange, ast::PrerunExpression*>{RangeAt(i + 2), nullptr};
							i        = i + 2;
						}
						if (is_next(TokenType::separator, i)) {
							i++;
						} else if (!is_next(TokenType::genericTypeEnd, i)) {
							add_error(
							    "Expected either a , after this, or expected ] to end the type specification here",
							    RangeSpan(start, i));
						}
					}
					if (!isPacked.has_value() || token.type == TokenType::separator) {
						auto validTyRes = do_type(preCtx, i, None);
						if (!is_next(TokenType::separator, validTyRes.second)) {
							add_error("Expected , after this to separate the valid type from the error type",
							          validTyRes.first->fileRange);
						}
						i             = validTyRes.second + 1;
						auto errTyRes = do_type(preCtx, i, None);
						if (errTyRes.second + 1 != gClose.value()) {
							add_error("Expected ] after this to end the type specification, but found something else",
							          RangeSpan(start, errTyRes.second));
						}
						providedType = Pair<ast::Type*, ast::Type*>{validTyRes.first, errTyRes.first};
					}
					i = gClose.value();
				}
				if (is_next(TokenType::parenthesisOpen, i)) {
					auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
					if (pCloseRes) {
						auto             pClose = pCloseRes.value();
						ast::Expression* exp    = nullptr;
						if (i + 2 != pClose) {
							exp = do_expression(preCtx, None, i + 1, pClose).first;
						}
						setCachedExpr(ast::OkExpression::create(exp, isPacked, providedType, RangeSpan(i, pClose)),
						              pClose);
						i = pClose;
					} else {
						add_error("Expected end for (", RangeAt(i + 1));
					}
				} else {
					add_error("Expected ( to start the " + color_error("ok") + " expression", RangeSpan(start, i));
				}
				break;
			}
			case TokenType::error: {
				/// NOTE - The logic is identical to that of an `ok` expression
				const auto                                     start = i;
				Maybe<Pair<FileRange, ast::PrerunExpression*>> isPacked;
				Maybe<Pair<ast::Type*, ast::Type*>>            providedType;
				if (is_next(TokenType::genericTypeStart, i)) {
					auto gClose = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (!gClose.has_value()) {
						add_error("Expected ] to end the type specification for the " + color_error("error") +
						              " expression",
						          RangeAt(i + 1));
					}
					if (is_next(TokenType::packed, i + 1)) {
						if (is_next(TokenType::associatedAssignment, i + 2)) {
							auto packExpRes = do_prerun_expression(preCtx, i + 3, None);
							isPacked = Pair<FileRange, ast::PrerunExpression*>{RangeSpan(i + 2, packExpRes.second + 1),
							                                                   packExpRes.first};
							i        = packExpRes.second;
						} else {
							isPacked = Pair<FileRange, ast::PrerunExpression*>{RangeAt(i + 2), nullptr};
							i        = i + 2;
						}
						if (is_next(TokenType::separator, i)) {
							i++;
						} else if (!is_next(TokenType::genericTypeEnd, i)) {
							add_error(
							    "Expected either a , after this, or expected ] to end the type specification here",
							    RangeSpan(start, i));
						}
					}
					if (!isPacked.has_value() || token.type == TokenType::separator) {
						auto validTyRes = do_type(preCtx, i, None);
						if (!is_next(TokenType::separator, validTyRes.second)) {
							add_error("Expected , after this to separate the valid type from the error type",
							          validTyRes.first->fileRange);
						}
						i             = validTyRes.second + 1;
						auto errTyRes = do_type(preCtx, i, None);
						if (errTyRes.second + 1 != gClose.value()) {
							add_error("Expected ] after this to end the type specification, but found something else",
							          RangeSpan(start, errTyRes.second));
						}
						providedType = Pair<ast::Type*, ast::Type*>{validTyRes.first, errTyRes.first};
					}
					i = gClose.value();
				}
				if (is_next(TokenType::parenthesisOpen, i)) {
					auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
					if (pCloseRes) {
						auto             pClose = pCloseRes.value();
						ast::Expression* exp    = nullptr;
						if (i + 2 != pClose) {
							exp = do_expression(preCtx, None, i + 1, pClose).first;
						}
						setCachedExpr(ast::ErrorExpression::create(exp, isPacked, providedType, RangeSpan(i, pClose)),
						              pClose);
						i = pClose;
					} else {
						add_error("Expected end for (", RangeAt(i + 1));
					}
				} else {
					add_error("Expected ( to start the " + color_error("error") + " expression", RangeSpan(start, i));
				}
				break;
			}
			case TokenType::referenceType: {
				if (hasCachedExpr() || hasCachedSymbol()) {
					if (hasCachedSymbol()) {
						if (hasCachedExpr()) {
							add_error("Compiler internal error : Both a cached expression and a cached symbol found",
							          RangeAt(i));
						} else {
							auto symbol = consumeCachedSymbol();
							setCachedExpr(ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange),
							              symbol.tokenIndex);
						}
					}
					auto* exp = consumeCachedExpr();
					setCachedExpr(ast::Dereference::create(exp, {exp->fileRange, RangeAt(i)}), i);
				} else {
					add_error("No expression found before pointer dereference operator", RangeAt(i));
				}
				break;
			}
			case TokenType::Await: {
				auto expRes = do_expression(preCtx, None, i, None);
				setCachedExpr(ast::Await::create(expRes.first, RangeSpan(i, expRes.second)), expRes.second);
				i = expRes.second;
				break;
			}
			case TokenType::binaryOperator: {
				SHOW("Binary operator found: " << token.value)
				if (!hasCachedExpr() && !hasCachedSymbol()) {
					if (token.value == "-") {
						auto expRes = do_expression(preCtx, None, i, upto, None, true);
						setCachedExpr(ast::Negative::create(expRes.first, RangeSpan(i, expRes.second)), expRes.second);
						i = expRes.second;
					} else {
						add_error("No expression found on the left side of the binary operator " + token.value,
						          token.fileRange);
					}
				} else {
					ast::Expression* lhs = nullptr;
					if (hasCachedSymbol()) {
						auto symbol = consumeCachedSymbol();
						lhs         = ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange);
					} else {
						lhs = consumeCachedExpr();
					}
					auto rhsRes = do_expression(preCtx, None, i, upto, None, true);
					if (is_next(TokenType::binaryOperator, rhsRes.second) && !returnAtFirstExp) {
						Deque<ast::Op>          operators{ast::operator_from_string(token.value)};
						Deque<ast::Expression*> expressions{lhs, rhsRes.first};
						i = rhsRes.second;
						while (is_next(TokenType::binaryOperator, i)) {
							auto newOp  = ast::operator_from_string(ValueAt(i + 1));
							auto newRhs = do_expression(preCtx, None, i + 1, upto, None, true);
							operators.push_back(newOp);
							expressions.push_back(newRhs.first);
							i = newRhs.second;
						}
						auto findLowestPrecedence = [&]() {
							usize lowestPrec = 1000;
							usize result     = 0;
							for (usize j = 0; j < operators.size(); j++) {
								if (ast::get_precedence_of(operators[j]) < lowestPrec) {
									lowestPrec = ast::get_precedence_of(operators[j]);
									result     = j;
								}
							}
							return result;
						};
						while (operators.size() > 0) {
							auto index  = findLowestPrecedence();
							auto newExp = ast::BinaryExpression::create(
							    expressions[index], ast::operator_to_string(operators[index]), expressions[index + 1],
							    {expressions[index]->fileRange, expressions[index + 1]->fileRange});
							operators.erase(operators.begin() + index);
							expressions.erase(expressions.begin() + index + 1);
							expressions.at(index) = newExp;
						}
						setCachedExpr(expressions.front(), i);
					} else {
						i = rhsRes.second;
						setCachedExpr(ast::BinaryExpression::create(lhs, token.value, rhsRes.first,
						                                            {lhs->fileRange, rhsRes.first->fileRange}),
						              i);
					}
				}
				break;
			}
				TYPE_TRIGGER_TOKENS {
					auto typeRes = do_type(preCtx, i - 1, None);
					setCachedType(typeRes.first);
					i = typeRes.second;
					break;
				}
			case TokenType::typeSeparator: {
				Maybe<ast::Type*> typeVal;
				if (is_next(TokenType::identifier, i)) {
					if (hasCachedSymbol()) {
						auto symbol = consumeCachedSymbol();
						typeVal     = ast::NamedType::create(symbol.relative, symbol.name, symbol.fileRange);
					}
					auto subName = IdentifierAt(i + 1);
					if (is_next(TokenType::parenthesisOpen, i + 1)) {
						auto pCloseRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2);
						if (pCloseRes) {
							auto  pClose = pCloseRes.value();
							auto* exp    = do_expression(preCtx, None, i + 2, pClose).first;
							setCachedExpr(
							    ast::MixOrChoiceInitialiser::create(
							        typeVal, subName, exp,
							        {typeVal.has_value() ? typeVal.value()->fileRange : RangeAt(i), RangeAt(pClose)}),
							    pClose);
							i = pClose;
						} else {
							add_error("Expected end for (", RangeAt(i + 2));
						}
					} else {
						setCachedExpr(
						    ast::MixOrChoiceInitialiser::create(
						        typeVal, subName, None,
						        {typeVal.has_value() ? typeVal.value()->fileRange : RangeAt(i), RangeAt(i + 1)}),
						    i + 1);
						i++;
					}
				}
				break;
			}
			case TokenType::is: {
				if (is_next(TokenType::parenthesisOpen, i)) {
					if (is_next(TokenType::parenthesisClose, i + 1)) {
						setCachedExpr(ast::IsExpression::create(nullptr, RangeSpan(i, i + 2)), i + 2);
						i = i + 2;
					} else {
						auto pEnd = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
						if (pEnd) {
							auto subExpRes = do_expression(preCtx, None, i + 1, pEnd);
							setCachedExpr(ast::IsExpression::create(subExpRes.first, RangeSpan(i, pEnd.value())),
							              pEnd.value());
							i = pEnd.value();
						} else {
							add_error("Expected end for (", RangeAt(i + 1));
						}
					}
				} else {
					add_error(
					    "Expected an expression after is. Did you forget to provide one? The syntax for `is` expression is `is(myexpr)`",
					    RangeAt(i));
				}
				break;
			}
			case TokenType::child: {
				SHOW("Expression parsing : Member access")
				if (hasCachedExpr() || hasCachedSymbol()) {
					if ((!hasCachedExpr()) && hasCachedSymbol()) {
						auto symbol = consumeCachedSymbol();
						setCachedExpr(ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange),
						              symbol.tokenIndex);
					} else if (hasCachedExpr() && hasCachedSymbol()) {
						add_error("Internal error - Cached expressions are not empty and also found symbol",
						          consumeCachedExpr()->fileRange);
					}
					auto* exp = consumeCachedExpr();
					if (is_next(TokenType::Not, i)) {
						setCachedExpr(ast::LogicalNot::create(exp, FileRange{exp->fileRange, RangeAt(i + 1)}), i + 1);
						i++;
						break;
					} else if (is_next(TokenType::copy, i)) {
						setCachedExpr(ast::Copy::create(exp, false, exp->fileRange.spanTo(RangeAt(i + 1))), i + 1);
						i++;
					} else if (is_next(TokenType::move, i)) {
						setCachedExpr(ast::Move::create(exp, false, exp->fileRange.spanTo(RangeAt(i + 1))), i + 1);
						i++;
					} else if (is_next(TokenType::to, i)) {
						if (is_next(TokenType::parenthesisOpen, i + 1)) {
							auto destTyRes = do_type(preCtx, i + 2, None);
							if (not is_next(TokenType::parenthesisClose, destTyRes.second)) {
								add_error("Could not find ) to end the target type of the " + color_error("to") +
								              " conversion",
								          RangeSpan(i, destTyRes.second));
							}
							setCachedExpr(
							    ast::ToConversion::create(exp, destTyRes.first,
							                              FileRange{exp->fileRange, RangeAt(destTyRes.second + 1)}),
							    destTyRes.second + 1);
							i = destTyRes.second + 1;
						} else {
							// TODO - Support type inference in this case ???
							add_error("Expected a type to be provided after this like " + color_error("'to(MyType)"),
							          RangeAt(i + 1));
						}
					} else if ((is_next(TokenType::var, i) && is_next(TokenType::colon, i + 1) &&
					            is_next(TokenType::identifier, i + 2)) ||
					           is_next(TokenType::identifier, i)) {
						auto        start = i;
						Maybe<bool> callNature;
						if (is_next(TokenType::var, i)) {
							callNature = true;
							i += 2;
						} else if (is_next(TokenType::constant, i)) {
							callNature = false;
							i += 2;
						}
						// FIXME - Support generic member function calls
						if (is_next(TokenType::parenthesisOpen, i + 1)) {
							auto pCloseRes =
							    get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2);
							if (pCloseRes.has_value()) {
								auto pClose = pCloseRes.value();
								auto args   = do_separated_expressions(preCtx, i + 2, pClose);
								setCachedExpr(ast::MethodCall::create(exp, false, IdentifierAt(i + 1), args, callNature,
								                                      exp->fileRange.spanTo(RangeSpan(start, pClose))),
								              pClose);
								i = pClose;
								break;
							} else {
								add_error("Expected end for (", RangeAt(i + 2));
							}
						} else {
							setCachedExpr(ast::MemberAccess::create(exp, false, callNature, IdentifierAt(i + 1),
							                                        exp->fileRange.spanTo(RangeSpan(i, i + 1))),
							              i + 1);
							i++;
							break;
						}
					} else if (is_next(TokenType::end, i)) {
						if (is_next(TokenType::parenthesisOpen, i + 1)) {
							auto pCloseRes =
							    get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 2);
							if (pCloseRes) {
								setCachedExpr(
								    ast::MethodCall::create(exp, false, Identifier{"end", RangeAt(i)}, {}, None,
								                            exp->fileRange.spanTo(RangeSpan(i, pCloseRes.value()))),
								    pCloseRes.value());
								i = pCloseRes.value();
								break;
							} else {
								add_error("Expected end for (", RangeAt(i + 2));
							}
						} else {
							add_error("Expected ( to start the destructor call", RangeAt(i));
						}
					} else if (is_next(TokenType::markType, i)) {
						setCachedExpr(ast::AddressOf::create(exp, {exp->fileRange, RangeAt(i + 1)}), i + 1);
						i += 1;
					} else {
						add_error("Expected an identifier for member access", RangeAt(i));
					}
				} else {
					add_error("No expression found for member access", RangeAt(i));
				}
				break;
			}
			case TokenType::assembly: {
				const auto start        = i;
				ast::Type* functionType = nullptr;
				if (is_next(TokenType::genericTypeStart, i)) {
					auto genInd = i + 1;
					auto gClose = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i + 1);
					if (!gClose.has_value()) {
						add_error("Expected ] to end the prerun parameters for the assembly block", RangeAt(i + 1));
					}
					auto fnTypeRes = do_type(preCtx, i + 1, None);
					functionType   = fnTypeRes.first;
					i              = fnTypeRes.second;
					if (is_next(TokenType::separator, i)) {
						auto expRes = do_prerun_expression(preCtx, i + 1, None);
						i           = expRes.second;
						if (is_next(TokenType::separator, i)) {
							auto clobRes = do_prerun_expression(preCtx, i + 1, None);
							i            = clobRes.second;
							Maybe<FileRange>       volatileRange;
							ast::PrerunExpression* volatileExpr = nullptr;
							if (is_next(TokenType::separator, i)) {
								i++;
								if (is_next(TokenType::Volatile, i)) {
									if (is_next(TokenType::parenthesisOpen, i + 1)) {
										auto pInd    = i + 2;
										auto volRes  = do_prerun_expression(preCtx, i + 2, None);
										volatileExpr = volRes.first;
										i            = volRes.second;
										if (is_next(TokenType::parenthesisClose, i)) {
											i++;
										} else {
											add_error("Expected ) to end the condition for volatility",
											          RangeSpan(pInd, i));
										}
									} else {
										volatileRange = RangeAt(i + 1);
										i++;
									}
									if (is_next(TokenType::separator, i)) {
										i++;
									}
								}
							}
							if (is_next(TokenType::genericTypeEnd, i) && ((i + 1) == gClose.value())) {
								i++;
							} else {
								add_error("Expected ] to end the list of prerun parameters for the assembly block",
								          RangeSpan(genInd, i));
							}
							Vec<ast::Expression*> args;
							FileRange             argsRange{""};
							if (is_next(TokenType::parenthesisOpen, i)) {
								auto pClose =
								    get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i + 1);
								if (!pClose.has_value()) {
									add_error("Expected ) to end the list of arguments for the assembly block",
									          RangeAt(i + 1));
								}
								args      = do_separated_expressions(preCtx, i + 1, pClose.value());
								argsRange = RangeSpan(i + 1, pClose.value());
								i         = pClose.value();
							} else {
								add_error("Expected ( to start the list of arguments for the assembly block",
								          RangeSpan(start, i));
							}
							setCachedExpr(ast::AssemblyBlock::create(functionType, expRes.first, args, argsRange,
							                                         clobRes.first, volatileRange, volatileExpr,
							                                         RangeSpan(start, i)),
							              i);
						} else {
							add_error("Expected , after this to precede the list of constraints", RangeSpan(start, i));
						}
					} else {
						add_error("Expected , after this to precede the assembly code", RangeSpan(start, i));
					}
				} else {
					add_error("Expected :[ to start the list of prerun parameters for the assembly block", RangeAt(i));
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
						return {ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange), i - 1};
					} else {
						add_error("Encountered an invalid token in the expression", RangeAt(i));
					}
				} else {
					if (hasCachedExpr()) {
						return {consumeCachedExpr(), i - 1};
					} else if (hasCachedSymbol()) {
						auto symbol = consumeCachedSymbol();
						return {ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange), i - 1};
					} else {
						add_error("No expression found and encountered invalid token", RangeAt(i));
					}
				}
			}
		}
	}
	if (!hasCachedExpr()) {
		if (hasCachedSymbol()) {
			auto symbol = consumeCachedSymbol();
			return {ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange),
			        (upto.has_value() && (i == upto.value())) ? (i - 1) : i};
		} else {
			add_error("No expression found", RangeAt(from));
		}
	} else {
		return {consumeCachedExpr(), (upto.has_value() && (i == upto.value())) ? (i - 1) : i};
	}
} // NOLINT(clang-diagnostic-return-type)

Vec<ast::PrerunExpression*> Parser::do_separated_prerun_expressions(ParserContext& preCtx, usize from, usize to) {
	Vec<ast::PrerunExpression*> result;
	for (usize i = from + 1; i < to; i++) {
		if (is_primary_within(lexer::TokenType::separator, i - 1, to)) {
			auto endResult = first_primary_position(lexer::TokenType::separator, i - 1);
			if (!endResult.has_value() || (endResult.value() >= to)) {
				add_error("Invalid position for separator `,`", RangeAt(i));
			}
			auto end = endResult.value();
			result.push_back(do_prerun_expression(preCtx, i - 1, end).first);
			i = end;
		} else {
			result.push_back(do_prerun_expression(preCtx, i - 1, to).first);
			i = to;
		}
	}
	return result;
}

Vec<ast::Expression*> Parser::do_separated_expressions(ParserContext& preCtx, usize from, usize to) {
	Vec<ast::Expression*> result;
	SHOW("Sep exp: " << from << " - " << to)
	for (usize i = from + 1; i < to; i++) {
		SHOW("Separated exp iter " << i)
		auto expRes = do_expression(preCtx, None, i - 1, None);
		result.push_back(expRes.first);
		i = expRes.second;
		if (is_next(lexer::TokenType::separator, i)) {
			i++;
		} else {
			if (i + 1 == to) {
				continue;
			}
			SHOW("from " << from << " to " << to << " and i " << i)
			add_error("Expected , after this expression as the list of expressions are not complete yet",
			          expRes.first->fileRange);
		}
		// if (is_primary_within(lexer::TokenType::separator, i - 1, to)) {
		//   auto endResult = first_primary_position(lexer::TokenType::separator, i - 1);
		//   if (!endResult.has_value() || (endResult.value() >= to)) {
		//     add_error("Invalid position for separator `,`", RangeAt(i));
		//   }
		//   auto end = endResult.value();
		//   result.push_back(do_expression(preCtx, None, i - 1, end).first);
		//   i = end;
		// } else {
		//   result.push_back(do_expression(preCtx, None, i - 1, to).first);
		//   i = to;
		// }
	}
	return result;
}

Vec<ast::Type*> Parser::do_separated_types(ParserContext& preCtx, usize from, usize upto) {
	Vec<ast::Type*> result;
	for (usize i = from + 1; i < upto; i++) {
		if (is_primary_within(lexer::TokenType::separator, i - 1, upto)) {
			auto endResult = first_primary_position(lexer::TokenType::separator, i - 1);
			if (!endResult.has_value() || (endResult.value() >= upto)) {
				add_error("Invalid position for separator `,`", RangeAt(i));
			}
			auto end = endResult.value();
			result.push_back(do_type(preCtx, i - 1, end).first);
			i = end;
		} else {
			result.push_back(do_type(preCtx, i - 1, upto).first);
			i = upto;
		}
	}
	return result;
}

Pair<CacheSymbol, usize> Parser::do_symbol(ParserContext& preCtx, const usize start) {
	using lexer::TokenType;

	Vec<Identifier> name;
	u32             relative = 0;
	usize           i        = start;
	while ((tokens->at(i).type == TokenType::super) && is_next(TokenType::colon, i)) {
		relative++;
		i += 2;
	}
	if (tokens->at(i).type == TokenType::super) {
		relative++;
		return {CacheSymbol(relative, {}, start, RangeSpan(start, i)), i};
	}
	if (tokens->at(i).type != TokenType::identifier) {
		add_error("This is an invalid symbol name. No identifier could be found", RangeSpan(start, i));
	}
	name.push_back(IdentifierAt(i));
	i++;
	while ((tokens->at(i).type == TokenType::colon) && is_next(TokenType::identifier, i)) {
		name.push_back(IdentifierAt(i + 1));
		i += 2;
	}
	return {CacheSymbol(relative, std::move(name), start, RangeSpan(start, i - 1)), i - 1};
}

Pair<Vec<ast::PrerunSentence*>, usize> Parser::do_prerun_sentences(ParserContext& preCtx, usize from) {
	using lexer::TokenType;

	Vec<ast::PrerunSentence*> sentences;
	for (usize i = from + 1; i < tokens->size(); i++) {
		switch (tokens->at(i).type) {
			case TokenType::bracketClose: {
				return {sentences, i};
			}
			case TokenType::Break: {
				auto              start = i;
				Maybe<Identifier> tag;
				if (is_next(TokenType::identifier, i)) {
					tag = IdentifierAt(i + 1);
					i++;
				}
				if (not is_next(TokenType::stop, i)) {
					add_error("Expected " + color_error(".") + " after this to end the " + color_error("break") +
					              " sentence",
					          RangeSpan(start, i));
				}
				i++;
				sentences.push_back(ast::PrerunBreak::create(tag, RangeSpan(start, i)));
				break;
			}
			case TokenType::Continue: {
				auto              start = i;
				Maybe<Identifier> tag;
				if (is_next(TokenType::identifier, i)) {
					tag = IdentifierAt(i + 1);
					i++;
				}
				if (not is_next(TokenType::stop, i)) {
					add_error("Expected " + color_error(".") + " after this to end the " + color_error("continue") +
					              " sentence",
					          RangeSpan(start, i));
				}
				i++;
				sentences.push_back(ast::PrerunContinue::create(tag, RangeSpan(start, i)));
				break;
			}
			case TokenType::give: {
				auto                          start = i;
				Maybe<ast::PrerunExpression*> value;
				i++;
				if (not is_next(TokenType::stop, i)) {
					auto expRes = do_prerun_expression(preCtx, i, None);
					value       = expRes.first;
					i           = expRes.second;
				}
				if (not is_next(TokenType::stop, i)) {
					add_error("Expected " + color_error(".") + " after this to end the " + color_error("give") +
					              " sentence",
					          RangeSpan(start, i));
				}
				i++;
				sentences.push_back(ast::PrerunGive::create(value, RangeSpan(start, i)));
				break;
			}
			case TokenType::loop: {
				break;
			}
			default: {
				break;
			}
		}
	}
}

Vec<ast::Sentence*> Parser::do_sentences(ParserContext& preCtx, usize from, usize upto) {
	using ast::FloatType;
	using ast::IntegerType;
	using ir::FloatTypeKind;
	using lexer::Token;
	using lexer::TokenType;

	auto  ctx = preCtx;
	usize i   = from + 1; // NOLINT(readability-identifier-length)

	Maybe<std::tuple<usize, String, FileRange>> totalComment;

	auto addComment = [&](std::tuple<usize, String, FileRange> commValue) {
		if (totalComment.has_value() && (std::get<0>(totalComment.value()) + 1 == std::get<0>(commValue))) {
			std::get<0>(totalComment.value()) = std::get<0>(commValue);
			std::get<1>(totalComment.value()).append(std::get<1>(commValue));
			std::get<2>(totalComment.value()) = std::get<2>(totalComment.value()).spanTo(std::get<2>(commValue));
		} else {
			totalComment = commValue;
		}
	};

	Vec<ast::Sentence*> result;
	auto                addSentence = [&](ast::Sentence* sentence) {
        if (sentence->isCommentable() && totalComment.has_value()) {
            sentence->asCommentable()->commentValue = {std::get<1>(totalComment.value()),
                                                       std::get<2>(totalComment.value())};
        }
        result.push_back(sentence);
	};

	Maybe<CacheSymbol> _cacheSymbol_;
	auto               hasCachedSymbol = [&]() { return _cacheSymbol_.has_value(); };
	auto               setCachedSymbol = [&](const CacheSymbol& symbol) {
        if (_cacheSymbol_.has_value()) {
            add_error(
                "Internal error : A cached symbol is already present, and there was an attempt to cache another symbol",
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
			add_error("Internal error : No cached symbol found, but was requested here", RangeAt(i));
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
	auto                    hasCachedExpr             = [&]() { return _cachedExpression_.has_value(); };
	auto                    setCachedExprForSentences = [&](ast::Expression* value) {
        if (_cachedExpression_.has_value()) {
            add_error(
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
			add_error("Internal error : No cached expression is found, but was requested here", RangeAt(i));
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
			case TokenType::comment: {
				addComment({i, token.value, token.fileRange});
				break;
			}
			case TokenType::meta: {
				if (is_next(TokenType::colon, i) && is_next(TokenType::identifier, i + 1) && ValueAt(i + 2) == "todo") {
					if (is_next(TokenType::parenthesisOpen, i + 2)) {
						if (is_next(TokenType::parenthesisClose, i + 3)) {
							if (is_next(TokenType::stop, i + 4)) {
								result.push_back(ast::MetaTodo::create(None, RangeSpan(i, i + 5)));
								i = i + 5;
							} else {
								add_error("Expected . after this to end the sentence", RangeSpan(i, i + 4));
							}
						} else if (is_next(TokenType::StringLiteral, i + 3)) {
							if (is_next(TokenType::parenthesisClose, i + 4)) {
								if (is_next(TokenType::stop, i + 5)) {
									result.push_back(ast::MetaTodo::create(ValueAt(i + 4), RangeSpan(i, i + 6)));
									i = i + 6;
								} else {
									add_error("Expected . after this to end the sentence", RangeSpan(i, i + 5));
								}
							} else {
								add_error("Expected ) to enclose the meta:todo message", RangeSpan(i, i + 4));
							}
						} else {
							add_error("Expected either meta:todo() or meta:todo(\"message\")", RangeSpan(i, i + 3));
						}
					} else {
						add_error("Expected either meta:todo() or meta:todo(\"message\")", RangeSpan(i, i + 2));
					}
				} else {
					auto expRes = do_expression(preCtx, None, i - 1, None);
					if (tokens->at(expRes.second + 1).type == TokenType::stop) {
						result.push_back(
						    ast::ExpressionSentence::create(expRes.first, RangeSpan(i, expRes.second + 1)));
						i = expRes.second + 1;
					} else {
						setCachedExprForSentences(expRes.first);
						i = expRes.second;
					}
				}
				break;
			}
			case TokenType::super:
			case TokenType::identifier: {
				SHOW("Identifier encountered: " << token.value)
				if (hasCachedSymbol()) {
					add_error("Another symbol found when there already is a symbol", RangeAt(i));
				} else {
					auto symRes = do_symbol(ctx, i);
					setCachedSymbol(symRes.first);
					i = symRes.second;
					if (is_next(TokenType::genericTypeStart, i)) {
						auto expRes = do_expression(preCtx, retrieveCachedSymbol(), i, None);
						setCachedExprForSentences(expRes.first);
						i = expRes.second;
					}
				}
				break;
			}
			case TokenType::selfInstance: {
				if (is_next(TokenType::identifier, i)) {
					if (is_next(TokenType::associatedAssignment, i + 1)) {
						auto stop = first_primary_position(TokenType::stop, i + 2);
						if (stop.has_value()) {
							auto expRes = do_expression(preCtx, None, i + 2, stop);
							SHOW("Member init exp end: " << expRes.second << "; stop at: " << stop.value())
							if ((expRes.second + 1) != stop.value()) {
								add_error("Expression for member initialisation did not span till .",
								          RangeSpan(expRes.second + 1, stop.value()));
							}
							result.push_back(ast::MemberInit::create(IdentifierAt(i + 1), expRes.first, false,
							                                         RangeSpan(i, stop.value())));
							i = stop.value();
							break;
						} else {
							add_error("Expected . to end the expression for member initialisation",
							          RangeSpan(i, i + 2));
						}
					}
				} else if (is_next(TokenType::is, i)) {
					if (is_next(TokenType::identifier, i + 1)) {
						if (is_next(TokenType::stop, i + 2)) {
							result.push_back(
							    ast::MemberInit::create(IdentifierAt(i + 2), nullptr, true, RangeSpan(i, i + 3)));
							i += 3;
							break;
						} else {
							add_error("Expected . to end the member initialisation", RangeSpan(i, i + 2));
						}
					} else {
						add_error("Expected identifier after " + color_error("'' is") +
						              " to specify the variant of mix or choice type to initialise",
						          RangeSpan(i, i + 1));
					}
				}
				auto expRes = do_expression(preCtx, None, i - 1, None);
				setCachedExprForSentences(expRes.first);
				i = expRes.second;
				break;
			}
			case TokenType::assignment: {
				SHOW("Assignment found")
				/* Assignment */
				if (hasCachedSymbol()) {
					auto end_res = first_primary_position(TokenType::stop, i);
					if (!end_res.has_value() || (end_res.value() >= upto)) {
						add_error("Invalid end for the sentence", token.fileRange);
					}
					auto end    = end_res.value();
					auto expRes = do_expression(ctx, None, i, end);
					if ((expRes.second + 1) != end) {
						add_error("Expression to assign did not span till the .", RangeSpan(expRes.second + 1, end));
					}
					auto  symbol = consumeCachedSymbol();
					auto* lhs    = ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange);
					result.push_back(
					    ast::Assignment::create(lhs, expRes.first, FileRange(symbol.fileRange, RangeAt(end))));
					i = end;
				} else if (hasCachedExpr()) {
					SHOW("Assignment has cached expression")
					auto endRes = first_primary_position(TokenType::stop, i);
					if (endRes.has_value() && (endRes.value() < upto)) {
						auto  end    = endRes.value();
						auto* lhs    = consumeCachedExpr();
						auto  expRes = do_expression(preCtx, None, i, end);
						if ((expRes.second + 1) != end) {
							add_error("Expression to assign did not span till the .",
							          RangeSpan(expRes.second + 1, end));
						}
						result.push_back(ast::Assignment::create(lhs, expRes.first, {lhs->fileRange, RangeAt(end)}));
						i = end;
					} else {
						add_error("Invalid end of sentence", {consumeCachedExpr()->fileRange, token.fileRange});
					}
				} else {
					add_error("Expected an expression to assign the expression to", token.fileRange);
				}
				break;
			}
			case TokenType::say: {
				ast::SayType sayTy = ast::SayType::say;
				if (is_next(TokenType::child, i)) {
					if (is_next(TokenType::identifier, i + 1)) {
						if (ValueAt(i + 2) == "dbg") {
							sayTy = ast::SayType::dbg;
						} else if (ValueAt(i + 2) == "only") {
							sayTy = ast::SayType::only;
						} else {
							add_error("Unexpected variant of say found", RangeSpan(i, i + 1));
						}
						i += 2;
					} else {
						add_error("Expected an identifier for the variant of say", RangeSpan(i, i + 1));
					}
				}
				auto end_res = first_primary_position(TokenType::stop, i);
				if (!end_res.has_value() || (end_res.value() >= upto)) {
					add_error("Say sentence has invalid end", token.fileRange);
				}
				auto end  = end_res.value();
				auto exps = do_separated_expressions(ctx, i, end);
				result.push_back(ast::SayLike::create(sayTy, exps, token.fileRange));
				i = end;
				break;
			}
			case TokenType::stop: {
				SHOW("Parsed expression sentence")
				if (hasCachedExpr()) {
					auto* expr = consumeCachedExpr();
					result.push_back(ast::ExpressionSentence::create(expr, {expr->fileRange, token.fileRange}));
				} else if (hasCachedSymbol()) {
					auto symbol = consumeCachedSymbol();
					result.push_back(ast::ExpressionSentence::create(
					    ast::Entity::create(symbol.relative, symbol.name, symbol.fileRange), symbol.fileRange));
				}
				break;
			}
			case TokenType::match: {
				auto start       = i;
				bool isTypeMatch = false;
				if (is_next(TokenType::Type, i)) {
					isTypeMatch = true;
					i++;
				}
				auto expRes = do_expression(preCtx, None, i, None);
				if (is_next(TokenType::altArrow, expRes.second)) {
					i = expRes.second + 1;
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
						if (bCloseRes) {
							Vec<Pair<Vec<ast::MatchValue*>, Vec<ast::Sentence*>>> chain;
							Maybe<Pair<Vec<ast::Sentence*>, FileRange>>           elseCase;
							parse_match_contents(preCtx, i + 1, bCloseRes.value(), chain, elseCase, isTypeMatch);
							result.push_back(ast::Match::create(isTypeMatch, expRes.first, std::move(chain),
							                                    std::move(elseCase),
							                                    RangeSpan(start, bCloseRes.value())));
							i = bCloseRes.value();
						} else {
							add_error("Expected end for [", RangeAt(i + 1));
						}
					} else {
						add_error("Expected [ here to start the cases for the match", RangeSpan(start, i));
					}
				} else {
					add_error("Expected => after the expression to be matched", expRes.first->fileRange);
				}
				break;
			}
			case TokenType::let: {
				const auto start = i;
				SHOW("Found new")
				bool isRef     = false;
				bool isVarDecl = false;
				if (is_next(TokenType::var, i)) {
					isVarDecl = true;
					if (is_next(TokenType::referenceType, i + 1)) {
						isRef = true;
						i += 2;
					} else {
						i++;
					}
				} else if (is_next(TokenType::referenceType, i)) {
					isRef = true;
					i++;
				}
				if (is_next(TokenType::identifier, i)) {
					auto name = IdentifierAt(i + 1);
					if (is_next(TokenType::assignment, i + 1)) {
						auto expRes = do_expression(preCtx, None, i + 2, None);
						i           = expRes.second;
						if (is_next(TokenType::stop, i)) {
							i++;
							result.push_back(ast::LocalDeclaration::create(nullptr, isRef, name, expRes.first,
							                                               isVarDecl, RangeSpan(start, i)));
						} else {
							add_error("Expected . to end the local declaration", RangeSpan(start, i));
						}
					} else if (is_next(TokenType::typeSeparator, i + 1)) {
						auto endRes = first_primary_position(TokenType::stop, i + 2);
						if (endRes.has_value()) {
							if (is_primary_within(TokenType::assignment, i + 2, endRes.value())) {
								auto  asgnPos = first_primary_position(TokenType::assignment, i + 2).value();
								auto* typeRes = do_type(preCtx, i + 2, asgnPos).first;
								auto* exp     = do_expression(preCtx, None, asgnPos, endRes.value()).first;
								result.push_back(ast::LocalDeclaration::create(typeRes, isRef, IdentifierAt(i + 1), exp,
								                                               isVarDecl,
								                                               RangeSpan(start, endRes.value())));
							} else {
								// No value for the assignment
								auto* typeRes = do_type(preCtx, i + 2, endRes.value()).first;
								result.push_back(ast::LocalDeclaration::create(typeRes, isRef, IdentifierAt(i + 1),
								                                               None, isVarDecl,
								                                               RangeSpan(start, endRes.value())));
							}
							i = endRes.value();
							break;
						} else {
							add_error("Expected . to mark the end of this declaration", RangeSpan(start, i + 1));
						}
					} else if (is_next(TokenType::from, i + 1)) {
						add_error("Initialisation via constructor or convertor can be used "
						          "only if there is a type provided. This is an inferred "
						          "declaration",
						          RangeSpan(start, i + 2));
					} else if (is_next(TokenType::stop, i + 1)) {
						add_error("Only declarations with `maybe` type can omit the value to be assigned",
						          RangeSpan(start, i + 2));
					} else {
						add_error("Unexpected token found after the beginning of local declaration", RangeAt(i + 1));
					}
				} else {
					add_error("Expected an identifier for the name of the local value", RangeSpan(start, i));
				}
				break;
			}
			case TokenType::If: {
				Vec<std::tuple<ast::Expression*, Vec<ast::Sentence*>, FileRange>> chain;
				Maybe<Pair<Vec<ast::Sentence*>, FileRange>>                       elseCase;
				FileRange                                                         fileRange = token.fileRange;
				auto                                                              start     = i;
				usize                                                             index     = 0;
				while (true) {
					auto altPos = first_primary_position(TokenType::altArrow, i);
					if (altPos.has_value()) {
						auto expRes = do_expression(ctx, None, i, altPos.value());
						if (expRes.second + 1 != altPos.value()) {
							add_error("Condition for " + color_error(index == 0 ? "if" : "else if") +
							              " do not span till => ",
							          RangeSpan(expRes.second + 1, altPos.value()));
						}
						i = altPos.value();
						if (is_next(TokenType::bracketOpen, i)) {
							auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
							if (bCloseRes.has_value()) {
								auto bClose = bCloseRes.value();
								fileRange   = FileRange(fileRange, RangeAt(bClose));
								auto snts   = do_sentences(preCtx, i + 1, bClose);
								chain.push_back({expRes.first, snts, RangeSpan(i, bClose)});
								if (is_next(TokenType::Else, bClose)) {
									SHOW("Found else")
									if (is_next(TokenType::If, bClose + 1)) {
										i = bClose + 2;
										index++;
										continue;
									} else if (is_next(TokenType::bracketOpen, bCloseRes.value() + 1)) {
										SHOW("Else case begin")
										auto bOp = bClose + 2;
										auto bClRes =
										    get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, bOp);
										if (bClRes) {
											fileRange = FileRange(fileRange, RangeAt(bClRes.value()));
											elseCase  = {do_sentences(preCtx, bOp, bClRes.value()),
											             RangeSpan(bClose + 1, bClRes.value())};
											i         = bClRes.value();
											break;
										} else {
											add_error("Expected end for [", RangeAt(bOp));
										}
									} else {
										add_error("Expected [ to start the body for " + color_error("else"),
										          RangeAt(bClose + 1));
									}
								} else {
									i = bClose;
									break;
								}
							} else {
								add_error("Expected end for [", RangeAt(bCloseRes.value()));
							}
						} else {
							add_error("Expected [ to start the body of the " +
							              color_error(index == 0 ? "if" : "else if") + " block",
							          index == 0 ? RangeAt(i) : FileRange RangeSpan(i - 1, i));
						}
					} else {
						add_error("Expected => to end the condition for " + color_error(index == 0 ? "if" : "else if") +
						              " block",
						          RangeAt(i));
					}
				}
				result.push_back(ast::IfElse::create(chain, elseCase, fileRange));
				break;
			}
			case TokenType::Do: {
				auto              start = i;
				Maybe<Identifier> tagVal;
				if (is_next(TokenType::altArrow, i)) {
					if (is_next(TokenType::identifier, i + 1)) {
						tagVal = IdentifierAt(i + 2);
						i += 2;
					} else {
						add_error("Expected an identifier for the tag of the " + color_error("do-loop-if"),
						          RangeAt(i + 1));
					}
				}
				if (is_next(TokenType::bracketOpen, i)) {
					auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
					if (bCloseRes.has_value()) {
						auto bClose = bCloseRes.value();
						if (is_next(TokenType::loop, bClose) && is_next(TokenType::If, bClose + 1)) {
							auto stopPos = first_primary_position(TokenType::stop, bClose + 2);
							if (stopPos.has_value()) {
								auto sentences = do_sentences(ctx, i + 1, bClose);
								auto condRes   = do_expression(ctx, None, bClose + 2,
								                               first_primary_position(TokenType::stop, bClose + 2));
								if (condRes.second + 1 != stopPos.value()) {
									add_error("Condition for the " + color_error("do-loop-if") +
									              " did not span till the .",
									          RangeSpan(condRes.second + 1, stopPos.value()));
								}
								result.push_back(ast::LoopIf::create(true, condRes.first, sentences, tagVal,
								                                     RangeSpan(start, stopPos.value())));
								i = stopPos.value();
							} else {
								add_error("Expected . to end the condition for the " + color_error("do-loop-if"),
								          RangeSpan(start, bClose + 2));
							}
						} else {
							add_error("Expected " + color_error("do-loop-if") +
							              " after ] to precede the condition for the loop",
							          RangeAt(bClose));
						}
					} else {
						add_error("Expected end for [", RangeAt(i + 1));
					}
				} else {
					add_error("Expected [ to start the body of the " + color_error("do-loop-if"), RangeAt(i));
				}
				break;
			}
			case TokenType::loop: {
				if (is_next(TokenType::child, i)) {
					auto expRes = do_expression(preCtx, None, i > 0 ? i - 1 : 0, None);
					setCachedExprForSentences(expRes.first);
					i = expRes.second;
					break;
				} else if (is_next(TokenType::altArrow, i)) {
					auto              start = i;
					Maybe<Identifier> tagValue;
					if (is_next(TokenType::identifier, i + 1)) {
						tagValue = IdentifierAt(i + 2);
						i += 2;
					} else {
						i++;
					}
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
						if (bCloseRes.has_value()) {
							auto bClose    = bCloseRes.value();
							auto sentences = do_sentences(preCtx, i + 1, bClose);
							result.push_back(ast::LoopInfinite::create(sentences, tagValue, RangeSpan(start, bClose)));
							i = bClose;
						} else {
							add_error("Expected end for [", RangeAt(i + 2));
						}
					} else {
						add_error("Expected [ to start the body of the loop", RangeSpan(start, i));
					}
				} else if (is_next(TokenType::in, i)) {
					auto start = i;
					auto exp   = do_expression(preCtx, None, i + 1, None);
					i          = exp.second;
					if (not is_next(TokenType::altArrow, i)) {
						add_error("Expected => to end the expression for the item to loop over", RangeSpan(start, i));
					}
					i++;
					Maybe<Identifier> itemName;
					Maybe<Identifier> indexName;
					if (is_next(TokenType::let, i)) {
						if (not is_next(TokenType::identifier, i + 1)) {
							add_error("Expected the name for the item of the loop after this", RangeAt(i + 1));
						}
						itemName = IdentifierAt(i + 2);
						if (is_next(TokenType::separator, i + 2)) {
							if (not is_next(TokenType::identifier, i + 3)) {
								add_error("Expected the name for the index of the loop after ,", RangeAt(i + 3));
							}
							indexName = IdentifierAt(i + 4);
							i += 4;
						} else {
							i += 2;
						}
					} else {
						add_error("Expected " + color_error("let") +
						              " after this to start the declaration for the element of the loop",
						          RangeSpan(start, i));
					}
					if (not is_next(TokenType::bracketOpen, i)) {
						add_error("Expected [ to start the block of sentences for this loop", RangeSpan(start, i));
					}
					auto bClose = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
					if (not bClose.has_value()) {
						add_error("Expected ] to end the block of sentences for this loop", RangeAt(i + 1));
					}
					auto sentences = do_sentences(preCtx, i + 1, bClose.value());
					i              = bClose.value();
					result.push_back(ast::LoopIn::create(exp.first, std::move(sentences), std::move(itemName.value()),
					                                     std::move(indexName), RangeSpan(start, i)));
					break;
				} else if (is_next(TokenType::If, i)) {
					auto start = i;
					if (first_primary_position(TokenType::altArrow, i + 1).has_value()) {
						auto altPos = first_primary_position(TokenType::altArrow, i + 1).value();
						auto cond   = do_expression(preCtx, None, i + 1, altPos);
						if (cond.second + 1 != altPos) {
							add_error("The condition for the " + color_error("loop if") + " did not span till the =>",
							          RangeSpan(cond.second + 1, altPos));
						}
						i = altPos;
						Maybe<Identifier> tagValue;
						if (is_next(TokenType::identifier, altPos)) {
							tagValue = IdentifierAt(altPos + 1);
							i++;
						} else if (is_next(TokenType::let, altPos)) {
							add_error("Please remove " + color_error("let") +
							              " from here. As this is a conditional loop, there is no loop counter or item",
							          RangeAt(altPos + 1));
						}
						if (is_next(TokenType::bracketOpen, i)) {
							auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
							if (bCloseRes.has_value()) {
								auto snts = do_sentences(preCtx, i + 1, bCloseRes.value());
								result.push_back(ast::LoopIf::create(false, cond.first, snts, tagValue,
								                                     RangeSpan(start, bCloseRes.value())));
								i = bCloseRes.value();
								break;
							} else {
								add_error("Expected end for [", RangeAt(altPos + 1));
							}
						} else {
							add_error("Expected [ to after this to start the body of the loop", RangeSpan(start, i));
						}
					} else {
						add_error("Expected => to end the condition for the " + color_error("loop if"),
						          RangeSpan(i, i + 1));
					}
				} else if (is_next(TokenType::to, i)) {
					auto              start    = i;
					Maybe<Identifier> loopTag  = None;
					auto              countRes = do_expression(preCtx, None, i, None);
					i                          = countRes.second;
					if (is_next(TokenType::altArrow, i)) {
						i++;
					} else {
						add_error("Expected => after the count of the " + color_error("loop to"), RangeSpan(start, i));
					}
					if (is_next(TokenType::let, i)) {
						if (is_next(TokenType::identifier, i + 1)) {
							loopTag = IdentifierAt(i + 2);
							i += 2;
						} else {
							add_error("Expected an identifier after " + color_error("let") +
							              " for the name of the loop counter variable",
							          RangeAt(i + 1));
						}
					} else if (is_next(TokenType::identifier, i)) {
						add_error("If this is meant to be a loop with a counter variable. Please use " +
						              color_error("let " + ValueAt(i + 1)) + " instead here",
						          RangeAt(i + 1));
					}
					if (is_next(TokenType::bracketOpen, i)) {
						auto bCloseRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i + 1);
						if (bCloseRes.has_value()) {
							auto snts = do_sentences(preCtx, i + 1, bCloseRes.value());
							result.push_back(ast::LoopTo::create(countRes.first,
							                                     do_sentences(preCtx, i + 1, bCloseRes.value()),
							                                     loopTag, RangeSpan(i, bCloseRes.value())));
							i = bCloseRes.value();
						} else {
							add_error("Expected end for [", RangeAt(i + 1));
						}
					} else {
						add_error("Expected [ to start the body of the loop", RangeSpan(start, i));
					}
				} else {
					add_error("Invalid type of loop", token.fileRange);
				}
				break;
			}
			case TokenType::give: {
				SHOW("give sentence found")
				if (is_next(TokenType::stop, i)) {
					i++;
					result.push_back(ast::GiveSentence::create(None, FileRange(token.fileRange, RangeAt(i + 1))));
				} else {
					auto end = first_primary_position(TokenType::stop, i);
					if (!end.has_value()) {
						add_error("Expected give sentence to end. Please add `.` wherever "
						          "appropriate to mark the end of the statement",
						          token.fileRange);
					}
					auto* exp = do_expression(ctx, None, i, end.value()).first;
					i         = end.value();
					result.push_back(ast::GiveSentence::create(exp, FileRange(token.fileRange, RangeAt(end.value()))));
				}
				break;
			}
			case TokenType::Break: {
				if (is_next(TokenType::child, i)) {
					if (is_next(TokenType::identifier, i + 1)) {
						result.push_back(ast::Break::create(IdentifierAt(i + 2), RangeSpan(i, i + 2)));
						i += 2;
					} else {
						add_error("Expected an identifier after break'", RangeSpan(i, i + 2));
					}
				} else if (is_next(TokenType::stop, i)) {
					result.push_back(ast::Break::create(None, token.fileRange));
					i++;
				} else {
					add_error("Unexpected token found after break", token.fileRange);
				}
				break;
			}
			case TokenType::Continue: {
				if (is_next(TokenType::child, i)) {
					if (is_next(TokenType::identifier, i + 1)) {
						result.push_back(ast::Continue::create(IdentifierAt(i + 2), RangeSpan(i, i + 2)));
						i += 2;
					} else {
						add_error("Expected an identifier after continue'", RangeSpan(i, i + 2));
					}
				} else if (is_next(TokenType::stop, i)) {
					result.push_back(ast::Continue::create(None, token.fileRange));
					i++;
				} else {
					add_error("Unexpected token found after continue", token.fileRange);
				}
				break;
			}
			case TokenType::assignedBinaryOperator: {
				if (!hasCachedSymbol() && !hasCachedExpr()) {
					add_error("No expression found before the " + token.value + " operator", RangeAt(i));
				}
				if (is_primary_within(TokenType::stop, i, upto)) {
					auto end_res = first_primary_position(TokenType::stop, i).value();
					auto lhs     = do_expression(preCtx, retrieveCachedSymbol(), i, i, retrieveCachedExpr()).first;
					auto rhs     = do_expression(preCtx, None, i, end_res, None).first;
					auto bin_exp =
					    ast::BinaryExpression::create(lhs, token.value, rhs, FileRange{lhs->fileRange, rhs->fileRange});
					result.push_back(
					    ast::ExpressionSentence::create(bin_exp, FileRange{bin_exp->fileRange, RangeAt(end_res)}));
				} else {
					add_error("Detected " + token.value +
					              " operator and expected . within this scope to end the sentence",
					          RangeAt(i));
				}
				break;
			}
			case TokenType::associatedAssignment: {
				// FIXME - ?? CHange this to support init of any reference
				const auto hasNameBefore = (hasCachedSymbol() && _cacheSymbol_.value().name.size() == 1);
				add_error(":= is only supported for initialising member fields of a struct type" +
				              (hasNameBefore ? (". Did you forget to add '' before " +
				                                color_error(_cacheSymbol_.value().name.front().value))
				                             : ""),
				          hasNameBefore ? FileRange{_cacheSymbol_->fileRange, RangeAt(i)} : RangeAt(i));
				break;
			}
			default: {
				SHOW("parseSentences - default case")
				auto expRes = do_expression(preCtx, retrieveCachedSymbol(), i - 1, None, retrieveCachedExpr());
				setCachedExprForSentences(expRes.first);
				i = expRes.second;
				if (is_next(TokenType::stop, i)) {
					auto expr = consumeCachedExpr();
					result.push_back(ast::ExpressionSentence::create(expr, expr->fileRange));
					i++;
				}
			}
		}
	}
	return result;
}

Pair<Vec<ast::Argument*>, bool> Parser::do_function_parameters(ParserContext& preCtx, usize from, usize upto) {
	using ast::FloatType;
	using ast::IntegerType;
	using ir::FloatTypeKind;
	using lexer::TokenType;
	SHOW("Starting parsing function parameters")
	Vec<ast::Argument*> args;

	for (usize i = from + 1; ((i < upto) && (i < tokens->size())); i++) {
		auto& token = tokens->at(i);
		switch (token.type) { // NOLINT(clang-diagnostic-switch)
			case TokenType::var: {
				if (is_next(TokenType::identifier, i)) {
					break;
				} else {
					add_error("Expected an identifier for the argument name after " + color_error("var"), RangeAt(i));
				}
			}
			case TokenType::identifier: {
				bool isVar = is_previous(TokenType::var, i);
				if (is_next(TokenType::typeSeparator, i)) {
					auto typRes = do_type(preCtx, i + 1, upto);
					if (typRes.second > upto) {
						add_error("Invalid end of argument type", typRes.first->fileRange);
					}
					if (typRes.first->type_kind() == ast::AstTypeKind::VOID) {
						add_error("Arguments can't be of type void", typRes.first->fileRange);
					}
					args.push_back(ast::Argument::create_normal(IdentifierAt(i), isVar, typRes.first));
					i = typRes.second;
					if (is_next(TokenType::separator, i)) {
						i++;
					}
				} else {
					add_error("Expected :: after the argument name", RangeAt(i));
				}
				break;
			}
			case TokenType::variadic: {
				if (is_next(TokenType::identifier, i)) {
					// FIXME - Variadic argument can be var?
					args.push_back(
					    ast::Argument::create_normal({ValueAt(i + 1), FileRange RangeSpan(i, i + 1)}, false, nullptr));
					if (is_next(TokenType::parenthesisClose, i + 1) ||
					    (is_next(TokenType::separator, i + 1) && ((i + 3) == upto))) {
						return {args, true};
					} else {
						add_error("Variadic argument should be the last argument of the function", token.fileRange);
					}
				} else {
					add_error("Expected name for the variadic argument. Please provide a name", token.fileRange);
				}
				break;
			}
			case TokenType::selfInstance: {
				if (is_next(TokenType::identifier, i)) {
					SHOW("Creating member argument: " << ValueAt(i + 1))
					args.push_back(ast::Argument::create_member({ValueAt(i + 1), RangeAt(i + 1)}, false, nullptr));
					if (is_next(TokenType::separator, i + 1) || is_next(TokenType::parenthesisClose, i + 1)) {
						i += 2;
					} else {
						add_error("Invalid token found after argument", RangeSpan(i, i + 1));
					}
				} else {
					add_error("Expected name of the member to be initialised", token.fileRange);
				}
				break;
			}
			default: {
				add_error("Expected name for the argument", RangeAt(i));
			}
		}
	}
	// FIXME - Support variadic function parameters
	return {args, false};
}

Maybe<usize> Parser::get_pair_end(const lexer::TokenType startType, const lexer::TokenType endType,
                                  const usize current) {
	usize collisions = 0;
	for (usize i = current + 1; i < tokens->size(); i++) {
		// SHOW("GetPairEnd :: Index = " << i << ", Token Type = " << (int)tokens->at(i).type)
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

bool Parser::is_previous(const lexer::TokenType type, const usize from) {
	if ((from - 1) >= 0) {
		return tokens->at(from - 1).type == type;
	} else {
		return false;
	}
}

bool Parser::are_only_present_within(const Vec<lexer::TokenType>& kinds, usize from, usize upto) {
	for (usize i = from + 1; i < upto; i++) {
		for (auto const& kind : kinds) {
			if (kind != tokens->at(i).type) {
				return false;
			}
		}
	}
	return true;
}

bool Parser::is_primary_within(lexer::TokenType candidate, usize from, usize upto) {
	auto posRes = first_primary_position(candidate, from);
	return posRes.has_value() && (posRes.value() < upto);
}

// bool Parser::isNotPartOfExpression(usize from, usize upto) {
//   using lexer::TokenType;
//   return isPrimaryWithin(TokenType::stop, from, upto) || isPrimaryWithin(TokenType::New, from, upto) ||
//          isPrimaryWithin(TokenType::give, from, upto);
// }

Maybe<usize> Parser::first_primary_position(const lexer::TokenType candidate, const usize from) {
	using lexer::TokenType;
	for (usize i = from + 1; i < tokens->size(); i++) {
		auto tok = tokens->at(i);
		if (tok.type == candidate) {
			return i;
		} else if (tok.type == TokenType::parenthesisOpen) {
			auto endRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i);
			if (endRes.has_value() && (endRes.value() < tokens->size())) {
				i = endRes.value();
			} else {
				return None;
			}
		} else if (tok.type == TokenType::bracketOpen) {
			auto endRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i);
			if (endRes.has_value() && (endRes.value() < tokens->size())) {
				i = endRes.value();
			} else {
				return None;
			}
		} else if (tok.type == TokenType::curlybraceOpen) {
			auto endRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i);
			if (endRes.has_value() && (endRes.value() < tokens->size())) {
				i = endRes.value();
			} else {
				return None;
			}
		} else if (tok.type == TokenType::genericTypeStart) {
			//   SHOW("FirstPrimaryPosition :: Generic type start: " << tok.fileRange << ", Index: " << i)
			auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i);
			if (endRes.has_value() && (endRes.value() < tokens->size())) {
				// SHOW("FirstPrimaryPosition :: Generic type end: " << RangeAt(endRes.value()))
				i = endRes.value();
			} else {
				return None;
			}
		}
	}
	return None;
}

Vec<usize> Parser::primary_positions_within(lexer::TokenType candidate, usize from, usize upto) {
	Vec<usize> result;
	using lexer::TokenType;
	for (usize i = from + 1; (i < upto && i < tokens->size()); i++) {
		auto tok = tokens->at(i);
		if (tok.type == TokenType::parenthesisOpen) {
			auto endRes = get_pair_end(TokenType::parenthesisOpen, TokenType::parenthesisClose, i);
			if (endRes.has_value() && (endRes.value() < upto)) {
				i = endRes.value();
			} else {
				return result;
			}
		} else if (tok.type == TokenType::bracketOpen) {
			auto endRes = get_pair_end(TokenType::bracketOpen, TokenType::bracketClose, i);
			if (endRes.has_value() && (endRes.value() < upto)) {
				i = endRes.value();
			} else {
				return result;
			}
		} else if (tok.type == TokenType::curlybraceOpen) {
			auto endRes = get_pair_end(TokenType::curlybraceOpen, TokenType::curlybraceClose, i);
			if (endRes.has_value() && (endRes.value() < upto)) {
				i = endRes.value();
			} else {
				return result;
			}
		} else if (tok.type == TokenType::genericTypeStart) {
			auto endRes = get_pair_end(TokenType::genericTypeStart, TokenType::genericTypeEnd, i);
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

void Parser::add_error(const String& message, const FileRange& fileRange) { irCtx->Error(message, fileRange); }

String Parser::color_error(const String& message) {
	auto* cfg = cli::Config::get();
	return ColoredOr(cli::Color::yellow, "`") + message + ColoredOr(cli::Color::white, "`");
}

void Parser::add_warning(const String& message, const FileRange& fileRange) {
	std::cout << cli::get_bg_color(cli::Color::orange) << " PARSER WARNING " << cli::get_color(cli::Color::reset)
	          << "▌ " << cli::get_color(cli::Color::yellow) << message << cli::get_color(cli::Color::reset) << " | "
	          << cli::get_color(cli::Color::green) << fileRange.file.string() << ":" << fileRange.start.line << ":"
	          << fileRange.start.character << cli::get_color(cli::Color::reset) << " >> "
	          << cli::get_color(cli::Color::green) << fileRange.file.string() << ":" << fileRange.end.line << ":"
	          << fileRange.end.character << cli::get_color(cli::Color::reset) << "\n";
}

} // namespace qat::parser
