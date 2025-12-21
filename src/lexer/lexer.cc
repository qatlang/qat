#include "./lexer.hpp"
#include "../IR/context.hpp"
#include "../show.hpp"
#include "../utils/end_fn.hpp"
#include "../utils/profiler.hpp"
#include "../utils/utils.hpp"
#include "./token_type.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>

#define NanosecondsInMicroseconds 1000
#define NanosecondsInMilliseconds 1000000
#define NanosecondsInSeconds      1000000000

#define Check_Normal_Keyword(ident, tokenName)                                                                         \
	if (wordValue == ident)                                                                                            \
	return Token::normal(TokenType::tokenName, getPos(std::string::traits_type::length(ident)))

#define Check_VALUED_Keyword(ident, tokenName)                                                                         \
	if (wordValue == ident)                                                                                            \
	return Token::valued(TokenType::tokenName, ident, getPos(std::string::traits_type::length(ident)))

namespace qat::lexer {

u64 Lexer::timeInNanoseconds = 0;
u64 Lexer::lineCount         = 0;

void Lexer::read() {
	//   try {
	if (has_file_ended()) {
		return;
	}
	advance_cursor();
	if (has_file_ended()) {
		return;
	}
	byteNumber++;
	if (byteSpanUTF8 == 0) {
		auto len = utils::get_utf8_byte_length(get());
		if (not len.has_value()) {
			throw_error(
			    "Invalid UTF-8 encoding. Could not determine the number of encoded bytes for this character, that starts with the byte " +
			    utils::to_hex_with_prefix(get(), 2));
		}
		byteSpanUTF8 += len.value() - 1;
	} else {
		byteSpanUTF8--;
		if (byteSpanUTF8 == 0) {
			charNumber++;
		}
	}
	if (has_file_ended()) {
		return;
	}
	if (get() == '\n') {
		previousLineEnd = byteNumber - 1;
		lineNumber++;
		byteNumber = 0;
	} else if (get() == '\r') {
		advance_cursor();
		if (has_file_ended()) {
			return;
		}
		if (get() == '\n') {
			previousLineEnd = byteNumber - 2;
			lineNumber++;
			byteNumber = 0;
		} else {
			lineNumber++;
			byteNumber = 1; // CR - legacy macOS line ending
		}
	}
	//   } catch (std::exception& err) {
	//     throwError(String("Lexer failed while reading the file. Error: ") + err.what());
	//   }
}

FileRangePtr Lexer::get_position(u64 length) {
	FilePos end = {lineNumber, byteNumber > 0 ? (byteNumber - 1) : byteNumber};
	if (byteNumber == 0) {
		end = {lineNumber - 1, previousLineEnd};
	}
	return FileRange::from(fs::path(filePath), {end.line, end.byteOffset - length}, end);
}

FileRange* Lexer::get_position_var(u64 length) {
	FilePos end = {lineNumber, byteNumber > 0 ? (byteNumber - 1) : byteNumber};
	if (byteNumber == 0) {
		end = {lineNumber - 1, previousLineEnd};
	}
	return FileRange::var_from(fs::path(filePath), {end.line, end.byteOffset - length}, end);
}

void Lexer::analyse() {
	auto fileSize = std::filesystem::file_size(filePath);
	content.resize(fileSize);
	std::ifstream inStream(filePath);
	inStream.read(&content[0], fileSize);

	auto startTime = std::chrono::high_resolution_clock::now();
	tokens.push_back(Token::valued(TokenType::startOfFile, filePath.string(), this->get_position(0)));
	read();
	while (not has_file_ended()) {
		tokens.push_back(tokeniser());
	}
	if (tokens.back().type != TokenType::endOfFile) {
		tokens.push_back(Token::valued(TokenType::endOfFile, filePath.string(), this->get_position(0)));
	}
	timeInNanoseconds +=
	    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - startTime)
	        .count();
	lineCount += lineNumber;
}

void Lexer::change_file(fs::path newFilePath) {
	tokens.clear();
	filePath   = std::move(newFilePath);
	cursor     = -1;
	lineNumber = 1;
	byteNumber = 0;
}

#define LOWER_LETTER_FIRST 'a'
#define LOWER_LETTER_LAST  'z'
#define UPPER_LETTER_FIRST 'A'
#define UPPER_LETTER_LAST  'Z'
#define DIGIT_FIRST        '0'
#define DIGIT_LAST         '9'
#define CURRENT_IS_ALPHABET                                                                                            \
	((get() >= LOWER_LETTER_FIRST && get() <= LOWER_LETTER_LAST) ||                                                    \
	 (get() >= UPPER_LETTER_FIRST && get() <= UPPER_LETTER_LAST))

#define CURRENT_IS_DIGIT (get() >= DIGIT_FIRST && get() <= DIGIT_LAST)

