#include "./json_parser.hpp"
#include "./json.hpp"
#include "helpers.hpp"
#include <optional>
#include <vector>

namespace qat {

bool JsonParser::lex(String val) {
	const String digits = "0123456789";
	const String alpha  = "truefalsn";
	for (usize i = 0; i < val.size(); i++) {
		auto chr = val.at(i);
		if (chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t') {
			continue;
		} else if (chr == '{') {
			toks.emplace_back(Token(TokenType::curlyBraceOpen));
		} else if (chr == '}') {
			toks.emplace_back(Token(TokenType::curlyBraceClose));
		} else if (chr == '[') {
			toks.emplace_back(Token(TokenType::bracketOpen));
		} else if (chr == ']') {
			toks.emplace_back(Token(TokenType::bracketClose));
		} else if (chr == ':') {
			toks.emplace_back(Token(TokenType::colon));
		} else if (chr == ',') {
			toks.emplace_back(Token(TokenType::comma));
		} else if (chr == '"') {
			String str;
			bool   isEscape = false;
			usize  j        = i + 1; // NOLINT(readability-identifier-length)
			for (; (isEscape ? true : val.at(j) != '"') && (j < val.size()); j++) {
				if (isEscape) {
					if (val.at(j) == '"') {
						str += '"';
					} else if (val.at(j) == 'b') {
						str += '\b';
					} else if (val.at(j) == 'f') {
						str += '\f';
					} else if (val.at(j) == 'n') {
						str += '\n';
					} else if (val.at(j) == 't') {
						str += '\t';
					} else if (val.at(j) == '\\') {
						str += "\\";
					} else {
						return false;
					}
					isEscape = false;
				} else {
					if (val.at(j) == '\\') {
						isEscape = true;
					} else {
						str += val.at(j);
					}
				}
			}
			i = j;
			toks.emplace_back(Token(TokenType::string));
		} else if ((digits.find(val.at(i)) != String::npos) || (val.at(i) == '-')) {
			bool   is_float = false;
			String num(val.substr(i, 1));
			String decimal;
			usize  jInd = i + 1;
			for (; ((is_float ? (digits.find(val.at(jInd)) != String::npos)
			                  : ((digits.find(val.at(jInd)) != String::npos) || (val.at(jInd) == '.'))) &&
			        (jInd < val.size()));
			     jInd++) {
				if (is_float) {
					decimal += val.at(jInd);
				} else {
					if (val.at(jInd) != '.') {
						num += val.at(jInd);
					} else {
						is_float = true;
					}
				}
			}
			i = jInd - 1;
			if (decimal.empty()) {
				is_float = false;
			} else {
				bool onlyZeroes = true;
				for (auto dec : decimal) {
					if (dec != '0') {
						onlyZeroes = false;
					}
				}
				is_float = !onlyZeroes;
			}
			if (is_float) {
				toks.emplace_back(Token(TokenType::floating, num.append(".").append(decimal)));
			} else {
				toks.emplace_back(Token(TokenType::integer, num));
			}
		} else if (alpha.find(val.at(i)) != String::npos) {
			String idt(val.substr(i, 1));
			usize  j = i + 1; // NOLINT(readability-identifier-length)
			for (; ((alpha.find(val.at(j)) != String::npos) && (j < val.size())); j++) {
				idt += val.at(j);
			}
			if (idt == "true") {
				toks.emplace_back(Token(TokenType::True));
			} else if (idt == "false") {
				toks.emplace_back(Token(TokenType::False));
			} else if (idt == "null") {
				toks.emplace_back(Token(TokenType::null));
			} else {
				return false;
			}
			i = j - 1;
		} else {
			return false;
		}
	}
	return true;
}

bool JsonParser::isNext(TokenType type, usize pos = 0) const {
	return (pos < toks.size()) ? (toks.at(pos + 1).type == type) : false;
}

std::optional<usize> JsonParser::getPairEnd(bool isList, usize from, std::optional<usize> upto) const {
	unsigned collisions = 0;
	for (usize i = from + 1; ((upto.has_value() ? (i < upto.value()) : true) && (i < toks.size())); i++) {
		auto tok = toks.at(i);
		if (isList) {
			if (tok.type == TokenType::bracketOpen) {
				collisions++;
			} else if (tok.type == TokenType::bracketClose) {
				if (collisions > 0) {
					collisions--;
				} else {
					return i;
				}
			}
		} else {
			if (tok.type == TokenType::curlyBraceOpen) {
				collisions++;
			} else if (tok.type == TokenType::curlyBraceClose) {
				if (collisions > 0) {
					collisions--;
				} else {
					return i;
				}
			}
		}
	}
	return std::nullopt;
}

Maybe<bool> JsonParser::hasPrimaryCommas(usize from, usize upto) const {
	for (usize i = from + 1; i < upto; i++) {
		auto tok = toks.at(i);
		switch (tok.type) {
			case TokenType::curlyBraceOpen:
			case TokenType::bracketOpen: {
				bool isList    = (tok.type == TokenType::bracketOpen);
				auto bCloseRes = getPairEnd(isList, i, upto);
				if (bCloseRes.has_value()) {
					i = bCloseRes.value();
				} else {
					return None;
				}
				break;
			}
			case TokenType::comma: {
				return true;
			}
			default: {
				continue;
			}
		}
	}
	return false;
}

Maybe<Vec<usize>> JsonParser::getPrimaryCommas(usize from, usize upto) const {
	Vec<usize> result;
	for (usize i = from + 1; i < upto; i++) {
		auto tok = toks.at(i);
		switch (tok.type) {
			case TokenType::curlyBraceOpen:
			case TokenType::bracketOpen: {
				bool isList    = (tok.type == TokenType::bracketOpen);
				auto bCloseRes = getPairEnd(isList, i, upto);
				if (bCloseRes.has_value()) {
					i = bCloseRes.value();
				} else {
					return None;
				}
				break;
			}
			case TokenType::comma: {
				result.push_back(i);
				break;
			}
			default: {
				continue;
			}
		}
	}
	return result;
}

Maybe<JsonValue> JsonParser::parseValue(usize from, usize upto) const {
	for (usize i = from + 1; i < upto; i++) {
		const auto& tok = toks.at(i);
		switch (tok.type) {
			case TokenType::True:
			case TokenType::False: {
				return JsonValue(tok.type == TokenType::True);
			}
			case TokenType::curlyBraceOpen: {
				auto bCloseRes = getPairEnd(false, i, upto);
				if (bCloseRes.has_value()) {
					auto json = parse(i - 1, bCloseRes.value() + 1);
					return json.has_value() ? JsonValue(json.value()) : Maybe<JsonValue>(None);
				} else {
					return None;
				}
			}
			case TokenType::string: {
				return JsonValue(tok.value);
			}
			case TokenType::integer: {
				return JsonValue(std::stoi(tok.value));
			}
			case TokenType::floating: {
				return JsonValue(std::stod(tok.value));
			}
			case TokenType::curlyBraceClose:
			case TokenType::comma:
			case TokenType::colon: {
				return None;
			}
			case TokenType::null: {
				return JsonValue();
			}
			case TokenType::bracketOpen: {
				auto bCloseRes = getPairEnd(true, i, upto);
				if (bCloseRes.has_value()) {
					auto           bClose = bCloseRes.value();
					Vec<JsonValue> vals;
					auto           hasPComma = hasPrimaryCommas(i, bClose);
					if (hasPComma) {
						if (hasPComma.value()) {
							auto sepPos = getPrimaryCommas(i, bClose);
							if (sepPos.has_value()) {
								auto firstVal = parseValue(i, sepPos->front());
								if (firstVal) {
									vals.push_back(firstVal.value());
									for (usize j = 0; j < (sepPos->size() - 1); j++) {
										auto midVal = parseValue(sepPos->at(j), sepPos->at(j + 1));
										if (midVal.value()) {
											vals.push_back(midVal.value());
										} else {
											return None;
										}
									}
									auto lastVal = parseValue(sepPos->back(), bClose);
									if (lastVal) {
										vals.push_back(lastVal.value());
									} else {
										return None;
									}
								} else {
									return None;
								}
							} else {
								return None;
							}
						} else if (i != bClose) {
							auto val = parseValue(i, bClose);
							if (val) {
								vals.push_back(val.value());
							} else {
								return None;
							}
						}
						return JsonValue(vals);
					} else {
						return None;
					}
				} else {
					return None;
				}
			}
			case TokenType::bracketClose:
				return None;
		}
	}
	return JsonValue::none();
}

Maybe<Vec<Pair<String, JsonValue>>> JsonParser::parsePairs(usize from, usize upto) const {
	Vec<Pair<String, JsonValue>> result;
	for (usize i = from + 1; i < upto; i++) {
		auto tok = toks.at(i);
		if (tok.type == TokenType::string) {
			if (isNext(TokenType::colon, i)) {
				switch (toks.at(i + 2).type) {
					case TokenType::True:
					case TokenType::False: {
						if (isNext(TokenType::comma, i + 2) || isNext(TokenType::curlyBraceClose, i + 2)) {
							auto val = parseValue(i + 1, i + 3);
							if (val.has_value()) {
								result.push_back(Pair<String, JsonValue>(tok.value, val.value()));
								i += 2;
								break;
							} else {
								return None;
							}
						} else {
							return None;
						}
					}
					case TokenType::curlyBraceOpen: {
						auto bCloseRes = getPairEnd(false, i + 2, upto);
						if (bCloseRes.has_value()) {
							auto bClose = bCloseRes.value();
							if (isNext(TokenType::comma, bClose) || isNext(TokenType::curlyBraceClose, bClose)) {
								auto jsonVal = parse(i + 1, bClose + 1);
								if (jsonVal) {
									result.push_back(Pair<String, JsonValue>(tok.value, JsonValue(jsonVal.value())));
									i = bClose;
									break;
								} else {
									return None;
								}
							} else {
								return None;
							}
						} else {
							return None;
						}
					}
					case TokenType::curlyBraceClose: {
						return None;
					}
					case TokenType::string: {
						if (isNext(TokenType::comma, i + 2) || isNext(TokenType::curlyBraceClose, i + 2)) {
							auto val = parseValue(i + 1, i + 3);
							if (val) {
								result.push_back(Pair<String, JsonValue>(tok.value, val.value()));
								i += 2;
								break;
							} else {
								return None;
							}
						} else {
							return None;
						}
					}
					case TokenType::integer: {
						if (isNext(TokenType::comma, i + 2) || isNext(TokenType::curlyBraceClose, i + 2)) {
							auto val = parseValue(i + 1, i + 3);
							if (val) {
								result.push_back(Pair<String, JsonValue>(tok.value, val.value()));
								i += 2;
								break;
							} else {
								return None;
							}
						} else {
							return None;
						}
					}
					case TokenType::floating: {
						if (isNext(TokenType::comma, i + 2) || isNext(TokenType::curlyBraceClose, i + 2)) {
							auto val = parseValue(i + 1, i + 3);
							if (val) {
								result.push_back(Pair<String, JsonValue>(tok.value, val.value()));
								i += 2;
								break;
							} else {
								return None;
							}
						} else {
							return None;
						}
					}
					case TokenType::bracketOpen: {
						auto bCloseRes = getPairEnd(true, i + 2, upto);
						if (bCloseRes.has_value()) {
							auto bClose = bCloseRes.value();
							if (isNext(TokenType::comma, bClose) || isNext(TokenType::curlyBraceClose, bClose)) {
								auto val = parseValue(i + 1, bClose + 1);
								if (val) {
									result.push_back(Pair<String, JsonValue>(tok.value, val.value()));
									i = bClose;
									break;
								} else {
									return None;
								}
							} else {
								return None;
							}
						} else {
							return None;
						}
					}
					case TokenType::null: {
						if (isNext(TokenType::comma, i + 2) || isNext(TokenType::curlyBraceClose, i + 2)) {
							result.push_back(Pair<String, JsonValue>(tok.value, JsonValue()));
							i += 2;
							break;
						} else {
							return None;
						}
					}
					case TokenType::comma: {
						return None;
					}
					case TokenType::colon: {
						return None;
					}
					case TokenType::bracketClose:
						return None;
				}
			} else {
				return None;
			}
		} else if (tok.type == TokenType::comma) {
			if (!isNext(TokenType::string, i)) {
				return None;
			}
		} else {
			return None;
		}
	}
	return result;
}

Maybe<Json> JsonParser::parse(usize from, usize upto) const {
	auto result = Json();
	if (upto == -1) {
		upto = toks.size();
	}
	for (usize i = from + 1; i < upto; i++) {
		auto tok = toks.at(i);
		switch (tok.type) {
			case TokenType::False: {
				return None;
				break;
			}
			case TokenType::True: {
				return None;
				break;
			}
			case TokenType::curlyBraceOpen: {
				auto bClose = getPairEnd(false, i, upto);
				if (bClose.has_value()) {
					auto pairsVal = parsePairs(i, bClose.value());
					if (pairsVal.has_value()) {
						for (auto& pair : pairsVal.value()) {
							result[pair.first] = pair.second;
						}
						i = bClose.value();
					} else {
						return None;
					}
				} else {
					return None;
				}
				break;
			}
			case TokenType::curlyBraceClose: {
				break;
			}
			case TokenType::string:
			case TokenType::integer:
			case TokenType::floating:
			case TokenType::comma:
			case TokenType::colon:
			case TokenType::null:
			case TokenType::bracketOpen:
			case TokenType::bracketClose: {
				return None;
			}
		}
	}

	return result;
}

} // namespace qat