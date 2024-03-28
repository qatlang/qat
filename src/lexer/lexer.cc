#include "lexer.hpp"
#include "../IR/context.hpp"
#include "../cli/color.hpp"
#include "../show.hpp"
#include "../utils/utils.hpp"
#include "token_type.hpp"
#include <chrono>
#include <string>

#define NanosecondsInMicroseconds 1000
#define NanosecondsInMilliseconds 1000000
#define NanosecondsInSeconds      1000000000

#define Check_Normal_Keyword(ident, tokenName)                                                                         \
  if (wordValue == ident)                                                                                              \
  return Token::normal(TokenType::tokenName, getPos(std::string::traits_type::length(ident)))

#define Check_VALUED_Keyword(ident, tokenName)                                                                         \
  if (wordValue == ident)                                                                                              \
  return Token::valued(TokenType::tokenName, ident, getPos(std::string::traits_type::length(ident)))

namespace qat::lexer {

Lexer* Lexer::get(ir::Ctx* irCtx) { return std::construct_at(OwnTracked(Lexer), irCtx); }

Lexer::~Lexer() {
  SHOW("About to delete remaining tokens")
  delete tokens;
  tokens = nullptr;
  SHOW("Tokens deleted")
  if (file.is_open()) {
    file.close();
    SHOW("Closed file that was open in the lexer")
  }
  SHOW("Cleared file content")
  buffer.clear();
  SHOW("Cleared token buffer")
}

u64 Lexer::timeInMicroSeconds = 0;
u64 Lexer::lineCount          = 0;

Vec<Token>* Lexer::get_tokens() {
  auto* res = tokens;
  tokens    = nullptr;
  return res;
}

void Lexer::read() {
  //   try {
  if (file.eof()) {
    prev    = current;
    current = -1;
  } else {
    prev = current;
    file.get(current);
    characterNumber++;
  }
  if (current == '\n') {
    previousLineEnd = characterNumber - 1;
    lineNumber++;
    characterNumber = 0;
  } else if (current == '\r') {
    prev = current;
    file.get(current);
    if (current != -1) {
      if (current == '\n') {
        previousLineEnd = characterNumber - 2;
        lineNumber++;
        characterNumber = 0;
      } else {
        characterNumber++;
      }
    }
    characterNumber = (current == '\n') ? 0 : 1;
  }
  //   } catch (std::exception& err) {
  //     throwError(String("Lexer failed while reading the file. Error: ") + err.what());
  //   }
}

FileRange Lexer::get_position(u64 length) {
  FilePos end = {lineNumber, characterNumber > 0 ? (characterNumber - 1) : characterNumber};
  if (characterNumber == 0) {
    if (previousLineEnd.has_value()) {
      end = {lineNumber - 1, previousLineEnd.value()};
    }
  }
  return {fs::path(filePath), {end.line, end.character - length}, end};
}

void Lexer::analyse() {
  file.open(filePath, std::ios::in);
  auto startTime = std::chrono::high_resolution_clock::now();
  tokens->push_back(Token::valued(TokenType::startOfFile, filePath.string(), this->get_position(0)));
  read();
  while (!file.eof()) {
    tokens->push_back(tokeniser());
  }
  file.close();
  if (tokens->back().type != TokenType::endOfFile) {
    tokens->push_back(Token::valued(TokenType::endOfFile, filePath.string(), this->get_position(0)));
  }
  timeInMicroSeconds +=
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime)
          .count();
  lineCount += lineNumber;
}

void Lexer::change_file(fs::path newFilePath) {
  tokens          = new Vec<Token>();
  filePath        = std::move(newFilePath);
  prev            = -1;
  current         = -1;
  lineNumber      = 1;
  characterNumber = 0;
}

#define LOWER_LETTER_FIRST 'a'
#define LOWER_LETTER_LAST  'z'
#define UPPER_LETTER_FIRST 'A'
#define UPPER_LETTER_LAST  'Z'
#define DIGIT_FIRST        '0'
#define DIGIT_LAST         '9'
#define CURRENT_IS_ALPHABET                                                                                            \
  ((current >= LOWER_LETTER_FIRST && current <= LOWER_LETTER_LAST) ||                                                  \
   (current >= UPPER_LETTER_FIRST && current <= UPPER_LETTER_LAST))

