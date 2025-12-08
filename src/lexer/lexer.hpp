#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include "../utils/file_range.hpp"
#include "./token.hpp"
#include <filesystem>
#include <fstream>
#include <utility>

namespace qat::ir {
class Ctx;
}

namespace qat::lexer {

class Lexer {
  private:
	fs::path      filePath;
	std::ifstream file;
	char          prev;
	char          current;
	Vec<Token>*   tokens = nullptr;
	Deque<Token>  buffer;
	bool          repeatingToken = false;

	bool isMultiStringAllowed = false;

	Vec<TokenType> bracketOccurences;

	ir::Ctx* irCtx;

  public:
	explicit Lexer(ir::Ctx* _irCtx) : irCtx(_irCtx) {};
	useit static Lexer* get(ir::Ctx* irCtx);

	~Lexer();

	u64        lineNumber = 1;
	u64        byteNumber = 0;
	u64        charNumber = 0;
	Maybe<u64> previousLineEnd;
	u8         byteSpanUTF8 = 0;
	static u64 timeInMicroSeconds;
	static u64 lineCount;

	void clear_tokens();
	void throw_error(const String& message, Maybe<usize> offset = None);
	void analyse();
	void read();
	void change_file(fs::path newFile);

	useit static Maybe<Token> word_to_token(const String& value, Lexer* lexInst);
	useit Vec<Token>* get_tokens();
	useit Token       tokeniser();

	useit static bool is_char_hex(char byte) {
		return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z');
	}

	useit static bool is_invisible_ascii_char(char byte) {
		return (byte >= 0 && byte <= 13) || (byte == 27) || (byte == 127);
	}

	useit static bool ascii_char_has_standard_escape(char byte) {
		return byte == '\n' || byte == '\t' || byte == '\f' || byte == '\a' || byte == '\b' || byte == '\v' ||
		       byte == '\0' || byte == 'r';
	}

	useit static String get_ascii_standard_escape(char byte) {
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

	useit FileRangePtr get_position(u64 length);

	useit FileRange* get_position_var(u64 length);

	useit FileRangePtr create_range(FilePos start, FilePos end);
};

} // namespace qat::lexer

#endif
