#include "lexer.hpp"
#include "../cli/color.hpp"
#include "../memory_tracker.hpp"
#include "../show.hpp"
#include "../utils/is_integer.hpp"
#include "token_type.hpp"
#include <chrono>

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

Lexer::~Lexer() {
  SHOW("About to delete remaining tokens")
  delete tokens;
  tokens = nullptr;
  SHOW("Tokens deleted")
  if (file.is_open()) {
    file.close();
    SHOW("Closed file that was open in the lexer")
  }
  content.clear();
  SHOW("Cleared file content")
  buffer.clear();
  SHOW("Cleared token buffer")
}

u64 Lexer::timeInMicroSeconds = 0;

Deque<Token>* Lexer::getTokens() {
  auto* res = tokens;
  tokens    = nullptr;
  return res;
}

Vec<String> Lexer::getContent() const { return content; }

void Lexer::read() {
  try {
    if (file.eof()) {
      prev    = current;
      current = -1;
    } else {
      prev = current;
      file.get(current);
      if (current != -1) {
        if (content.empty()) {
          content.push_back("");
        }
        if (current != '\n' || current != '\r') {
          content.back() += current;
        }
      }
      characterNumber++;
      totalCharacterCount++;
    }
    if (current == '\n') {
      previousLineEnd = characterNumber;
      lineNumber++;
      characterNumber = 0;
      content.push_back("");
    } else if (current == '\r') {
      prev = current;
      file.get(current);
      if (current != -1) {
        if (current == '\n') {
          previousLineEnd = characterNumber;
          lineNumber++;
          characterNumber = 0;
          totalCharacterCount++;
          content.push_back("");
        } else {
          characterNumber++;
          totalCharacterCount++;
          (content.back() += "\r") += current;
        }
      }
      totalCharacterCount++;
      characterNumber = (current == '\n') ? 0 : 1;
    }
  } catch (std::exception& err) {
    throwError(String("Lexer failed while reading the file. Error: ") + err.what());
  }
}

FileRange Lexer::getPosition(u64 length) {
  FilePos end = {lineNumber, characterNumber};
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
  tokens->push_back(Token::valued(TokenType::startOfFile, filePath.string(), this->getPosition(0)));
  read();
  while (!file.eof()) {
    tokens->push_back(tokeniser());
  }
  file.close();
  if (tokens->back().type != TokenType::endOfFile) {
    tokens->push_back(Token::valued(TokenType::endOfFile, filePath.string(), this->getPosition(0)));
  }
  if (tokens->size() == 2) {
    throwError("This file is empty");
  }
  timeInMicroSeconds +=
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime)
          .count();
}

void Lexer::changeFile(fs::path newFilePath) {
  tokens = new Deque<Token>{};
  content.clear();
  filePath            = std::move(newFilePath);
  prev                = -1;
  current             = -1;
  lineNumber          = 1;
  characterNumber     = 0;
  totalCharacterCount = 0;
}