Token Lexer::tokeniser() {
	auto endFn = EndFunction([&]() {
		if (repeatingToken) {
			repeatingToken = false;
		}
	});
	if (not buffer.empty()) {
		Token token = buffer.back();
		buffer.pop_back();
		return token;
	}
	if (has_file_ended()) {
		return Token::valued(TokenType::endOfFile, filePath.string(), this->get_position(0));
	}
	if (has_previous() && (previous() == '\r')) {
		read();
		return tokeniser();
	}
	switch (get()) {
		case ' ':
		case '\n':
		case '\r':
		case '\t': {
			read();
			return tokeniser();
		}
		case '.': {
			read();
			if (get() == '.') {
				read();
				if (get() == '.') {
					read();
					return Token::normal(TokenType::ellipsis, this->get_position(3));
				} else {
					throw_error("Expected either . or ... here, but found an invalid token instead", 2);
				}
			} else {
				return Token::normal(TokenType::stop, this->get_position(1));
			}
		}
		case ',': {
			read();
			return Token::normal(TokenType::separator, this->get_position(1));
		}
		case '(': {
			read();
			return Token::normal(TokenType::parenthesisOpen, this->get_position(1));
		}
		case ')': {
			read();
			return Token::normal(TokenType::parenthesisClose, this->get_position(1));
		}
		case '[': {
			read();
			bracketOccurences.push_back(TokenType::bracketOpen);
			return Token::normal(TokenType::bracketOpen, this->get_position(1));
		}
		case ']': {
			read();
			if ((not bracketOccurences.empty()) && (bracketOccurences.back() == TokenType::genericTypeStart)) {
				bracketOccurences.pop_back();
				return Token::normal(TokenType::genericTypeEnd, this->get_position(1));
			} else {
				bracketOccurences.pop_back();
				return Token::normal(TokenType::bracketClose, this->get_position(1));
			}
		}
		case '{': {
			read();
			return Token::normal(TokenType::curlybraceOpen, this->get_position(1));
		}
		case '}': {
			read();
			return Token::normal(TokenType::curlybraceClose, this->get_position(1));
		}
		case '^': {
			read();
			if (get() == '=') {
				read();
				return Token::valued(TokenType::assignedBinaryOperator, "^=", this->get_position(2));
			}
			return Token::valued(TokenType::binaryOperator, "^", this->get_position(1));
		}
		case ':': {
			read();
			if (get() == '[') {
				read();
				bracketOccurences.push_back(TokenType::genericTypeStart);
				return Token::normal(TokenType::genericTypeStart, this->get_position(2));
			} else if (get() == '=') {
				read();
				return Token::normal(TokenType::associatedAssignment, this->get_position(2));
			} else if (get() == ':') {
				read();
				return Token::normal(TokenType::typeSeparator, this->get_position(2));
			} else {
				return Token::normal(TokenType::colon, this->get_position(1));
			}
		}
		case '/': {
			String value = "/";
			read();
			if (get() == '*') {
				bool   star = false;
				String commentValue;
				read();
				auto commentPos = this->get_position_var(0);
				while ((not star || (get() != '/')) && not has_file_ended()) {
					if (star) {
						star = false;
					}
					if (get() == '*') {
						star = true;
					}
					read();
					if (not star || (get() != '/')) {
						commentValue += get();
						if (get() == '\n') {
							commentPos->end.line++;
							commentPos->end.byteOffset = 0;
						} else {
							commentPos->end.byteOffset++;
						}
					}
				}
				commentPos->end.byteOffset--;
				read();
				return Token::valued(TokenType::comment, commentValue, commentPos);
			} else if (get() == '/') {
				String commentValue;
				auto   commRange = this->get_position(2);
				while ((get() != '\n' && (has_previous() ? (previous() != '\r') : true)) && not has_file_ended()) {
					commentValue += get();
					read();
					commRange = this->get_position(commentValue.length());
				}
				return Token::valued(TokenType::comment, commentValue, commRange);
			} else {
				return Token::valued(TokenType::binaryOperator, value, this->get_position(1));
			}
		}
		case '!': {
			read();
			if (get() == '=') {
				read();
				return Token::valued(TokenType::binaryOperator, "!=", this->get_position(2));
			} else {
				return Token::valued(TokenType::exclamation, "!", this->get_position(1));
			}
		}
		case '~': {
			read();
			if (get() == '=') {
				read();
				return Token::valued(TokenType::assignedBinaryOperator, "~=", this->get_position(2));
			} else {
				return Token::valued(TokenType::unaryOperator, "~", this->get_position(1));
			}
		}
		case '&': {
			read();
			if (get() == '=') {
				read();
				return Token::valued(TokenType::assignedBinaryOperator, "&=", this->get_position(2));
			} else if (get() == '&') {
				read();
				if (get() == '=') {
					read();
					return Token::valued(TokenType::assignedBinaryOperator, "&&=", this->get_position(3));
				} else {
					return Token::valued(TokenType::binaryOperator, "&&", this->get_position(2));
				}
			} else {
				return Token::valued(TokenType::binaryOperator, "&", this->get_position(1));
			}
		}
		case '|': {
			read();
			if (get() == '=') {
				read();
				return Token::valued(TokenType::assignedBinaryOperator, "|=", this->get_position(2));
			} else if (get() == '|') {
				read();
				if (get() == '=') {
					read();
					return Token::valued(TokenType::assignedBinaryOperator, "||=", this->get_position(3));
				} else {
					return Token::valued(TokenType::binaryOperator, "||", this->get_position(2));
				}
			} else {
				return Token::valued(TokenType::binaryOperator, "|", this->get_position(1));
			}
		}
		case '?': {
			read();
			return Token::normal(TokenType::questionMark, this->get_position(1));
		}
		case '+':
		case '-':
		case '%':
		case '*':
		case '<':
		case '>': {
			String operatorValue;
			operatorValue += get();
			read();
			if (get() == '=' && operatorValue != "<" && operatorValue != ">") {
				operatorValue += get();
				read();
				return Token::valued(TokenType::assignedBinaryOperator, operatorValue, this->get_position(2));
			} else if (get() == '=' && (operatorValue == "<" || operatorValue == ">")) {
				operatorValue += get();
				read();
				return Token::valued(TokenType::binaryOperator, operatorValue, this->get_position(2));
			} else if ((get() == '<' && operatorValue == "<") || (get() == '>' && operatorValue == ">")) {
				operatorValue += get();
				read();
				return Token::valued(TokenType::binaryOperator, operatorValue, this->get_position(2));
			} else if (get() == '>' && operatorValue == "-") {
				read();
				return Token::normal(TokenType::givenTypeSeparator, this->get_position(2));
			} else if (operatorValue == "<") {
				return Token::valued(TokenType::binaryOperator, "<", this->get_position(1));
			} else if (operatorValue == ">") {
				return Token::valued(TokenType::binaryOperator, ">", this->get_position(1));
			}
			return Token::valued(TokenType::binaryOperator, operatorValue, this->get_position(1));
		}
		case '=': {
			read();
			if (get() == '=') {
				read();
				return Token::valued(TokenType::binaryOperator, "==", this->get_position(2));
			} else if (get() == '>') {
				read();
				return Token::normal(TokenType::fatArrow, this->get_position(2));
			} else {
				return Token::normal(TokenType::assignment, this->get_position(1));
			}
		}
		case '\'': {
			read();
			if (get() == '\'') {
				read();
				return Token::normal(TokenType::selfInstance, this->get_position(2));
			} else {
				return Token::normal(TokenType::child, this->get_position(1));
			}
		}
		case '`': {
			auto              start = byteNumber;
			std::array<u8, 4> bytes = {0, 0, 0, 0};
			read();
			if (get() == '\\') {
				read();
				if (get() == '0') {
					bytes[0] = '\0';
				} else if (get() == '\\') {
					bytes[0] = '\\';
				} else if (get() == 'n') {
					bytes[0] = '\n';
				} else if (get() == 'b') {
					bytes[0] = '\b';
				} else if (get() == 't') {
					bytes[0] = '\t';
				} else if (get() == 'a') {
					bytes[0] = '\a';
				} else if (get() == 'r') {
					bytes[0] = '\r';
				} else if (get() == '`') {
					bytes[0] = '`';
				} else if (get() == 'f') {
					bytes[0] = '\f';
				} else if (get() == 'v') {
					bytes[0] = '\v';
				} else if (get() == 'u') {
					read();
					if (get() != '{') {
						throw_error("Expected { and } to enclose the unicode scalar value after this");
					}
					read();
					String hex;
					while ((not has_file_ended()) &&
					       ((get() >= '0' && get() <= '9') || (get() >= 'a' && get() <= 'f') ||
					        (get() >= 'A' && get() <= 'F'))) {
						hex += get();
						read();
					}
					if (hex.empty()) {
						throw_error("Expected hex digits after \\u{");
					}
					if (hex.length() > 6) {
						throw_error("Maximum hex digits allowed in Unicode scalar value is 6. Found " + hex +
						            " instead");
					}
					if (get() != '}') {
						throw_error("Expected } after the unicode scalar value");
					}
					auto scalar  = static_cast<u32>(std::stoul(hex, nullptr, 16));
					auto utf8Res = utils::unicode_scalar_to_utf8(scalar);
					if (utf8Res.has_value()) {
						bytes = utf8Res->first;
					} else {
						throw_error(
						    "The value obtained from the unicode escape sequence is " +
						    utils::to_hex_with_prefix(scalar, None) +
						    " which is not a valid unicode scalar value. Unicode scalar values are in the range 0x0000 to 0x10FFFF");
					}
				} else {
					throw_error("Invalid escape sequence found. The escape sequences allowed are: \n" +
					            String("\\0, \\\\\\\\, \\`, \\n, \\b, \\t, \\r, \\a, \\f, \\v and \\u{XXXX}") +
					            "\nThe byte representation of the character following \\ is " +
					            utils::to_hex_with_prefix(get(), 2));
				}
			} else {
				auto byteLen = utils::get_utf8_byte_length(get());
				if (not byteLen.has_value()) {
					throw_error(
					    "Invalid UTF-8 encoded character. The first byte read for the Uniform scalar value is " +
					    utils::to_hex_with_prefix(get(), 2));
				}
				switch (byteLen.value()) {
					case 1: {
						if (is_invisible_ascii_char(get())) {
							const auto hexStr = (std::stringstream() << std::hex << std::setw(4) << std::setfill('0')
							                                         << std::uppercase << get())
							                        .str();
							if (ascii_char_has_standard_escape(get())) {
								throw_error(
								    "Found invisible ASCII character here that can be encoded as a standard escape sequence. Please change this to `" +
								    get_ascii_standard_escape(get()) + "`");
							} else {
								throw_error(
								    "Found invisible character U+" + hexStr +
								    " in the unicode scalar value. Such characters should be provided as escape sequences. Please change this to `\\u{" +
								    hexStr + "}`");
							}
						}
						bytes[0] = get();
						break;
					}
					case 2: {
						bytes[0] = get();
						read();
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the second byte in a 2-byte encoded character. This byte does not follow the UTF-8 encoding"
							    " constraints. The byte sequence that have been read as part of a single unicode scalar value is " +
							    utils::to_hex(previous(), 2) + ' ' + utils::to_hex(get(), 2));
						}
						bytes[1] = get();
						break;
					}
					case 3: {
						bytes[0] = get();
						read();
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the second byte in a 3-byte encoded character. This byte does not follow the UTF-8 encoding"
							    " constraints. The byte sequence that have been read as part of a single unicode scalar value is " +
							    utils::to_hex(bytes[0], 2) + ' ' + utils::to_hex(get(), 2));
						}
						bytes[1] = get();
						read();
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the third byte in a 3-byte encoded character. This byte does not follow the UTF-8 encoding"
							    " constraints. The byte sequence that have been read as part of a single unicode scalar value is " +
							    utils::to_hex(bytes[0], 2) + ' ' + utils::to_hex(bytes[1], 2) + ' ' +
							    utils::to_hex(get(), 2));
						}
						bytes[2] = get();
						break;
					}
					case 4: {
						bytes[0] = get();
						read();
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the second byte in a 3-byte encoded character. This byte does not follow the UTF-8 encoding"
							    " constraints. The byte sequence that have been read as part of a single unicode scalar value is " +
							    utils::to_hex(bytes[0], 2) + ' ' + utils::to_hex(get(), 2));
						}
						bytes[1] = get();
						read();
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the third byte in a 3-byte encoded character. This byte does not follow the UTF-8 encoding"
							    " constraints. The byte sequence that have been read as part of a single unicode scalar value is " +
							    utils::to_hex(bytes[0], 2) + ' ' + utils::to_hex(bytes[1], 2) + ' ' +
							    utils::to_hex(get(), 2));
						}
						bytes[2] = get();
						read();
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the fourth byte in a 3-byte encoded character. This byte does not follow the UTF-8 encoding"
							    " constraints. The byte sequence that have been read as part of a single unicode scalar value is " +
							    utils::to_hex(bytes[0], 2) + ' ' + utils::to_hex(bytes[1], 2) + ' ' +
							    utils::to_hex(bytes[2], 2) + ' ' + utils::to_hex(get(), 2));
						}
						bytes[3] = get();
						break;
					}
				}
				if (byteLen.value() > 1) {
					auto scalar = utils::utf8_to_unicode_scalar(bytes);
					if (not scalar.has_value()) {
						throw_error(
						    "The provided UTF-8 encoding could not be converted to a valid Unicode scalar value. Unicode scalar values"
						    " should be in the inclusive range from 0x0000 to 0x10FFFF. The byte representation of the bytes read is " +
						    utils::to_hex(bytes[0], 2) + ' ' +
						    (byteLen.value() >= 2 ? (utils::to_hex(bytes[1], 2) + ' ') : "") +
						    (byteLen.value() >= 3 ? (utils::to_hex(bytes[2], 2) + ' ') : "") +
						    (byteLen.value() == 4 ? (utils::to_hex(bytes[3], 2) + ' ') : ""));
					} else if (utils::is_invisible_unicode(scalar.value())) {
						throw_error(
						    "Found an invisible character here. If this was intentional, please change this to \\u{" +
						    utils::to_hex(scalar.value(), None) +
						    "} instead. Such characters should be encoded as unicode scalar values");
					}
				}
			}
			read();
			if (get() != '`') {
				throw_error("Expected ` to end the unicode scalar value");
			}
			String tokRes(4, 0);
			tokRes[0] = bytes[0];
			tokRes[1] = bytes[1];
			tokRes[2] = bytes[2];
			tokRes[3] = bytes[3];
			read();
			return Token::valued(TokenType::characterLiteral, std::move(tokRes),
			                     this->get_position(byteNumber - start));
		}
		case ';': {
			read();
			return Token::normal(TokenType::semiColon, this->get_position(1));
		}
		case '"': {
			bool escape = false;
			read();
			String str_val;
			while (escape ? not has_file_ended() : (get() != '"' && not has_file_ended())) {
				if (escape) {
					escape = false;
					if (get() == '"') {
						str_val += '"';
					} else if (get() == '\\') {
						str_val += '\\';
					} else if (get() == 'n') {
						str_val += '\n';
					} else if (get() == 'b') {
						str_val += '\b';
					} else if (get() == 'a') {
						str_val += '\a';
					} else if (get() == 'f') {
						str_val += '\f';
					} else if (get() == 'r') {
						str_val += '\r';
					} else if (get() == 't') {
						str_val += '\t';
					} else if (get() == 'v') {
						str_val += '\v';
					} else if (get() == 'x') {
						read();
						if (get() != '{') {
							throw_error("Expected { and } to enclose the ASCII byte, which is to be provided");
						}
						read();
						String hex;
						while (is_char_hex(get())) {
							hex += get();
							read();
						}
						if (hex.empty()) {
							throw_error("Could not find any hex digits after \\x{");
						} else if (hex.length() > 2) {
							throw_error(
							    "Escape sequence to provide the ASCII byte can only contain atmost 2 hex digits. Found " +
							    hex + " instead");
						}
						if (get() != '}') {
							throw_error("Expected } to end the ASCII byte");
						}
						auto charVal = (unsigned char)std::stoul(hex);
						if (charVal >= 0x80) {
							throw_error(
							    "The byte " + hex +
							    " is not in the ASCII range. ASCII characters should be in the inclusive range from 0x00 to 0x7F");
						}
						str_val += charVal;
					} else if (get() == 'u') {
						read();
						if (get() != '{') {
							throw_error(
							    "Expected { and } to enclose the Unicode scalar value, which is to be provided");
						}
						read();
						String hex;
						while (is_char_hex(get())) {
							hex += get();
							read();
						}
						if (hex.empty()) {
							throw_error("Expected hex digits to be provided after \\u{");
						}
						if (hex.length() > 6) {
							throw_error("The maximum number of hex digits allowed in Unicode scalar value is 6");
						}
						if (get() != '}') {
							throw_error("Expected } to end the Unicode scalar value");
						}
						auto scalar  = static_cast<u32>(std::stoul(hex, nullptr, 16));
						auto utf8Res = utils::unicode_scalar_to_utf8(scalar);
						if (utf8Res.has_value()) {
							for (u8 i = 0; i < utf8Res->second; i++) {
								str_val += utf8Res->first[i];
							}
						} else {
							throw_error("The value obtained from the unicode escape sequence is " +
							            utils::to_hex_with_prefix(scalar, None) +
							            " which is not a valid unicode scalar value. Unicode scalar values "
							            "should be in the inclusive range from 0x0000 to 0x10FFFF");
						}
					} else {
						throw_error("Invalid escape sequence found. The escape sequences allowed are: \n" +
						            String("\\0, \\\\\\\\, \\\", \\n, \\b, \\t, \\r, \\a, \\f, \\v and \\u{XXXX}") +
						            "\nThe byte representation of the character following \\ is " +
						            utils::to_hex_with_prefix(get(), 2));
					}
				} else {
					if (get() == '\\' && (has_previous() ? (previous() != '\\') : true)) {
						escape = true;
					} else {
						auto byteLen = utils::get_utf8_byte_length(get());
						if (not byteLen.has_value()) {
							throw_error("Invalid UTF-8 encoding found here. The byte which was supposed to be "
							            "first in the sequence for a character is " +
							            utils::to_hex_with_prefix(get(), 2));
						}
						std::array<u8, 4> bytes{0, 0, 0, 0};
						switch (byteLen.value()) {
							case 1: {
								if (is_invisible_ascii_char(get())) {
									if (not(isMultiStringAllowed && (get() == '\n' || get() == '\t'))) {
										if (ascii_char_has_standard_escape(get())) {
											throw_error(
											    "Found an invisible character here that can be encoded as a standard escape sequence. "
											    "Please change this to " +
											    get_ascii_standard_escape(get()) +
											    ((get() == '\n')
											         ? ". If you want to use multiline strings, use the syntax multi\"String content\""
											         : ""));
										} else {
											throw_error(
											    "Found an invisible character here. If this was intentional, please change this to \\u{" +
											    utils::to_hex(get(), 4) +
											    "}. Such characters should be encoded as Unicode scalar values");
										}
									}
								}
								str_val += get();
								break;
							}
							case 2: {
								bytes[0] = get();
								str_val += get();
								read();
								bytes[1] = get();
								if (not utils::is_follow_byte_utf8(get())) {
									throw_error(
									    "Found " + utils::to_hex_with_prefix(get(), 2) +
									    " as the second byte in a 2-byte encoded character in this UTF-8 text. This byte does not follow the"
									    " UTF-8 encoding constraints. The byte sequence that have been read as a single unicode character is " +
									    utils::to_hex(previous(), 2) + ' ' + utils::to_hex(get(), 2));
								}
								str_val += get();
								break;
							}
							case 3: {
								bytes[0] = get();
								str_val += get();
								read();
								bytes[1] = get();
								if (not utils::is_follow_byte_utf8(get())) {
									throw_error(
									    "Found " + utils::to_hex_with_prefix(get(), 2) +
									    " as the second byte in a 3-byte encoded character in this UTF-8 text. This byte does not follow the"
									    " UTF-8 encoding constraints. The byte sequence that have been read so far for this character is " +
									    utils::to_hex(previous(), 2) + ' ' + utils::to_hex(get(), 2));
								}
								str_val += get();
								read();
								bytes[2] = get();
								if (not utils::is_follow_byte_utf8(get())) {
									throw_error(
									    "Found " + utils::to_hex_with_prefix(get(), 2) +
									    " as the third byte in a 3-byte encoded character in this UTF-8 text. This byte does not follow the"
									    " UTF-8 encoding constraints. The byte sequence that have been read so far for this character is " +
									    utils::to_hex(str_val[str_val.length() - 2], 2) + ' ' +
									    utils::to_hex(str_val[str_val.length() - 1], 2) + ' ' +
									    utils::to_hex(get(), 2));
								}
								str_val += get();

								break;
							}
							case 4: {
								bytes[0] = get();
								str_val += get();
								read();
								bytes[1] = get();
								if (not utils::is_follow_byte_utf8(get())) {
									throw_error(
									    "Found " + utils::to_hex_with_prefix(get(), 2) +
									    " as the second byte in a 4-byte encoded character in this UTF-8 text. This byte does not follow the"
									    " UTF-8 encoding constraints. The byte sequence that have been read so far for this character is " +
									    utils::to_hex(previous(), 2) + ' ' + utils::to_hex(get(), 2));
								}
								str_val += get();
								read();
								bytes[2] = get();
								if (not utils::is_follow_byte_utf8(get())) {
									throw_error(
									    "Found " + utils::to_hex_with_prefix(get(), 2) +
									    " as the third byte in a 4-byte encoded character in this UTF-8 text. This byte does not follow the"
									    " UTF-8 encoding constraints. The byte sequence that have been read so far for this character is " +
									    utils::to_hex(str_val[str_val.length() - 2], 2) + ' ' +
									    utils::to_hex(str_val[str_val.length() - 1], 2) + ' ' +
									    utils::to_hex(get(), 2));
								}
								str_val += get();
								read();
								bytes[3] = get();
								if (not utils::is_follow_byte_utf8(get())) {
									throw_error(
									    "Found " + utils::to_hex_with_prefix(get(), 2) +
									    " as the fourth byte in a 4-byte encoded character in this UTF-8 text. This byte does not follow the"
									    " UTF-8 encoding constraints. The byte sequence that have been read so far for this character is " +
									    utils::to_hex(str_val[str_val.length() - 3], 2) + ' ' +
									    utils::to_hex(str_val[str_val.length() - 2], 2) + ' ' +
									    utils::to_hex(str_val[str_val.length() - 1], 2) + ' ' +
									    utils::to_hex(get(), 2));
								}
								str_val += get();
								break;
							}
						}
						if (byteLen.value() > 1) {
							auto scalar = utils::utf8_to_unicode_scalar(bytes);
							if (scalar.has_value() && utils::is_invisible_unicode(scalar.value())) {
								throw_error(
								    "Found an invisible character here. If this was intentional, please change this to \\u{" +
								    utils::to_hex(scalar.value(), None) +
								    "} instead. Such characters should be encoded as unicode scalar values");
							}
						}
					}
				}
				read();
			}
			if (get() != '"') {
				throw_error("Could not find \" at the end of the string literal");
			}
			read();
			isMultiStringAllowed = false;
			return Token::valued(TokenType::StringLiteral, str_val, this->get_position(str_val.length() + 2));
		}
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			String numVal;
			bool   is_float         = false;
			bool   exponentialFloat = false;
			bool   foundRadix       = false;
			if (get() == '0') {
				read();
				if (get() == 'b') {
					read();
					numVal     = "0b";
					foundRadix = true;
				} else if (get() == 'c') {
					read();
					numVal     = "0c";
					foundRadix = true;
				} else if (get() == 'x') {
					read();
					numVal     = "0x";
					foundRadix = true;
				} else if (get() == 'r') {
					numVal += "0r";
					read();
					while (CURRENT_IS_DIGIT) {
						numVal += get();
						read();
					}
					if (get() == '_') {
						numVal += '_';
						read();
					} else {
						throw_error("Invalid custom radix integer literal. Expected _ after " + numVal);
					}
					foundRadix = true;
				} else {
					numVal += "0";
				}
			}
			bool foundSpec = false;
			while ((CURRENT_IS_DIGIT || (foundRadix && not foundSpec && CURRENT_IS_ALPHABET) ||
			        (not is_float && (get() == '.')) || (not foundRadix && not exponentialFloat && (get() == 'e')) ||
			        (not foundSpec && (get() == '_'))) &&
			       not has_file_ended()) {
				if (not foundRadix && not exponentialFloat && get() == 'e') {
					is_float         = true;
					exponentialFloat = true;
					String expStr("e");
					read();
					if (get() == '-') {
						expStr += get();
						read();
					}
					while (CURRENT_IS_DIGIT) {
						expStr += get();
						read();
					}
					numVal += expStr;
					continue;
				} else if (get() == '.') {
					read();
					if (CURRENT_IS_DIGIT) {
						if (foundRadix) {
							throw_error("This literal is in custom radix format and hence cannot contain decimal point",
							            numVal.length() + 1);
						}
						is_float = true;
						numVal += '.';
					} else {
						/// This is in the reverse order since the last element is returned
						/// first
						buffer.push_back(Token::normal(TokenType::stop, this->get_position(1)));
						auto fileRange = this->get_position_var(numVal.length() + 1);
						if (fileRange->end.byteOffset > 0) {
							fileRange->end.byteOffset--;
						}
						buffer.push_back(Token::valued(is_float ? TokenType::floatLiteral : TokenType::integerLiteral,
						                               numVal, fileRange));
						return tokeniser();
					}
				} else if (get() == '_') {
					String specString;
					read();
					if (CURRENT_IS_DIGIT) {
						numVal += "_";
					} else if (CURRENT_IS_ALPHABET) {
						foundSpec  = true;
						specString = "_";
						while (CURRENT_IS_ALPHABET || CURRENT_IS_DIGIT) {
							specString += get();
							read();
						}
						numVal += specString;
						if (specString == "_f32" || specString == "_f64" || specString == "_f128" ||
						    specString == "_f128ppc" || specString == "_f16" || specString == "_fbrain" ||
						    specString == "_float" || specString == "_double" || specString == "_longdouble") {
							is_float = true;
						}
						return Token::valued(is_float ? TokenType::floatLiteral : TokenType::integerLiteral, numVal,
						                     this->get_position(numVal.length()));
					} else {
						throw_error("Invalid literal. Found _ without anything following");
					}
				}
				numVal += get();
				read();
			}
			return Token::valued(is_float ? TokenType::floatLiteral : TokenType::integerLiteral, numVal,
			                     this->get_position(numVal.length()));
		}
		default: {
			if (has_file_ended()) {
				return Token::valued(TokenType::endOfFile, filePath.string(), this->get_position(0));
			}
			auto   start = byteNumber;
			String idVal;
			auto   idRange = FileRange::null;
			if (get() == 'b') {
				idVal += get();
				read();
				if (not has_file_ended() && (get() == '`')) {
					// BYTE LITERAL
					String byteValue;
					read();
					if (get() == '`') {
						throw_error("Byte literals cannot be empty, please provide an Extended ASCII character in it. "
						            "Please use \\` if you literally meant to include the character `");
					}
					if (is_invisible_ascii_char(get())) {
						if (ascii_char_has_standard_escape(get())) {
							throw_error(
							    "Found an invisible character in the byte literal which could be encoded as an escape sequence. Please change this to b`" +
							    String(get_ascii_standard_escape(get())) + "`");
						} else {
							throw_error(
							    "Found an invisible character in the byte literal. Such characters should be encoded as escape sequences. Please change this to b`\\x{" +
							    (std::stringstream()
							     << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << get())
							        .str() +
							    "}`");
						}
					} else if (get() == '\\') {
						read();
						if (get() == '0') {
							byteValue += '\0';
						} else if (get() == 'n') {
							byteValue += '\n';
						} else if (get() == 'b') {
							byteValue += '\b';
						} else if (get() == 't') {
							byteValue += '\t';
						} else if (get() == 'f') {
							byteValue += '\f';
						} else if (get() == 'r') {
							byteValue += '\r';
						} else if (get() == 'v') {
							byteValue += '\v';
						} else if (get() == 'a') {
							byteValue += '\a';
						} else if (get() == 'x') {
							read();
							if (get() != '{') {
								throw_error("Expected { and } to enclose the Extended ASCII byte after this");
							}
							String hex;
							read();
							while ((not has_file_ended()) &&
							       ((get() >= '0' && get() <= '9') || (get() >= 'a' && get() <= 'z') ||
							        (get() >= 'A' && get() <= 'Z'))) {
								hex += get();
								read();
							}
							if (hex.empty()) {
								throw_error("Could not find any hex digits after \\x{");
							} else if (hex.length() > 2) {
								throw_error(
								    "Escape sequence to provide the Extented ASCII byte can only contain atmost 2 hex digits. Found " +
								    hex + " instead");
							}
							if (get() != '}') {
								throw_error("Expected } to end the Extented ASCII byte after this");
							}
							byteValue += (char)std::stoul(hex, nullptr, 16);
						} else {
							throw_error(
							    "Invalid escape sequence. The byte representation of the character following the \\ is " +
							    utils::to_hex_with_prefix(get(), 2));
						}
					} else {
						byteValue += get();
					}
					read();
					if (get() != '`') {
						throw_error(
						    "Expected ` after the byte value, which could not be found. Byte literals can only contain one Extended ASCII character.");
					}
					read();
					return Token::valued(TokenType::byteLiteral, byteValue, this->get_position(byteNumber - start + 1));
				} else if (has_file_ended()) {
					return Token::valued(TokenType::identifier, "b", this->get_position(1));
				}
			}
			bool skipPreRead = false;
			bool breakLoop   = false;
			while (not has_file_ended()) {
				if (idVal.length() != 0) {
					idRange = this->get_position(idVal.length());
				}
				auto byteLen = utils::get_utf8_byte_length(get());
				if (not byteLen.has_value()) {
					throw_error(
					    "Invalid UTF-8 encoding. Could not calculate the number of bytes required for the current character from its first byte. The first byte is " +
					    utils::to_hex_with_prefix(get(), 2));
				}
				std::array<u8, 4> bytes{0, 0, 0, 0};
				switch (byteLen.value()) {
					case 1: {
						auto tempRange = idVal.empty() ? idRange : this->get_position(idVal.length());
						if (not(idVal.empty() ? (CURRENT_IS_ALPHABET || (get() == '_'))
						                      : (CURRENT_IS_ALPHABET || CURRENT_IS_DIGIT || (get() == '_')))) {
							if (repeatingToken || idVal.empty()) {
								// std::cout << "Repeating token: " << repeatingToken
								//           << " Empty identifier: " << idVal.empty() << "\n";
								throw_error(
								    String("The character ") + get() +
								    " cannot be part of an identifier and is also not a recognised symbol in the language. "
								    "Identifiers should start with _ or letters from any unicode supported language, and can "
								    "contain digits from any unicode supported language as well in the middle");
							} else {
								repeatingToken = true;
								skipPreRead    = true;
								breakLoop      = true;
								idRange        = std::move(tempRange);
								break;
							}
						}
						bytes[0] = get();
						break;
					}
					case 2: {
						bytes[0] = get();
						read();
						if (has_file_ended()) {
							throw_error(
							    "File ended before reading the second byte of the 2-byte encoded UTF-8 character. The first byte read was " +
							    utils::to_hex_with_prefix(bytes[0], 2));
						}
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the second byte of the 2-byte encoded UTF-8 character. This byte does not follow the UTF-8 encoding constraints");
						}
						bytes[1] = get();
						break;
					}
					case 3: {
						bytes[0] = get();
						if (has_file_ended()) {
							throw_error(
							    "File ended before reading the second byte of the 3-byte encoded UTF-8 character. The first byte read was " +
							    utils::to_hex_with_prefix(bytes[0], 2));
						}
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the second byte of the 3-byte encoded UTF-8 character. This byte does not follow UTF-8 encoding constraints");
						}
						bytes[1] = get();
						if (has_file_ended()) {
							throw_error(
							    "File ended before reading the third byte of the 3-byte encoded UTF-8 character. The bytes read so far is " +
							    utils::to_hex_with_prefix(bytes[0], 2) + ' ' + utils::to_hex(bytes[1], 2));
						}
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the third byte of the 3-byte encoded UTF-8 character. This byte does not follow UTF-8 encoding constraints");
						}
						bytes[2] = get();
						break;
					}
					case 4: {
						bytes[0] = get();
						if (has_file_ended()) {
							throw_error(
							    "File ended before reading the second byte of the 4-byte encoded UTF-8 character. The first byte read was " +
							    utils::to_hex_with_prefix(bytes[0], 2));
						}
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the second byte of the 4-byte encoded UTF-8 character. This byte does not follow UTF-8 encoding constraints");
						}
						bytes[1] = get();
						if (has_file_ended()) {
							throw_error(
							    "File ended before reading the third byte of the 4-byte encoded UTF-8 character. The bytes read so far is " +
							    utils::to_hex_with_prefix(bytes[0], 2) + ' ' + utils::to_hex(bytes[1], 2));
						}
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the third byte of the 4-byte encoded UTF-8 character. This byte does not follow UTF-8 encoding constraints");
						}
						bytes[2] = get();
						if (has_file_ended()) {
							throw_error(
							    "File ended before reading the fourth byte of the 4-byte encoded UTF-8 character. The bytes read so far is " +
							    utils::to_hex_with_prefix(bytes[0], 2) + ' ' + utils::to_hex(bytes[1], 2) + ' ' +
							    utils::to_hex(bytes[2], 2));
						}
						if (not utils::is_follow_byte_utf8(get())) {
							throw_error(
							    "Found " + utils::to_hex_with_prefix(get(), 2) +
							    " as the fourth byte of the 4-byte encoded UTF-8 character. This byte does not follow UTF-8 encoding constraints");
						}
						bytes[3] = get();
						break;
					}
				}
				if (breakLoop) {
					break;
				}
				auto scalar = utils::utf8_to_unicode_scalar(bytes);
				if (not scalar.has_value()) {
					throw_error(
					    "The UTF-8 character read could not be converted to a Unicode scalar value. The bytes read are " +
					    utils::to_hex_with_prefix(bytes[0], 2) + ' ' + utils::to_hex(bytes[1], 2) + ' ' +
					    utils::to_hex(bytes[2], 2) + ' ' + utils::to_hex(bytes[3], 2));
				}
				if (idVal.empty() ? (utils::is_unicode_scalar_letter(scalar.value()) || (scalar == 0x5F))
				                  : (utils::is_unicode_scalar_letter(scalar.value()) ||
				                     utils::is_unicode_scalar_digit(scalar.value()) || (scalar == 0x5F))) {
					for (u8 k = 0; k < byteLen.value(); k++) {
						idVal += bytes[k];
					}
				} else if (byteLen.value() == 1) {
					skipPreRead = true;
					break;
				} else {
					String charStr;
					for (u8 k = 0; k < byteLen.value(); k++) {
						charStr += bytes[k];
					}
					throw_error(
					    "The character " + charStr +
					    " cannot be part of an identifier and is also not a recognised symbol in the language. "
					    "Identifiers should start with _ or letters from any unicode supported language, and can "
					    "contain digits from any unicode supported language as well in the middle");
				}
				read();
			}
			if (not skipPreRead) {
				read();
			}
			auto tokRes = word_to_token(idVal, this);
			if (not tokRes.has_value()) {
				throw_error("Could not convert " + idVal + " to a keyword or identifier in the language");
			}
			if ((tokRes.value().type == TokenType::multiPtrType) && (get() == '"')) {
				isMultiStringAllowed = true;
			}
			return tokRes.value();
		}
	}
}