#define CURRENT_IS_DIGIT (current >= DIGIT_FIRST && current <= DIGIT_LAST)

Token Lexer::tokeniser() {
  if (!buffer.empty()) {
    Token token = buffer.back();
    buffer.pop_back();
    return token;
  }
  if (file.eof()) {
    return Token::valued(TokenType::endOfFile, filePath.string(), this->get_position(0));
  }
  if (prev == '\r') {
    read();
    return tokeniser();
  }
  switch (current) {
    case ' ':
    case '\n':
    case '\r':
    case '\t': {
      read();
      return tokeniser();
    }
    case '.': {
      read();
      return Token::normal(TokenType::stop, this->get_position(1));
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
      if ((!bracketOccurences.empty()) && (bracketOccurences.back() == TokenType::genericTypeStart)) {
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
    case '@': {
      read();
      return Token::normal(TokenType::referenceType, this->get_position(1));
    }
    case '^': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "^=", this->get_position(2));
      }
      return Token::valued(TokenType::binaryOperator, "^", this->get_position(1));
    }
    case ':': {
      read();
      if (current == '[') {
        read();
        bracketOccurences.push_back(TokenType::genericTypeStart);
        return Token::normal(TokenType::genericTypeStart, this->get_position(2));
      } else if (current == '=') {
        read();
        return Token::normal(TokenType::associatedAssignment, this->get_position(2));
      } else if (current == ':') {
        read();
        return Token::normal(TokenType::typeSeparator, this->get_position(2));
      } else {
        return Token::normal(TokenType::colon, this->get_position(1));
      }
    }
    case '/': {
      String value = "/";
      read();
      if (current == '*') {
        bool   star = false;
        String commentValue;
        read();
        auto commentPos = this->get_position(0);
        while ((!star || (current != '/')) && !file.eof()) {
          if (star) {
            star = false;
          }
          if (current == '*') {
            star = true;
          }
          read();
          if (!star || (current != '/')) {
            commentValue += current;
            if (current == '\n') {
              commentPos.end.line++;
              commentPos.end.character = 0;
            } else {
              commentPos.end.character++;
            }
          }
        }
        commentPos.end.character--;
        read();
        return Token::valued(TokenType::comment, commentValue, commentPos);
      } else if (current == '/') {
        String commentValue;
        auto   commentPos = this->get_position(0);
        while ((current != '\n' && prev != '\r') && !file.eof()) {
          read();
          if (current != '\n' && current != '\r') {
            commentValue += current;
            commentPos.end.character++;
          }
        }
        SHOW("Single line comment value is " << commentValue)
        return Token::valued(TokenType::comment, commentValue, this->get_position(commentValue.length()));
      } else {
        return Token::valued(TokenType::binaryOperator, value, this->get_position(1));
      }
    }
    case '!': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::binaryOperator, "!=", this->get_position(2));
      } else {
        return Token::valued(TokenType::exclamation, "!", this->get_position(1));
      }
    }
    case '~': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "~=", this->get_position(2));
      } else {
        return Token::valued(TokenType::unaryOperator, "~", this->get_position(1));
      }
    }
    case '&': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "&=", this->get_position(2));
      } else if (current == '&') {
        read();
        return Token::valued(TokenType::binaryOperator, "&&", this->get_position(2));
      } else {
        return Token::valued(TokenType::binaryOperator, "&", this->get_position(1));
      }
    }
    case '|': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "|=", this->get_position(2));
      } else if (current == '|') {
        read();
        return Token::valued(TokenType::binaryOperator, "||", this->get_position(2));
      } else {
        return Token::valued(TokenType::binaryOperator, "|", this->get_position(1));
      }
    }
    case '?': {
      read();
      if (current == '?') {
        read();
        if (current == '=') {
          read();
          return Token::normal(TokenType::assignToNullPointer, this->get_position(3));
        } else {
          return Token::normal(TokenType::isNullPointer, this->get_position(2));
        }
      } else if (current == '!') {
        read();
        if (current == '=') {
          read();
          return Token::normal(TokenType::assignToNonNullPointer, this->get_position(3));
        } else {
          return Token::normal(TokenType::isNotNullPointer, this->get_position(2));
        }
      } else {
        return Token::normal(TokenType::questionMark, this->get_position(1));
      }
    }
    case '+':
    case '-':
    case '%':
    case '*':
    case '<':
    case '>': {
      String operatorValue;
      operatorValue += current;
      read();
      if (current == '=' && operatorValue != "<" && operatorValue != ">") {
        operatorValue += current;
        read();
        return Token::valued(TokenType::assignedBinaryOperator, operatorValue, this->get_position(2));
      } else if (current == '=' && (operatorValue == "<" || operatorValue == ">")) {
        operatorValue += current;
        read();
        return Token::valued(TokenType::binaryOperator, operatorValue, this->get_position(2));
      } else if ((current == '<' && operatorValue == "<") || (current == '>' && operatorValue == ">")) {
        operatorValue += current;
        read();
        return Token::valued(TokenType::binaryOperator, operatorValue, this->get_position(2));
      } else if (current == '>' && operatorValue == "-") {
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
      if (current == '=') {
        read();
        return Token::valued(TokenType::binaryOperator, "==", this->get_position(2));
      } else if (current == '>') {
        read();
        return Token::normal(TokenType::altArrow, this->get_position(2));
      } else {
        return Token::normal(TokenType::assignment, this->get_position(1));
      }
    }
    case '\'': {
      read();
      if (current == '\'') {
        read();
        return Token::normal(TokenType::self_instance, this->get_position(2));
      } else {
        return Token::normal(TokenType::child, this->get_position(1));
      }
    }
    case ';': {
      read();
      return Token::normal(TokenType::semiColon, this->get_position(1));
    }
    case '"': {
      bool escape = false;
      read();
      String str_val;
      while (escape ? !file.eof() : (current != '"' && !file.eof())) {
        if (escape) {
          escape = false;
          if (current == '"') {
            str_val += '"';
          } else if (current == '\\') {
            str_val += "\\\\";
          } else if (current == 'n') {
            str_val += "\n";
          } else if (current == '?') {
            str_val += "\?";
          } else if (current == 'b') {
            str_val += "\b";
          } else if (current == 'a') {
            str_val += "\a";
          } else if (current == 'f') {
            str_val += "\f";
          } else if (current == 'r') {
            str_val += "\r";
          } else if (current == 't') {
            str_val += "\t";
          } else if (current == 'v') {
            str_val += "\v";
          }
        } else {
          if (current == '\\' && prev != '\\') {
            escape = true;
          } else {
            str_val += current;
          }
        }
        read();
      }
      read();
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
      if (current == '0') {
        read();
        if (current == 'b') {
          read();
          numVal     = "0b";
          foundRadix = true;
        } else if (current == 'c') {
          read();
          numVal     = "0c";
          foundRadix = true;
        } else if (current == 'x') {
          read();
          numVal     = "0x";
          foundRadix = true;
        } else if (current == 'r') {
          numVal += "0r";
          read();
          while (CURRENT_IS_DIGIT) {
            numVal += current;
            read();
          }
          if (current == '_') {
            numVal += '_';
            read();
          } else {
            throw_error("Invalid custom radix integer literal");
          }
          foundRadix = true;
        } else {
          numVal += "0";
        }
      }
      bool foundSpec = false;
      while ((CURRENT_IS_DIGIT || (foundRadix && !foundSpec && CURRENT_IS_ALPHABET) ||
              (!is_float && (current == '.')) || (!foundRadix && !exponentialFloat && (current == 'e')) ||
              (!foundSpec && (current == '_'))) &&
             !file.eof()) {
        if (!foundRadix && !exponentialFloat && current == 'e') {
          is_float         = true;
          exponentialFloat = true;
          String expStr("e");
          read();
          if (current == '-') {
            expStr += current;
            read();
          }
          while (CURRENT_IS_DIGIT) {
            expStr += current;
            read();
          }
          numVal += expStr;
          continue;
        } else if (current == '.') {
          read();
          if (CURRENT_IS_DIGIT) {
            if (foundRadix) {
              throw_error("This literal is in custom radix format and hence cannot contain decimal point ",
                          numVal.length() + 1);
            }
            is_float = true;
            numVal += '.';
          } else {
            /// This is in the reverse order since the last element is returned
            /// first
            buffer.push_back(Token::normal(TokenType::stop, this->get_position(1)));
            auto fileRange = this->get_position(numVal.length() + 1);
            if (fileRange.end.character > 0) {
              fileRange.end.character--;
            }
            buffer.push_back(
                Token::valued(is_float ? TokenType::floatLiteral : TokenType::integerLiteral, numVal, fileRange));
            return tokeniser();
          }
        } else if (current == '_') {
          String specString;
          read();
          if (CURRENT_IS_DIGIT) {
            numVal += "_";
          } else if (CURRENT_IS_ALPHABET) {
            foundSpec  = true;
            specString = "_";
            while (CURRENT_IS_ALPHABET || CURRENT_IS_DIGIT) {
              specString += current;
              read();
            }
            numVal += specString;
            if (specString == "_f32" || specString == "_f64" || specString == "_f128" || specString == "_f128ppc" ||
                specString == "_f16" || specString == "_fbrain" || specString == "_float" || specString == "_double" ||
                specString == "_longdouble") {
              is_float = true;
            }
            return Token::valued(is_float ? TokenType::floatLiteral : TokenType::integerLiteral, numVal,
                                 this->get_position(numVal.length()));
          } else {
            throw_error("Invalid literal. Found _ without anything following");
          }
        }
        numVal += current;
        read();
      }
      return Token::valued(is_float ? TokenType::floatLiteral : TokenType::integerLiteral, numVal,
                           this->get_position(numVal.length()));
    }
    case -1: {
      return Token::valued(TokenType::endOfFile, filePath.string(), this->get_position(0));
    }
    default: {
      if (CURRENT_IS_ALPHABET || current == '_') {
        String value;
        while ((CURRENT_IS_ALPHABET || CURRENT_IS_DIGIT || (current == '_')) && !file.eof()) {
          value += current;
          read();
        }
        auto wordRes = word_to_token(value, this);
        if (wordRes.has_value()) {
          return wordRes.value();
        } else {
          throw_error("Could not convert the value " + value + " to a token");
          return Token::normal(TokenType::endOfFile, this->get_position(0));
        }
      } else {
        throw_error("Unrecognised character found: " + String(1, current));
        return Token::normal(TokenType::endOfFile, this->get_position(0));
      }
    }
  }
}

