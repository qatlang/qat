#include "lexer.hpp"
#include "token_type.hpp"

#define NanosecondsInMicroseconds 1000
#define NanosecondsInMilliseconds 1000000
#define NanosecondsInSeconds      1000000000

namespace qat::lexer {

bool Lexer::emit_tokens = false;

bool Lexer::show_report = false;

Lexer::Lexer() {
  auto *cfg          = cli::Config::get();
  Lexer::emit_tokens = cfg->shouldLexerEmitTokens();
  Lexer::show_report = cfg->shouldShowReport();
}

Vec<Token> &Lexer::get_tokens() { return tokens; }

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
      char_num++;
      total_char_count++;
    }
    if (current == '\n') {
      line_num++;
      char_num = 0;
      content.push_back("");
    } else if (current == '\r') {
      prev = current;
      file.get(current);
      if (current != -1) {
        if (current == '\n') {
          line_num++;
          char_num = 1;
          total_char_count++;
          content.push_back("");
        } else {
          char_num++;
          total_char_count++;
          (content.back() += "\r") += current;
        }
      }
      total_char_count++;
      char_num = (current == '\n') ? 0 : 1;
    }
  } catch (std::exception &err) {
    throw_error(String("Lexer failed while reading the file. Error: ") +
                err.what());
  }
}

utils::FileRange Lexer::getPosition(u64 length) {
  utils::FilePos end   = {line_num, char_num};
  utils::FilePos start = {line_num, end.character - length};
  return {fs::path(filePath), start, end};
}

void Lexer::analyse() {
  file.open(filePath, std::ios::in);
  auto lexer_start = std::chrono::high_resolution_clock::now();
  tokens.push_back(Token::valued(TokenType::startOfFile, filePath.string(),
                                 this->getPosition(0)));
  read();
  while (!file.eof()) {
    tokens.push_back(tokeniser());
  }
  file.close();
  if (tokens.back().type != TokenType::endOfFile) {
    tokens.push_back(Token::valued(TokenType::endOfFile, filePath.string(),
                                   this->getPosition(0)));
  }
  std::chrono::nanoseconds lexer_elapsed =
      std::chrono::high_resolution_clock::now() - lexer_start;
  timeInNS = lexer_elapsed.count();
  printStatus();
}

void Lexer::changeFile(fs::path newFilePath) {
  tokens.clear();
  content.clear();
  filePath         = std::move(newFilePath);
  prev_ctx         = "";
  prev             = -1;
  current          = -1;
  line_num         = 1;
  char_num         = 0;
  timeInNS         = 0;
  total_char_count = 0;
}

