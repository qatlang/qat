#ifndef QAT_JSON_PARSER_HPP
#define QAT_JSON_PARSER_HPP

#include "helpers.hpp"
#include "macros.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace qat {

class Json;
class JsonValue;

class JsonParser {
	friend class Json;

  private:
	enum class TokenType {
		False,
		True,
		curlyBraceOpen,
		curlyBraceClose,
		string,
		integer,
		floating,
		comma,
		colon,
		null,
		bracketOpen,
		bracketClose,
	};

	class Token {
	  public:
		Token(TokenType _type) : type(_type) {}
		Token(TokenType _type, String _val) : type(_type), value(std::move(_val)) {}

		TokenType	type;
		std::string value;
	};

	Vec<Token> toks;

	useit bool lex(std::string val);
	useit bool isNext(TokenType type, usize current) const;
	useit Maybe<bool> hasPrimaryCommas(usize from, usize upto) const;
	useit Maybe<Json> parse(usize from = -1, usize upto = -1) const;
	useit Maybe<JsonValue> parseValue(usize from, usize upto) const;
	useit Maybe<Vec<Pair<String, JsonValue>>> parsePairs(usize from, usize upto) const;
	useit Maybe<usize> getPairEnd(bool isList, usize from, std::optional<usize> upto) const;
	useit Maybe<Vec<usize>> getPrimaryCommas(usize from, usize upto) const;
};

} // namespace qat

#endif