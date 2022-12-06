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

utils::FileRange Lexer::getPosition(u64 length) {
  utils::FilePos end = {lineNumber, characterNumber};
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
      return Token::normal(TokenType::bracketOpen, this->getPosition(1));
    }
    case ']': {
      read();
      return Token::normal(TokenType::bracketClose, this->getPosition(1));
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
    case '#': {
      read();
      return Token::normal(TokenType::pointerType, this->getPosition(1));
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
      } else {
        return Token::valued(TokenType::binaryOperator, "&", this->getPosition(1));
      }
    }
    case '|': {
      read();
      if (current == '=') {
        read();
        return Token::valued(TokenType::assignedBinaryOperator, "|=", this->getPosition(2));
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
      if ((current == '>') && (genericStartCount > 0)) {
        read();
        genericStartCount--;
        return Token::normal(TokenType::templateTypeEnd, this->getPosition(1));
      }
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
        std::cout << "Binary operator found " << operatorValue << "\n";
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
      if (current == '<') {
        read();
        genericStartCount++;
        return Token::normal(TokenType::templateTypeStart, this->getPosition(2));
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

Token Lexer::wordToToken(const String& value, Lexer* lexInst) {
  auto getPos = [&](usize len) {
    if (lexInst) {
      return lexInst->getPosition(len);
    } else {
      return utils::FileRange("", {0u, 0u}, {0u, 0u});
    }
  };

  // NOLINTBEGIN(readability-magic-numbers)
  if (value == "null") {
    return Token::normal(TokenType::null, getPos(4));
  } else if (value == "bring") {
    return Token::normal(TokenType::bring, getPos(5));
  } else if (value == "pub") {
    return Token::normal(TokenType::Public, getPos(3));
  } else if (value == "new") {
    return Token::normal(TokenType::New, getPos(3));
  } else if (value == "self") {
    return Token::normal(TokenType::self, getPos(3));
  } else if (value == "void") {
    return Token::normal(TokenType::voidType, getPos(4));
  } else if (value == "type") {
    return Token::normal(TokenType::Type, getPos(4));
  } else if (value == "model") {
    return Token::normal(TokenType::model, getPos(5));
  } else if (value == "const") {
    return Token::normal(TokenType::constant, getPos(5));
  } else if (value == "box") {
    return Token::normal(TokenType::box, getPos(5));
  } else if (value == "from") {
    return Token::normal(TokenType::from, getPos(4));
  } else if (value == "to") {
    return Token::normal(TokenType::to, getPos(2));
  } else if (value == "true") {
    return Token::normal(TokenType::TRUE, getPos(4));
  } else if (value == "false") {
    return Token::normal(TokenType::FALSE, getPos(5));
  } else if (value == "say") {
    return Token::normal(TokenType::say, getPos(3));
  } else if (value == "as") {
    return Token::normal(TokenType::as, getPos(2));
  } else if (value == "lib") {
    return Token::normal(TokenType::lib, getPos(3));
  } else if (value == "cstring") {
    return Token::normal(TokenType::cstringType, getPos(7));
  } else if (value == "await") {
    return Token::normal(TokenType::Await, getPos(5));
  } else if (value == "default") {
    return Token::normal(TokenType::Default, getPos(7));
  } else if (value == "static") {
    return Token::normal(TokenType::Static, getPos(6));
  } else if (value == "async") {
    return Token::normal(TokenType::Async, getPos(5));
  } else if (value == "sizeOf") {
    return Token::normal(TokenType::sizeOf, getPos(6));
  } else if (value == "variadic") {
    return Token::normal(TokenType::variadic, getPos(8));
  } else if (value == "loop") {
    return Token::normal(TokenType::loop, getPos(4));
  } else if (value == "while") {
    return Token::normal(TokenType::While, getPos(5));
  } else if (value == "over") {
    return Token::normal(TokenType::over, getPos(4));
  } else if (value == "heap") {
    return Token::normal(TokenType::heap, getPos(4));
  } else if (value == "operator") {
    return Token::normal(TokenType::Operator, getPos(8));
  } else if (value == "mix") {
    return Token::normal(TokenType::mix, getPos(5));
  } else if (value == "match") {
    return Token::normal(TokenType::match, getPos(5));
  } else if (value == "alias") {
    return Token::normal(TokenType::alias, getPos(5));
  } else if (value == "copy") {
    return Token::normal(TokenType::copy, getPos(4));
  } else if (value == "move") {
    return Token::normal(TokenType::move, getPos(4));
  } else if (value == "usize") {
    return Token::valued(TokenType::unsignedIntegerType, "usize", getPos(5));
  } else if (value.substr(0, 1) == "u" &&
             ((value.length() > 1) ? utils::isInteger(value.substr(1, value.length() - 1)) : false)) {
    return Token::valued(TokenType::unsignedIntegerType, value.substr(1, value.length() - 1), getPos(value.length()));
  } else if (value.substr(0, 1) == "i" &&
             ((value.length() > 1) ? utils::isInteger(value.substr(1, value.length() - 1)) : false)) {
    return Token::valued(TokenType::integerType, value.substr(1, value.length() - 1), getPos(value.length()));
  } else if (value == "fbrain") {
    return Token::valued(TokenType::floatType, "brain", getPos(6));
  } else if (value == "fhalf") {
    return Token::valued(TokenType::floatType, "half", getPos(5));
  } else if (value == "f32") {
    return Token::valued(TokenType::floatType, "32", getPos(3));
  } else if (value == "f64") {
    return Token::valued(TokenType::floatType, "64", getPos(3));
  } else if (value == "f80") {
    return Token::valued(TokenType::floatType, "80", getPos(3));
  } else if (value == "f128ppc") {
    return Token::valued(TokenType::floatType, "128ppc", getPos(7));
  } else if (value == "f128") {
    return Token::valued(TokenType::floatType, "128", getPos(4));
  } else if (value == "str") {
    return Token::normal(TokenType::stringSliceType, getPos(3));
  } else if (value == "alias") {
    return Token::normal(TokenType::alias, getPos(5));
  } else if (value == "for") {
    return Token::normal(TokenType::For, getPos(3));
  } else if (value == "give") {
    return Token::normal(TokenType::give, getPos(4));
  } else if (value == "expose") {
    return Token::normal(TokenType::expose, getPos(6));
  } else if (value == "var") {
    return Token::normal(TokenType::var, getPos(3));
  } else if (value == "if") {
    return Token::normal(TokenType::If, getPos(2));
  } else if (value == "else") {
    return Token::normal(TokenType::Else, getPos(4));
  } else if (value == "break") {
    return Token::normal(TokenType::Break, getPos(5));
  } else if (value == "continue") {
    return Token::normal(TokenType::Continue, getPos(8));
  } else if (value == "own") {
    return Token::normal(TokenType::own, getPos(3));
  } else if (value == "disown") {
    return Token::normal(TokenType::disown, getPos(6));
  } else if (value == "end") {
    return Token::normal(TokenType::end, getPos(3));
  } else if (value == "choice") {
    return Token::normal(TokenType::choice, getPos(6));
  } else if (value == "future") {
    return Token::normal(TokenType::future, getPos(6));
  } else if (value == "maybe") {
    return Token::normal(TokenType::maybe, getPos(5));
  } else if (value == "none") {
    return Token::normal(TokenType::none, getPos(4));
  } else if (value == "meta") {
    return Token::normal(TokenType::meta, getPos(4));
  } else if (value == "region") {
    return Token::normal(TokenType::region, getPos(6));
  } else {
    return Token::valued(TokenType::identifier, value, getPos(value.length()));
  }
  // NOLINTEND(readability-magic-numbers)
}

void Lexer::throwError(const String& message) {
  std::cout << colors::highIntensityBackground::red << " lexer error " << colors::reset << " " << colors::bold::red
            << message << colors::reset << " | " << colors::underline::green
            << fs::absolute(filePath).lexically_normal().string() << ":" << lineNumber << ":" << characterNumber
            << colors::reset << "\n";
  exit(1);
}

} // namespace qat::lexer