Token Lexer::tokeniser() {
  if (!buffer.empty()) {
    Token token = buffer.back();
    buffer.pop_back();
    return token;
  }
  if (file.eof()) {
    return Token::valued(TokenType::endOfFile, filePath.string(), this->getPosition(0));
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
      if (current == '.') {
        read();
        return Token::normal(TokenType::super, this->getPosition(2));
      }
      return Token::normal(TokenType::stop, this->getPosition(1));
    }
    case ',': {
      read();
      return Token::normal(TokenType::separator, this->getPosition(1));
    }
    case '(': {
      read();
      return Token::normal(TokenType::parenthesisOpen, this->getPosition(1));
    }
    case ')': {
      read();
      return Token::normal(TokenType::parenthesisClose, this->getPosition(1));
    }
    case '[': {
      read();
      bracketOccurences.push_back(TokenType::bracketOpen);
      return Token::normal(TokenType::bracketOpen, this->getPosition(1));
    }
    case ']': {
      read();
      if ((!bracketOccurences.empty()) && (bracketOccurences.back() == TokenType::genericTypeStart)) {
        bracketOccurences.pop_back();
        return Token::normal(TokenType::genericTypeEnd, this->getPosition(1));
      } else {
        bracketOccurences.pop_back();
        return Token::normal(TokenType::bracketClose, this->getPosition(1));
      }
    }
    case '{': {
      read();
      return Token::normal(TokenType::curlybraceOpen, this->getPosition(1));
    }
    case '}': {
      read();
      return Token::normal(TokenType::curlybraceClose, this->getPosition(1));
    }
    case '@': {
      read();
      return Token::normal(TokenType::referenceType, this->getPosition(1));
    }
    case '^': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "^=", this->getPosition(2));
      }
      return Token::valued(TokenType::binaryOperator, "^", this->getPosition(1));
    }
    case ':': {
      read();
      if (current == '=') {
        read();
        return Token::normal(TokenType::associatedAssignment, this->getPosition(2));
      } else if (current == ':') {
        read();
        return Token::normal(TokenType::typeSeparator, this->getPosition(2));
      } else {
        return Token::normal(TokenType::colon, this->getPosition(1));
      }
    }
    case '/': {
      String value = "/";
      read();
      if (current == '*') {
        bool star = false;
        read();
        while ((!star || (current != '/')) && !file.eof()) {
          if (star) {
            star = false;
          }
          if (current == '*') {
            star = true;
          }
          read();
        }
        if (file.eof()) {
          return Token::valued(TokenType::endOfFile, filePath.string(), this->getPosition(0));
        } else {
          read();
          return tokeniser();
        }
      } else if (current == '/') {
        while ((current != '\n' && prev != '\r') && !file.eof()) {
          read();
        }
        if (file.eof()) {
          return Token::valued(TokenType::endOfFile, filePath.string(), this->getPosition(0));
        } else {
          read();
          return tokeniser();
        }
      } else {
        return Token::valued(TokenType::binaryOperator, value, this->getPosition(1));
      }
    }
    case '!': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::binaryOperator, "!=", this->getPosition(2));
      } else {
        return Token::valued(TokenType::unaryOperator, "!", this->getPosition(1));
      }
    }
    case '~': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "~=", this->getPosition(2));
      } else {
        return Token::valued(TokenType::binaryOperator, "~", this->getPosition(1));
      }
    }
    case '&': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "&=", this->getPosition(2));
      } else if (current == '&') {
        read();
        return Token::valued(TokenType::binaryOperator, "&&", this->getPosition(2));
      } else {
        return Token::valued(TokenType::binaryOperator, "&", this->getPosition(1));
      }
    }
    case '|': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "|=", this->getPosition(2));
      } else if (current == '|') {
        read();
        return Token::valued(TokenType::binaryOperator, "||", this->getPosition(2));
      } else {
        return Token::valued(TokenType::binaryOperator, "|", this->getPosition(1));
      }
    }
    case '?': {
      read();
      if (current == '?') {
        read();
        if (current == '=') {
          read();
          return Token::normal(TokenType::assignToNullPointer, this->getPosition(3));
        } else {
          return Token::normal(TokenType::isNullPointer, this->getPosition(2));
        }
      } else if (current == '!') {
        read();
        if (current == '=') {
          read();
          return Token::normal(TokenType::assignToNonNullPointer, this->getPosition(3));
        } else {
          return Token::normal(TokenType::isNotNullPointer, this->getPosition(2));
        }
      } else {
        return Token::normal(TokenType::ternary, this->getPosition(1));
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
      if ((current == '+' && operatorValue == "+") || (current == '-' && operatorValue == "-")) {
        operatorValue += current;
        read();
        const String identifierStart = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
        return Token::valued(TokenType::unaryOperator, operatorValue, this->getPosition(2));
      } else if (current == '=' && operatorValue != "<" && operatorValue != ">") {
        operatorValue += current;
        read();
        return Token::valued(TokenType::assignedBinaryOperator, operatorValue, this->getPosition(2));
      } else if (current == '=' && (operatorValue == "<" || operatorValue == ">")) {
        operatorValue += current;
        read();
        return Token::valued(TokenType::binaryOperator, operatorValue, this->getPosition(2));
      } else if ((current == '<' && operatorValue == "<") || (current == '>' && operatorValue == ">")) {
        operatorValue += current;
        read();
        return Token::valued(TokenType::binaryOperator, operatorValue, this->getPosition(2));
      } else if (current == '~' && operatorValue == "<") {
        read();
        return Token::normal(TokenType::variationMarker, this->getPosition(2));
      } else if (current == '>' && operatorValue == "-") {
        read();
        return Token::normal(TokenType::givenTypeSeparator, this->getPosition(2));
      } else if (operatorValue == "<") {
        return Token::valued(TokenType::binaryOperator, "<", this->getPosition(1));
      } else if (operatorValue == ">") {
        return Token::valued(TokenType::binaryOperator, ">", this->getPosition(1));
      }
      return Token::valued(TokenType::binaryOperator, operatorValue, this->getPosition(1));
    }
    case '=': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::binaryOperator, "==", this->getPosition(2));
      } else if (current == '>') {
        read();
        return Token::normal(TokenType::altArrow, this->getPosition(2));
      } else {
        return Token::normal(TokenType::assignment, this->getPosition(1));
      }
    }
    case '\'': {
      read();
      if (current == '[') {
        read();
        bracketOccurences.push_back(TokenType::genericTypeStart);
        return Token::normal(TokenType::genericTypeStart, this->getPosition(2));
      } else if (current == '\'') {
        read();
        return Token::normal(TokenType::self, this->getPosition(2));
      } else {
        return Token::normal(TokenType::child, this->getPosition(1));
      }
    }
    case ';': {
      read();
      return Token::normal(TokenType::semiColon, this->getPosition(1));
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
      return Token::valued(TokenType::StringLiteral, str_val, this->getPosition(str_val.length() + 2));
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
      String       numVal;
      bool         is_float = false;
      const String alphabets("abcdefghijklmnopqrstuvwxyz");
      const String digits("0123456789");
      bool         found_spec = false;
      while (((digits.find(current, 0) != String::npos) || (!is_float && (current == '.')) ||
              (!found_spec && (current == '_'))) &&
             !file.eof()) {
        if (current == '.') {
          read();
          if (digits.find(current, 0) != String::npos) {
            is_float = true;
            numVal += '.';
          } else {
            /// This is in the reverse order since the last element is returned
            /// first
            buffer.push_back(Token::normal(TokenType::stop, this->getPosition(1)));
            auto fileRange = this->getPosition(numVal.length() + 1);
            if (fileRange.end.character > 0) {
              fileRange.end.character--;
            }
            buffer.push_back(Token::valued(TokenType::integerLiteral, numVal, fileRange));
            return tokeniser();
          }
        } else if (current == '_') {
          found_spec = true;
          String bitString;
          read();
          if (current == 'u') {
            read();
            if (current == 's') {
              read();
              if (current == 'i') {
                read();
                if (current == 'z') {
                  read();
                  if (current == 'e') {
                    read();
                    auto resString = numVal + "_usize";
                    return Token::valued(TokenType::unsignedLiteral, resString, this->getPosition(resString.length()));
                  }
                } else {
                  throwError("Invalid unsigned integer literal");
                }
              } else {
                throwError("Invalid unsigned integer literal");
              }
            } else {
              while (digits.find(current) != String::npos) {
                bitString += current;
                read();
              }
              auto resString = numVal;
              resString.append("_u").append(bitString);
              return Token::valued(TokenType::unsignedLiteral, resString, this->getPosition(resString.length()));
            }
          } else if (current == 'i') {
            read();
            while (digits.find(current) != String::npos) {
              bitString += current;
              read();
            }
            auto resString = numVal;
            resString.append("_i").append(bitString);
            return Token::valued(TokenType::integerLiteral, resString, this->getPosition(resString.length()));
          } else if (current == 'f') {
            read();
            while ((digits.find(current) != String::npos) || (alphabets.find(current) != String::npos)) {
              bitString += current;
              read();
            }
            auto resString = numVal;
            resString.append("_f").append(bitString);
            return Token::valued(TokenType::floatLiteral, resString, this->getPosition(resString.length()));
          } else {
            throwError("Invalid integer literal suffix");
          }
        }
        numVal += current;
        read();
      }
      return Token::valued(is_float ? TokenType::floatLiteral : TokenType::integerLiteral, numVal,
                           this->getPosition(numVal.length()) //
      );
    }
    case -1: {
      return Token::valued(TokenType::endOfFile, filePath.string(), this->getPosition(0));
    }
    default: {
      const String alphabets = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
      if ((alphabets.find(current, 0) != String::npos) || current == '_') {
        const String digits = "0123456789";
        String       value;
        while (((alphabets.find(current, 0) != String::npos) || (digits.find(current, 0) != String::npos) ||
                (current == '_')) &&
               !file.eof()) {
          value += current;
          read();
        }
        return wordToToken(value, this);
      } else {
        throwError("Unrecognised character found: " + String(1, current));
        return Token::normal(TokenType::endOfFile, this->getPosition(0));
      }
    }
  }
}

