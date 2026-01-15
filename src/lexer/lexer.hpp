#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include "../utils/file_range.hpp"
#include "./token.hpp"

#include <helpers/files.hpp>
#include <helpers/hashmap.hpp>
#include <helpers/hashset.hpp>
#include <helpers/integers.hpp>
#include <helpers/maybe.hpp>
#include <helpers/string.hpp>
#include <helpers/string_view.hpp>
#include <helpers/vec.hpp>

namespace qat::ir {
class Ctx;
}

namespace qat {
class QatSitter;
}

namespace qat::lexer {

class Lexer {
  private:
	String*    filePath;
	String     content;
	usize      cursor = 0;
	Vec<Token> tokens;
	bool       repeatingToken = false;

	bool isMultiStringAllowed = false;

	Vec<TokenType> bracketOccurences;

	ir::Ctx* irCtx;

	static const HashMap<StringView, TokenType> keywordMapping;

	static const HashSet<StringView> nativeTypeMapping;

	static const HashSet<StringView> floatTypeMapping;

	static const HashSet<StringView> intTypeMapping;

	static const HashSet<StringView> unsignedTypeMapping;

	static const HashMap<StringView, TokenType> valuedTokenMapping;

  public:
	explicit Lexer(ir::Ctx* _irCtx) : irCtx(_irCtx) {};

	static Lexer* get(ir::Ctx* irCtx) { return new Lexer(irCtx); }

	~Lexer() = default;

	u64        lineNumber = 1;
	u64        byteNumber = 0;
	u64        charNumber = 0;
	u64        previousLineEnd;
	u8         byteSpanUTF8 = 0;
	static u64 timeInNanoseconds;
	static u64 lineCount;

	void error(String const& message, Maybe<usize> offset = None);

	void analyse();

	Vec<Token>& get_tokens() { return tokens; }

	void read();

	void change_file(FilePath newFile);

	static Maybe<Token> word_to_token(const String& value, Lexer* lexInst);

	void tokeniser();

	inline void advance_cursor() { cursor++; }

	inline bool has_previous() const { return cursor > 0; }

	inline char previous() const { return content[cursor - 1]; }

	inline char get() const { return content[cursor]; }

	inline bool has_file_ended() { return cursor == content.size(); }

	static bool is_char_hex(char byte) {
		return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z');
	}

	static bool is_invisible_ascii_char(char byte) {
		return (byte >= 0 && byte <= 13) || (byte == 27) || (byte == 127);
	}

	static bool ascii_char_has_standard_escape(char byte) {
		return byte == '\n' || byte == '\t' || byte == '\f' || byte == '\a' || byte == '\b' || byte == '\v' ||
		       byte == '\0' || byte == '\r';
	}

	static String get_ascii_standard_escape(char byte) {
		switch (byte) {
			case '\n':
				return "\\n";
			case '\t':
				return "\\t";
			case '\f':
				return "\\f";
			case '\a':
				return "\\a";
			case '\b':
				return "\\b";
			case '\v':
				return "\\v";
			case '\0':
				return "\\0";
			case '\r':
				return "\\r";
		}
		std::unreachable();
	}

	FileRangePtr get_position(u64 length);

	FileRange* get_position_var(u64 length);

	FileRangePtr create_range(FilePos start, FilePos end);
};

} // namespace qat::lexer

#endif