Token Lexer::tokeniser() {
  if (!buffer.empty()) {
    Token token = buffer.back();
    buffer.pop_back();
    return token;
  }
  if (file.eof()) {
    return Token::valued(TokenType::endOfFile, filePath.string(),
                         this->getPosition(0));
  }
  if (prev == '\r') {
    prev_ctx = "whitespace";
    read();
    return tokeniser();
  }
  switch (current) {
  case ' ':
  case '\n':
  case '\r':
  case '\t': {
    prev_ctx = "whitespace";
    read();
    return tokeniser();
  }
  case '.': {
    prev_ctx = "stop";
    read();
    if (current == '.') {
      prev_ctx = "super";
      read();
      return Token::normal(TokenType::super, this->getPosition(2));
    }
    return Token::normal(TokenType::stop, this->getPosition(1));
  }
  case ',': {
    prev_ctx = "separator";
    read();
    return Token::normal(TokenType::separator, this->getPosition(1));
  }
  case '(': {
    prev_ctx = "parenthesisOpen";
    read();
    return Token::normal(TokenType::parenthesisOpen, this->getPosition(1));
  }
  case ')': {
    prev_ctx = "parenthesisClose";
    read();
    return Token::normal(TokenType::parenthesisClose, this->getPosition(1));
  }
  case '[': {
    prev_ctx = "bracketOpen";
    read();
    return Token::normal(TokenType::bracketOpen, this->getPosition(1));
  }
  case ']': {
    prev_ctx = "bracketClose";
    read();
    return Token::normal(TokenType::bracketClose, this->getPosition(1));
  }
  case '{': {
    prev_ctx = "curlybraceOpen";
    read();
    return Token::normal(TokenType::curlybraceOpen, this->getPosition(1));
  }
  case '}': {
    prev_ctx = "curlybraceClose";
    read();
    return Token::normal(TokenType::curlybraceClose, this->getPosition(1));
  }
  case '@': {
    prev_ctx = "reference";
    read();
    return Token::normal(TokenType::referenceType, this->getPosition(1));
  }
  case '^': {
    prev_ctx = "operator";
    read();
    if (current == '=') {
      prev_ctx = "assignedBinaryOperator";
      read();
      return Token::valued(TokenType::assignedBinaryOperator,
                           "^=", this->getPosition(2));
    }
    return Token::valued(TokenType::binaryOperator, "^", this->getPosition(1));
  }
  case ':': {
    read();
    if (current == '<') {
      prev_ctx = "templateTypeStart";
      read();
      template_type_start_count++;
      return Token::normal(TokenType::templateTypeStart, this->getPosition(2));
    } else if (current == '=') {
      prev_ctx = "associatedAssignment";
      read();
      return Token::normal(TokenType::associatedAssignment,
                           this->getPosition(2));
    } else {
      prev_ctx = "colon";
      return Token::normal(TokenType::colon, this->getPosition(1));
    }
  }
  case '#': {
    prev_ctx = "pointerType";
    read();
    return Token::normal(TokenType::pointerType, this->getPosition(1));
  }
  case '/': {
    prev_ctx     = "operator";
    String value = "/";
    read();
    if (current == '*') {
      bool star = false;
      prev_ctx  = "multiLineComment";
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
        return Token::valued(TokenType::endOfFile, filePath.string(),
                             this->getPosition(0));
      } else {
        read();
        return tokeniser();
      }
    } else if (current == '/') {
      prev_ctx = "singleLineComment";
      while ((current != '\n' && prev != '\r') && !file.eof()) {
        read();
      }
      if (file.eof()) {
        return Token::valued(TokenType::endOfFile, filePath.string(),
                             this->getPosition(0));
      } else {
        read();
        return tokeniser();
      }
    } else {
      return Token::valued(TokenType::binaryOperator, value,
                           this->getPosition(1));
    }
  }
  case '!': {
    prev_ctx = "operator";
    read();
    if (current == '=') {
      read();
      return Token::valued(TokenType::binaryOperator,
                           "!=", this->getPosition(2));
    } else {
      return Token::valued(TokenType::unaryOperatorLeft, "!",
                           this->getPosition(1));
    }
  }
  case '~': {
    prev_ctx = "bitwiseNot";
    read();
    if (current == '=') {
      read();
      return Token::valued(TokenType::assignedBinaryOperator,
                           "~=", this->getPosition(2));
    } else {
      return Token::valued(TokenType::binaryOperator, "~",
                           this->getPosition(1));
    }
  }
  case '&': {
    prev_ctx = "bitwiseAnd";
    read();
    if (current == '=') {
      read();
      return Token::valued(TokenType::assignedBinaryOperator,
                           "&=", this->getPosition(2));
    } else {
      return Token::valued(TokenType::binaryOperator, "&",
                           this->getPosition(1));
    }
  }
  case '|': {
    prev_ctx = "bitwiseOr";
    read();
    if (current == '=') {
      read();
      return Token::valued(TokenType::assignedBinaryOperator,
                           "|=", this->getPosition(2));
    } else {
      return Token::valued(TokenType::binaryOperator, "|",
                           this->getPosition(1));
    }
  }
  case '?': {
    prev_ctx = "ternary";
    read();
    if (current == '?') {
      prev_ctx = "isNullPointer";
      read();
      if (current == '=') {
        prev_ctx = "assignToNullPointer";
        read();
        return Token::normal(TokenType::assignToNullPointer,
                             this->getPosition(3));
      } else {
        return Token::normal(TokenType::isNullPointer, this->getPosition(2));
      }
    } else if (current == '!') {
      prev_ctx = "isNotNullPointer";
      read();
      if (current == '=') {
        prev_ctx = "assignToNonNullPointer";
        read();
        return Token::normal(TokenType::assignToNonNullPointer,
                             this->getPosition(3));
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
    if ((current == '>') && (template_type_start_count > 0)) {
      prev_ctx = "templateTypeEnd";
      read();
      template_type_start_count--;
      return Token::normal(TokenType::templateTypeEnd, this->getPosition(1));
    }
    prev_ctx = "operator";
    String operatorValue;
    operatorValue += current;
    read();
    if ((current == '+' && operatorValue == "+") ||
        (current == '-' && operatorValue == "-")) {
      operatorValue += current;
      read();
      const String identifierStart =
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
      return Token::valued((identifierStart.find(current) != String::npos
                                ? TokenType::unaryOperatorLeft
                                : TokenType::unaryOperatorRight),
                           operatorValue, this->getPosition(2));
    } else if (current == '=' && operatorValue != "<" && operatorValue != ">") {
      operatorValue += current;
      read();
      return Token::valued(TokenType::assignedBinaryOperator, operatorValue,
                           this->getPosition(2));
    } else if (current == '=' &&
               (operatorValue == "<" || operatorValue == ">")) {
      operatorValue += current;
      read();
      std::cout << "Binary operator found " << operatorValue << "\n";
      return Token::valued(TokenType::binaryOperator, operatorValue,
                           this->getPosition(2));
    } else if ((current == '<' && operatorValue == "<") ||
               (current == '>' && operatorValue == ">")) {
      operatorValue += current;
      read();
      return Token::valued(TokenType::binaryOperator, operatorValue,
                           this->getPosition(2));
    } else if (current == '~' && operatorValue == "<") {
      prev_ctx = "variationMarker";
      read();
      return Token::normal(TokenType::variationMarker, this->getPosition(2));
    } else if (current == '>' && operatorValue == "-") {
      prev_ctx = "givenTypeSeparator";
      read();
      return Token::normal(TokenType::givenTypeSeparator, this->getPosition(2));
    } else if (operatorValue == "<") {
      return Token::valued(TokenType::binaryOperator, "<",
                           this->getPosition(1));
    } else if (operatorValue == ">") {
      if (current == '-') {
        prev_ctx = "pointerAccess";
        read();
        return Token::normal(TokenType::pointerAccess, this->getPosition(2));
      }
      return Token::valued(TokenType::binaryOperator, ">",
                           this->getPosition(1));
    }
    return Token::valued(TokenType::binaryOperator, operatorValue,
                         this->getPosition(1));
  }
  case '=': {
    read();
    if (current == '=') {
      prev_ctx = "operator";
      read();
      return Token::valued(TokenType::binaryOperator,
                           "==", this->getPosition(2));
    } else if (current == '>') {
      prev_ctx = "singleStatementMarker";
      read();
      return Token::normal(TokenType::singleStatementMarker,
                           this->getPosition(2));
    } else {
      prev_ctx = "assignment";
      return Token::normal(TokenType::assignment, this->getPosition(1));
    }
  }
  case '\'': {
    read();
    if (current == '\'') {
      prev_ctx = "self";
      read();
      return Token::normal(TokenType::self, this->getPosition(2));
    } else {
      prev_ctx = "child";
      return Token::normal(TokenType::child, this->getPosition(1));
    }
  }
  case ';': {
    prev_ctx = "semiColon";
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
    prev_ctx = "literal";
    read();
    return Token::valued(TokenType::StringLiteral, str_val,
                         this->getPosition(str_val.length() + 2));
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
    while (((digits.find(current, 0) != String::npos) ||
            (!is_float && (current == '.')) ||
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
          buffer.push_back(
              Token::normal(TokenType::stop, this->getPosition(1)));
          auto fileRange = this->getPosition(numVal.length() + 1);
          if (fileRange.end.character > 0) {
            fileRange.end.character--;
          }
          buffer.push_back(
              Token::valued(TokenType::integerLiteral, numVal, fileRange));
          return tokeniser();
        }
      } else if (current == '_') {
        found_spec = true;
        String bitString;
        read();
        if (current == 'u') {
          read();
          while (digits.find(current) != String::npos) {
            bitString += current;
            read();
          }
          auto resString = numVal;
          resString.append("_u").append(bitString);
          return Token::valued(TokenType::unsignedLiteral, resString,
                               this->getPosition(resString.length()));
        } else if (current == 'i') {
          read();
          while (digits.find(current) != String::npos) {
            bitString += current;
            read();
          }
          auto resString = numVal;
          resString.append("_i").append(bitString);
          return Token::valued(TokenType::integerLiteral, resString,
                               this->getPosition(resString.length()));
        } else if (current == 'f') {
          read();
          while ((digits.find(current) != String::npos) ||
                 (alphabets.find(current) != String::npos)) {
            bitString += current;
            read();
          }
          auto resString = numVal;
          resString.append("_f").append(bitString);
          return Token::valued(TokenType::floatLiteral, resString,
                               this->getPosition(resString.length()));
        } else {
          throw_error("Invalid integer literal suffix");
        }
      }
      numVal += current;
      read();
    }
    prev_ctx = "literal";
    return Token::valued(is_float ? TokenType::floatLiteral
                                  : TokenType::integerLiteral,
                         numVal, this->getPosition(numVal.length()) //
    );
  }
  case -1: {
    return Token::valued(TokenType::endOfFile, filePath.string(),
                         this->getPosition(0));
  }
  default: {
    const String alphabets =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if ((alphabets.find(current, 0) != String::npos) || current == '_') {
      const String digits = "0123456789";
      String       value;
      while ((alphabets.find(current, 0) != String::npos) ||
             (digits.find(current, 0) != String::npos) ||
             (current == '_') && !file.eof()) {
        value += current;
        read();
      }
      prev_ctx = "keyword";
      if (value == "null") {
        return Token::normal(TokenType::null, this->getPosition(4));
      } else if (value == "bring") {
        return Token::normal(TokenType::bring, this->getPosition(5));
      } else if (value == "pub") {
        return Token::normal(TokenType::Public, this->getPosition(3));
      } else if (value == "let") {
        return Token::normal(TokenType::let, this->getPosition(3));
      } else if (value == "self") {
        return Token::normal(TokenType::self, this->getPosition(3));
      } else if (value == "void") {
        return Token::normal(TokenType::voidType, this->getPosition(4));
      } else if (value == "type") {
        return Token::normal(TokenType::Type, this->getPosition(4));
      } else if (value == "model") {
        return Token::normal(TokenType::model, this->getPosition(5));
      } else if (value == "const") {
        return Token::normal(TokenType::constant, this->getPosition(5));
      } else if (value == "box") {
        return Token::normal(TokenType::box, this->getPosition(5));
      } else if (value == "from") {
        return Token::normal(TokenType::from, this->getPosition(4));
      } else if (value == "to") {
        return Token::normal(TokenType::to, this->getPosition(2));
      } else if (value == "true") {
        return Token::normal(TokenType::TRUE, this->getPosition(4));
      } else if (value == "false") {
        return Token::normal(TokenType::FALSE, this->getPosition(5));
      } else if (value == "say") {
        return Token::normal(TokenType::say, this->getPosition(3));
      } else if (value == "as") {
        return Token::normal(TokenType::as, this->getPosition(2));
      } else if (value == "lib") {
        return Token::normal(TokenType::lib, this->getPosition(3));
      } else if (value == "bool") {
        return Token::normal(TokenType::boolType, this->getPosition(4));
      } else if (value == "cstring") {
        return Token::normal(TokenType::cstringType, this->getPosition(7));
      } else if (value == "await") {
        return Token::normal(TokenType::Await, this->getPosition(5));
      } else if (value == "default") {
        return Token::normal(TokenType::Default, this->getPosition(7));
      } else if (value == "static") {
        return Token::normal(TokenType::Static, this->getPosition(6));
      } else if (value == "async") {
        return Token::normal(TokenType::Async, this->getPosition(5));
      } else if (value == "sizeOf") {
        return Token::normal(TokenType::sizeOf, this->getPosition(6));
      } else if (value == "variadic") {
        return Token::normal(TokenType::variadic, this->getPosition(8));
      } else if (value == "loop") {
        return Token::normal(TokenType::loop, this->getPosition(4));
      } else if (value == "while") {
        return Token::normal(TokenType::While, this->getPosition(5));
      } else if (value == "over") {
        return Token::normal(TokenType::over, this->getPosition(4));
      } else if (value.substr(0, 1) == "u" &&
                 ((value.length() > 1)
                      ? utils::isInteger(value.substr(1, value.length() - 1))
                      : false)) {
        return Token::valued(TokenType::unsignedIntegerType,
                             value.substr(1, value.length() - 1),
                             this->getPosition(value.length()));
      } else if (value.substr(0, 1) == "i" &&
                 ((value.length() > 1)
                      ? utils::isInteger(value.substr(1, value.length() - 1))
                      : false)) {
        return Token::valued(TokenType::integerType,
                             value.substr(1, value.length() - 1),
                             this->getPosition(value.length()));
      } else if (value == "fbrain") {
        return Token::valued(TokenType::floatType, "brain",
                             this->getPosition(6));
      } else if (value == "fhalf") {
        return Token::valued(TokenType::floatType, "half",
                             this->getPosition(5));
      } else if (value == "f32") {
        return Token::valued(TokenType::floatType, "32", this->getPosition(3));
      } else if (value == "f64") {
        return Token::valued(TokenType::floatType, "64", this->getPosition(3));
      } else if (value == "f80") {
        return Token::valued(TokenType::floatType, "80", this->getPosition(3));
      } else if (value == "f128ppc") {
        return Token::valued(TokenType::floatType, "128ppc",
                             this->getPosition(7));
      } else if (value == "f128") {
        return Token::valued(TokenType::floatType, "128", this->getPosition(4));
      } else if (value == "str") {
        return Token::normal(TokenType::stringSliceType, this->getPosition(3));
      } else if (value == "alias") {
        return Token::normal(TokenType::alias, this->getPosition(5));
      } else if (value == "for") {
        return Token::normal(TokenType::For, this->getPosition(3));
      } else if (value == "give") {
        return Token::normal(TokenType::give, this->getPosition(4));
      } else if (value == "expose") {
        return Token::normal(TokenType::expose, this->getPosition(6));
      } else if (value == "var") {
        return Token::normal(TokenType::var, this->getPosition(3));
      } else if (value == "if") {
        return Token::normal(TokenType::If, this->getPosition(2));
      } else if (value == "else") {
        return Token::normal(TokenType::Else, this->getPosition(4));
      } else if (value == "break") {
        return Token::normal(TokenType::Break, this->getPosition(5));
      } else if (value == "continue") {
        return Token::normal(TokenType::Continue, this->getPosition(8));
      } else {
        prev_ctx = "identifier";
        return Token::valued(TokenType::identifier, value,
                             this->getPosition(value.length()));
      }
    } else {
      throw_error("Unrecognised character found: " + String(1, current));
      return Token::normal(TokenType::endOfFile, this->getPosition(0));
    }
  }
  }
}