Token Lexer::wordToToken(const String& wordValue, Lexer* lexInst) {
  std::cout << "WordToToken\n";
  std::cout << wordValue;
  SHOW("WordToToken : string value is = " << wordValue)
  auto getPos = [&](usize len) {
    if (lexInst) {
      return lexInst->getPosition(len);
    } else {
      return FileRange("", {0u, 0u}, {0u, 0u});
    }
  };

  Check_Normal_Keyword("null", null);
  else Check_Normal_Keyword("bring", bring);
  else Check_Normal_Keyword("pub", Public);
  else Check_Normal_Keyword("new", New);
  else Check_Normal_Keyword("self", self);
  else Check_Normal_Keyword("void", voidType);
  else Check_Normal_Keyword("type", Type);
  else Check_Normal_Keyword("const", constant);
  else Check_Normal_Keyword("box", box);
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
  else Check_Normal_Keyword("async", Async);
  else Check_Normal_Keyword("sizeOf", sizeOf);
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
  else Check_Normal_Keyword("else", Else);
  else Check_Normal_Keyword("break", Break);
  else Check_Normal_Keyword("continue", Continue);
  else Check_Normal_Keyword("own", own);
  else Check_Normal_Keyword("disown", disown);
  else Check_Normal_Keyword("end", end);
  else Check_Normal_Keyword("choice", choice);
  else Check_Normal_Keyword("future", future);
  else Check_Normal_Keyword("maybe", maybe);
  else Check_Normal_Keyword("none", none);
  else Check_Normal_Keyword("meta", meta);
  else Check_Normal_Keyword("region", region);
  else Check_VALUED_Keyword("bool", unsignedIntegerType);
  else Check_Normal_Keyword("ptr", pointerType);
  else Check_Normal_Keyword("multiptr", multiPointerType);
  else Check_Normal_Keyword("range", range);
  else Check_VALUED_Keyword("int", cType);
  else Check_VALUED_Keyword("uint", cType);
  else Check_VALUED_Keyword("char", cType);
  else Check_VALUED_Keyword("uchar", cType);
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
  else Check_VALUED_Keyword("cFloatHalf", cType);
  else Check_VALUED_Keyword("cFloatBrain", cType);
  else Check_VALUED_Keyword("cFloat128", cType);
  else Check_VALUED_Keyword("cStr", cType);
  else Check_VALUED_Keyword("cBool", cType);
  else Check_VALUED_Keyword("cPtr", cType);
  else if (wordValue.substr(0, 1) == "u" &&
           ((wordValue.length() > 1) ? utils::isInteger(wordValue.substr(1, wordValue.length() - 1)) : false)) {
    return Token::valued(TokenType::unsignedIntegerType, wordValue.substr(1, wordValue.length() - 1),
                         getPos(wordValue.length()));
  }
  else if (wordValue.substr(0, 1) == "i" &&
           ((wordValue.length() > 1) ? utils::isInteger(wordValue.substr(1, wordValue.length() - 1)) : false)) {
    return Token::valued(TokenType::integerType, wordValue.substr(1, wordValue.length() - 1),
                         getPos(wordValue.length()));
  }
  else if (wordValue == "fbrain") {
    return Token::valued(TokenType::floatType, "brain", getPos(6));
  }
  else if (wordValue == "f16") {
    return Token::valued(TokenType::floatType, "16", getPos(3));
  }
  else if (wordValue == "f32") {
    return Token::valued(TokenType::floatType, "32", getPos(3));
  }
  else if (wordValue == "f64") {
    return Token::valued(TokenType::floatType, "64", getPos(3));
  }
  else if (wordValue == "f80") {
    return Token::valued(TokenType::floatType, "80", getPos(3));
  }
  else if (wordValue == "f128ppc") {
    return Token::valued(TokenType::floatType, "128ppc", getPos(7));
  }
  else if (wordValue == "f128") {
    return Token::valued(TokenType::floatType, "128", getPos(4));
  }
  else {
    return Token::valued(TokenType::identifier, wordValue, getPos(wordValue.length()));
  }
}

void Lexer::throwError(const String& message) {
  std::cout << colors::highIntensityBackground::red << " lexer error " << colors::reset << " " << colors::bold::red
            << message << colors::reset << " | " << colors::underline::green
            << fs::absolute(filePath).lexically_normal().string() << ":" << lineNumber << ":" << characterNumber
            << colors::reset << "\n";
  exit(1);
}

} // namespace qat::lexer