Maybe<Token> Lexer::word_to_token(const String& wordValue, Lexer* lexInst) {
	// SHOW("WordToToken : string value is = " << wordValue)
	auto getPos = [&](usize len) {
		if (lexInst) {
			return lexInst->get_position(len);
		} else {
			return FileRange::null;
		}
	};

	Check_Normal_Keyword("null", null);
	else Check_Normal_Keyword("bring", bring);
	else Check_Normal_Keyword("pub", pub);
	else Check_Normal_Keyword("let", let);
	else Check_Normal_Keyword("self", selfWord);
	else Check_Normal_Keyword("void", voidType);
	else Check_Normal_Keyword("ref", referenceType);
	else Check_Normal_Keyword("type", Type);
	else Check_Normal_Keyword("define", define);
	else Check_Normal_Keyword("skill", skill);
	else Check_Normal_Keyword("pre", pre);
	else Check_Normal_Keyword("up", super);
	//   else Check_Normal_Keyword("const", constant);
	else Check_Normal_Keyword("from", from);
	else Check_Normal_Keyword("to", to);
	else Check_Normal_Keyword("true", TRUE);
	else Check_Normal_Keyword("false", FALSE);
	else Check_Normal_Keyword("say", say);
	else Check_Normal_Keyword("as", as);
	else Check_Normal_Keyword("lib", lib);
	else Check_Normal_Keyword("await", Await);
	else Check_Normal_Keyword("default", Default);
	else Check_Normal_Keyword("static", Static);
	else Check_Normal_Keyword("variadic", variadic);
	else Check_Normal_Keyword("loop", loop);
	else Check_Normal_Keyword("heap", heap);
	else Check_Normal_Keyword("operator", Operator);
	else Check_Normal_Keyword("mix", mix);
	else Check_Normal_Keyword("match", match);
	else Check_Normal_Keyword("copy", copy);
	else Check_Normal_Keyword("move", move);
	else Check_Normal_Keyword("swap", swap);
	else Check_Normal_Keyword("text", textType);
	else Check_Normal_Keyword("ptr", ptrType);
	else Check_Normal_Keyword("multi", multiPtrType);
	else Check_Normal_Keyword("char", characterType);
	else Check_Normal_Keyword("for", For);
	else Check_Normal_Keyword("give", give);
	else Check_Normal_Keyword("var", var);
	else Check_Normal_Keyword("if", If);
	else Check_Normal_Keyword("not", Not);
	else Check_Normal_Keyword("any", any);
	else Check_Normal_Keyword("else", Else);
	else Check_Normal_Keyword("where", where);
	else Check_Normal_Keyword("do", Do);
	else Check_Normal_Keyword("break", Break);
	else Check_Normal_Keyword("continue", Continue);
	else Check_Normal_Keyword("own", own);
	else Check_Normal_Keyword("end", end);
	else Check_Normal_Keyword("choice", choice);
	else Check_Normal_Keyword("flag", flag);
	else Check_Normal_Keyword("future", futureType);
	else Check_Normal_Keyword("maybe", maybeType);
	else Check_Normal_Keyword("none", none);
	else Check_Normal_Keyword("meta", meta);
	else Check_Normal_Keyword("region", region);
	else Check_VALUED_Keyword("bool", unsignedIntegerType);
	else Check_Normal_Keyword("slice", sliceType);
	else Check_Normal_Keyword("struct", structType);
	else Check_Normal_Keyword("toggle", toggle);
	else Check_Normal_Keyword("vec", vectorType);
	else Check_Normal_Keyword("is", is);
	else Check_Normal_Keyword("in", in);
	else Check_Normal_Keyword("ok", ok);
	else Check_Normal_Keyword("of", of);
	else Check_Normal_Keyword("or", orWord);
	else Check_Normal_Keyword("and", andWord);
	else Check_Normal_Keyword("range", range);
	else Check_Normal_Keyword("result", result);
	else Check_Normal_Keyword("error", error);
	else Check_Normal_Keyword("integer", genericIntegerType);
	else Check_Normal_Keyword("opaque", opaque);
	else Check_Normal_Keyword("assembly", assembly);
	else Check_Normal_Keyword("volatile", Volatile);
	else Check_Normal_Keyword("inline", Inline);
	else Check_Normal_Keyword("use", use);
	else Check_Normal_Keyword("poly", polymorph);
	else Check_Normal_Keyword("ignore", ignore);
	else Check_Normal_Keyword("atomic", atomic);
	else Check_VALUED_Keyword("int", nativeType);
	else Check_VALUED_Keyword("uint", nativeType);
	else Check_VALUED_Keyword("byte", nativeType);
	else Check_VALUED_Keyword("ubyte", nativeType);
	else Check_VALUED_Keyword("shortint", nativeType);
	else Check_VALUED_Keyword("ushortint", nativeType);
	else Check_VALUED_Keyword("widechar", nativeType);
	else Check_VALUED_Keyword("uwidechar", nativeType);
	else Check_VALUED_Keyword("longint", nativeType);
	else Check_VALUED_Keyword("ulongint", nativeType);
	else Check_VALUED_Keyword("longlong", nativeType);
	else Check_VALUED_Keyword("ulonglong", nativeType);
	else Check_VALUED_Keyword("usize", nativeType);
	else Check_VALUED_Keyword("isize", nativeType);
	else Check_VALUED_Keyword("float", nativeType);
	else Check_VALUED_Keyword("double", nativeType);
	else Check_VALUED_Keyword("longdouble", nativeType);
	else Check_VALUED_Keyword("intmax", nativeType);
	else Check_VALUED_Keyword("uintmax", nativeType);
	else Check_VALUED_Keyword("intptr", nativeType);
	else Check_VALUED_Keyword("uintptr", nativeType);
	else Check_VALUED_Keyword("ptrdiff", nativeType);
	else Check_VALUED_Keyword("uptrdiff", nativeType);
	else Check_VALUED_Keyword("sigatomic", nativeType);
	else Check_VALUED_Keyword("bytestring", nativeType);
	else Check_VALUED_Keyword("widebool", nativeType);
	else if (wordValue.substr(0, 1) == "u" &&
	         ((wordValue.length() > 1) ? utils::is_integer(wordValue.substr(1, wordValue.length() - 1)) : false)) {
		return Token::valued(TokenType::unsignedIntegerType, wordValue.substr(1, wordValue.length() - 1),
		                     getPos(wordValue.length()));
	}
	else if (wordValue.substr(0, 1) == "i" &&
	         ((wordValue.length() > 1) ? utils::is_integer(wordValue.substr(1, wordValue.length() - 1)) : false)) {
		return Token::valued(TokenType::integerType, wordValue.substr(1, wordValue.length() - 1),
		                     getPos(wordValue.length()));
	}
#define FBRAIN_NAME  "fbrain"
#define F16_NAME     "f16"
#define F32_NAME     "f32"
#define F64_NAME     "f64"
#define F80_NAME     "f80"
#define F128PPC_NAME "f128ppc"
#define F128_NAME    "f128"
	// Yes, I know the lengths of these literals, however repeating the strings can lead me into a rabbit hole
	// of confusing behaviour. It has happened before
	else if (wordValue == FBRAIN_NAME) {
		return Token::valued(TokenType::floatType, FBRAIN_NAME, getPos(std::string::traits_type::length(FBRAIN_NAME)));
	}
	else if (wordValue == F16_NAME) {
		return Token::valued(TokenType::floatType, F16_NAME, getPos(std::string::traits_type::length(F16_NAME)));
	}
	else if (wordValue == F32_NAME) {
		return Token::valued(TokenType::floatType, F32_NAME, getPos(std::string::traits_type::length(F32_NAME)));
	}
	else if (wordValue == F64_NAME) {
		return Token::valued(TokenType::floatType, F64_NAME, getPos(std::string::traits_type::length(F64_NAME)));
	}
	else if (wordValue == F80_NAME) {
		return Token::valued(TokenType::floatType, F80_NAME, getPos(std::string::traits_type::length(F80_NAME)));
	}
	else if (wordValue == F128PPC_NAME) {
		return Token::valued(TokenType::floatType, F128PPC_NAME,
		                     getPos(std::string::traits_type::length(F128PPC_NAME)));
	}
	else if (wordValue == F128_NAME) {
		return Token::valued(TokenType::floatType, F128_NAME, getPos(std::string::traits_type::length(F128_NAME)));
	}
	else {
		if (wordValue.empty()) {
			return None;
		}
		// IMPORTANT - If there is no lexer instance, the word is checked (Useful for compiler API or integration). If
		// there is, it is assumed that the identifier is checked beforehand, which it should be.
		if (not lexInst) {
			auto firstByteLen = utils::get_utf8_byte_length(wordValue[0]);
			if (not firstByteLen.has_value()) {
				return None;
			}
			if (firstByteLen.value() > wordValue.length()) {
				return None;
			}
			std::array<u8, 4> firstBytes{0, 0, 0, 0};
			for (u8 i = 0; i < firstByteLen.value(); i++) {
				firstBytes[i] = wordValue[i];
			}
			auto firstScalar = utils::utf8_to_unicode_scalar(firstBytes);
			if (not firstScalar.has_value()) {
				return None;
			}
			if (utils::is_unicode_scalar_letter(firstScalar.value()) || (firstScalar.value() == 0x5F)) {
				for (usize i = firstByteLen.value(); i < wordValue.length();) {
					auto byteLen = utils::get_utf8_byte_length(wordValue[i]);
					if (not byteLen.has_value()) {
						return None;
					}
					std::array<u8, 4> bytes{0, 0, 0, 0};
					for (u8 j = 0; j < byteLen.value(); j++) {
						bytes[j] = wordValue[i + j];
					}
					auto scalar = utils::utf8_to_unicode_scalar(bytes);
					if (not scalar.has_value()) {
						return None;
					}
					if (not(utils::is_unicode_scalar_letter(scalar.value()) ||
					        utils::is_unicode_scalar_digit(scalar.value()) || (scalar.value() == 0x5F))) {
						return None;
					}
					i += byteLen.value();
				}
				return Token::valued(TokenType::identifier, wordValue, getPos(wordValue.length()));
			} else {
				return None;
			}
		} else {
			return Token::valued(TokenType::identifier, wordValue, getPos(wordValue.length()));
		}
	}
}

void Lexer::throw_error(const String& message, Maybe<usize> offset) {
	irCtx->Error(message,
	             offset.has_value()
	                 ? get_position(offset.value())
	                 : FileRange::from(filePath, FilePos{lineNumber, byteNumber > 0 ? byteNumber - 1 : byteNumber},
	                                   FilePos{lineNumber, byteNumber > 0 ? byteNumber : byteNumber + 1}));
}

} // namespace qat::lexer