void Lexer::printStatus() {
  if (Lexer::show_report) {
    double time_val;
    String time_unit;
    if (timeInNS < NanosecondsInMicroseconds) {
      time_val  = ((double)timeInNS) / NanosecondsInMicroseconds;
      time_unit = " microseconds \n";
    } else if (timeInNS < NanosecondsInMilliseconds) {
      time_val  = ((double)timeInNS) / NanosecondsInMilliseconds;
      time_unit = " milliseconds \n";
    } else {
      time_val  = ((double)timeInNS) / NanosecondsInSeconds;
      time_unit = " seconds \n";
    }
    std::cout << colors::cyan << "[ LEXER ] " << colors::bold::green << filePath
              << colors::reset << "\n   " << --line_num << " lines & "
              << --total_char_count << " characters in " << colors::bold_
              << time_val << time_unit << colors::reset;
  }
  if (Lexer::emit_tokens) {
    std::cout << "\nAnalysed Tokens \n\n";
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (usize i = 0; i < tokens.size(); i++) {
      switch (tokens.at(i).type) {
      case TokenType::alias:
        std::cout << " alias ";
        break;
      case TokenType::as:
        std::cout << " as ";
        break;
      case TokenType::assignment:
        std::cout << " = ";
        break;
      case TokenType::model:
        std::cout << " box ";
        break;
      case TokenType::bracketClose:
        std::cout << " ] ";
        break;
      case TokenType::bracketOpen:
        std::cout << " [ ";
        break;
      case TokenType::bring:
        std::cout << " bring ";
        break;
      case TokenType::child:
        std::cout << " ' ";
        break;
      case TokenType::colon:
        std::cout << " : ";
        break;
      case TokenType::constant:
        std::cout << " const ";
        break;
      case TokenType::curlybraceClose:
        std::cout << " } ";
        break;
      case TokenType::curlybraceOpen:
        std::cout << " { ";
        break;
      case TokenType::endOfFile:
        std::cout << "\nENDOFFILE  -  " << filePath << "\n";
        break;
      case TokenType::external:
        std::cout << " external ";
        break;
      case TokenType::floatType:
        std::cout << " f" << tokens.at(i).value << " ";
        break;
      case TokenType::floatLiteral:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::FALSE:
        std::cout << " false ";
        break;
      case TokenType::For:
        std::cout << " for ";
        break;
      case TokenType::from:
        std::cout << " from ";
        break;
      case TokenType::give:
        std::cout << " give ";
        break;
      case TokenType::givenTypeSeparator:
        std::cout << " -> ";
        break;
      case TokenType::pointerType:
        std::cout << " # ";
        break;
      case TokenType::binaryOperator:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::templateTypeEnd:
        std::cout << " > ";
        break;
      case TokenType::templateTypeStart:
        std::cout << " :< ";
        break;
      case TokenType::identifier:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::let:
        std::cout << " let ";
        break;
      case TokenType::variadic:
        std::cout << " variadic ";
        break;
      case TokenType::integerType:
        std::cout << " i" << tokens.at(i).value << " ";
        break;
      case TokenType::integerLiteral:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::pointerAccess:
        std::cout << " -> ";
        break;
      case TokenType::pointerVariation:
        std::cout << " ~> ";
        break;
      case TokenType::lib:
        std::cout << " lib ";
        break;
      case TokenType::var:
        std::cout << " var ";
        break;
      case TokenType::Type:
        std::cout << " obj ";
        break;
      case TokenType::null:
        std::cout << " null ";
        break;
      case TokenType::parenthesisClose:
        std::cout << " ) ";
        break;
      case TokenType::parenthesisOpen:
        std::cout << " ( ";
        break;
      case TokenType::Public:
        std::cout << " pub ";
        break;
      case TokenType::referenceType:
        std::cout << " @ ";
        break;
      case TokenType::say:
        std::cout << " say ";
        break;
      case TokenType::self:
        std::cout << " '' ";
        break;
      case TokenType::separator:
        std::cout << " , ";
        break;
      case TokenType::singleStatementMarker:
        std::cout << " => ";
        break;
      case TokenType::super:
        std::cout << " .. ";
        break;
      case TokenType::box:
        std::cout << " box ";
        break;
      case TokenType::startOfFile:
        std::cout << "STARTOFFILE  -  " << filePath << "\n";
        break;
      case TokenType::stop:
        std::cout << " .\n";
        break;
      case TokenType::stringSliceType:
        std::cout << " str ";
        break;
      case TokenType::StringLiteral:
        std::cout << " \"" << tokens.at(i).value << "\" ";
        break;
      case TokenType::to:
        std::cout << " to ";
        break;
      case TokenType::TRUE:
        std::cout << " true ";
        break;
      case TokenType::unaryOperatorLeft:
        std::cout << " " << tokens.at(i).value;
        break;
      case TokenType::unaryOperatorRight:
        std::cout << tokens.at(i).value << " ";
        break;
      case TokenType::variationMarker:
        std::cout << " <~ ";
        break;
      case TokenType::voidType:
        std::cout << " void ";
        break;
      case TokenType::expose:
        std::cout << " expose ";
        break;
      case TokenType::ternary:
        std::cout << " ? ";
        break;
      default:
        std::cout << colors::bold::red << " UNKNOWN " << colors::reset;
        break;
      }
    }
    std::cout << "\n" << std::endl;
  }
}

void Lexer::throw_error(const String &message) {
  std::cout << colors::highIntensityBackground::red << " lexer error "
            << colors::reset << " " << colors::bold::red << message
            << colors::reset << " | " << colors::underline::green << filePath
            << ":" << line_num << ":" << char_num << colors::reset << "\n";
  exit(0);
}

} // namespace qat::lexer