Maybe<Token> Lexer::word_to_token(const String& wordValue, Lexer* lexInst) {
  // SHOW("WordToToken : string value is = " << wordValue)
  auto getPos = [&](usize len) {
    if (lexInst) {
      return lexInst->get_position(len);
    } else {
      return FileRange("", {0u, 0u}, {0u, 0u});
    }
  };

  Check_Normal_Keyword("null", null);
  else Check_Normal_Keyword("bring", bring);
  else Check_Normal_Keyword("pub", Public);
  else Check_Normal_Keyword("let", let);
  else Check_Normal_Keyword("self", selfWord);
  else Check_Normal_Keyword("void", voidType);
  else Check_Normal_Keyword("type", Type);
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
  else Check_Normal_Keyword("while", While);
  else Check_Normal_Keyword("over", over);
  else Check_Normal_Keyword("heap", heap);
  else Check_Normal_Keyword("operator", Operator);
  else Check_Normal_Keyword("mix", mix);
  else Check_Normal_Keyword("match", match);
  else Check_Normal_Keyword("copy", copy);
  else Check_Normal_Keyword("move", move);
  else Check_Normal_Keyword("str", stringSliceType);
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
  else Check_Normal_Keyword("disown", disown);
  else Check_Normal_Keyword("end", end);
  else Check_Normal_Keyword("choice", choice);
  else Check_Normal_Keyword("future", futureType);
  else Check_Normal_Keyword("maybe", maybeType);
  else Check_Normal_Keyword("none", none);
  else Check_Normal_Keyword("meta", meta);
  else Check_Normal_Keyword("region", region);
  else Check_VALUED_Keyword("bool", unsignedIntegerType);
  else Check_Normal_Keyword("ptr", pointerType);
  else Check_Normal_Keyword("multiptr", multiPointerType);
  else Check_Normal_Keyword("vec", vectorType);
  else Check_Normal_Keyword("is", is);
  else Check_Normal_Keyword("ok", ok);
  else Check_Normal_Keyword("range", range);
  else Check_Normal_Keyword("result", result);
  else Check_Normal_Keyword("opaque", opaque);
  else Check_VALUED_Keyword("int", cType);
  else Check_VALUED_Keyword("uint", cType);
  else Check_VALUED_Keyword("char", cType);
  else Check_VALUED_Keyword("uchar", cType);
  else Check_VALUED_Keyword("shortint", cType);
  else Check_VALUED_Keyword("ushortint", cType);
  else Check_VALUED_Keyword("widechar", cType);
  else Check_VALUED_Keyword("uwidechar", cType);
  else Check_VALUED_Keyword("longint", cType);
  else Check_VALUED_Keyword("ulongint", cType);
  else Check_VALUED_Keyword("longlong", cType);
  else Check_VALUED_Keyword("ulonglong", cType);
  else Check_VALUED_Keyword("usize", cType);
  else Check_VALUED_Keyword("isize", cType);
  else Check_VALUED_Keyword("float", cType);
  else Check_VALUED_Keyword("double", cType);
  else Check_VALUED_Keyword("longdouble", cType);
  else Check_VALUED_Keyword("intmax", cType);
  else Check_VALUED_Keyword("uintmax", cType);
  else Check_VALUED_Keyword("intptr", cType);
  else Check_VALUED_Keyword("uintptr", cType);
  else Check_VALUED_Keyword("ptrdiff", cType);
  else Check_VALUED_Keyword("uptrdiff", cType);
  else Check_VALUED_Keyword("cSigAtomic", cType);
  else Check_VALUED_Keyword("cProcessID", cType);
  else Check_VALUED_Keyword("cStr", cType);
  else Check_VALUED_Keyword("cBool", cType);
  else Check_VALUED_Keyword("cPtr", cType);
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
    return Token::valued(TokenType::floatType, F128PPC_NAME, getPos(std::string::traits_type::length(F128PPC_NAME)));
  }
  else if (wordValue == F128_NAME) {
    return Token::valued(TokenType::floatType, F128_NAME, getPos(std::string::traits_type::length(F128_NAME)));
  }
  else {
    if (wordValue.empty()) {
      return None;
    }
    if (!((wordValue[0] >= LOWER_LETTER_FIRST && wordValue[0] <= LOWER_LETTER_LAST) ||
          (wordValue[0] >= UPPER_LETTER_FIRST && wordValue[0] <= UPPER_LETTER_LAST))) {
      return None;
    }
    for (auto current : wordValue) {
      if (!(CURRENT_IS_ALPHABET || CURRENT_IS_DIGIT || (current == '_'))) {
        return None;
      }
    }
    return Token::valued(TokenType::identifier, wordValue, getPos(wordValue.length()));
  }
}

void Lexer::throw_error(const String& message, Maybe<usize> offset) {
  irCtx->Error(message, offset.has_value() ? get_position(offset.value())
                                           : FileRange{filePath, FilePos{lineNumber, characterNumber},
                                                       FilePos{lineNumber, characterNumber + 1}});
}

} // namespace qat::lexer