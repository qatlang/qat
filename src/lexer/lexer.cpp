/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#include "lexer.hpp"

bool qat::lexer::Lexer::emit_tokens = false;
bool qat::lexer::Lexer::show_report = false;

void qat::lexer::Lexer::read(std::string context) {
  try {
    if (file.eof()) {
      prev = curr;
      curr = -1;
    } else {
      prev = curr;
      file.get(curr);
      char_num++;
      total_char_count++;
    }
    if (curr == '\n') {
      line_num++;
      char_num = 0;
    } else if (curr == '\r') {
      line_num++;
      prev = curr;
      file.get(curr);
      total_char_count++;
      char_num = (curr == '\n') ? 0 : 1;
    }
  } catch (std::exception &err) {
    throwError(std::string("Lexer failed while reading the file.\nError: ") +
               err.what());
  }
}

qat::utils::FilePlacement
qat::lexer::Lexer::getPosition(unsigned long long length) {
  qat::utils::Position start = {line_num,
                                ((char_num > 0) ? (char_num - 1) : 0)};
  qat::utils::Position end = {line_num, start.character + length};
  return qat::utils::FilePlacement(fsexp::path(filePath), start, end);
}

std::vector<qat::lexer::Token> *qat::lexer::Lexer::analyse() {
  file.open(filePath, std::ios::in);
  auto lexerStartTime = std::chrono::high_resolution_clock::now();
  tokens.push_back(
      Token::valued(TokenType::startOfFile, filePath, this->getPosition(0)));
  read("start");
  while (!file.eof()) {
    tokens.push_back(tokeniser());
  }
  file.close();
  if (tokens.back().type != TokenType::endOfFile) {
    tokens.push_back(
        Token::valued(TokenType::endOfFile, filePath, this->getPosition(0)));
  }
  std::chrono::nanoseconds lexerElapsed =
      std::chrono::high_resolution_clock::now() - lexerStartTime;
  timeInNS = lexerElapsed.count();
  return &tokens;
}

void qat::lexer::Lexer::changeFile(std::string newFilePath) {
  tokens.clear();
  filePath = newFilePath;
  prev_ctx = "";
  prev = -1;
  curr = -1;
  line_num = 1;
  char_num = 0;
  timeInNS = 0;
  total_char_count = 0;
}

qat::lexer::Token qat::lexer::Lexer::tokeniser() {
  if (buffer.size() != 0) {
    qat::lexer::Token token = buffer.back();
    buffer.pop_back();
    return token;
  }
  if (file.eof()) {
    return Token::valued(TokenType::endOfFile, filePath, this->getPosition(0));
  }
  if (prev == '\r') {
    prev_ctx = "whitespace";
    read(prev_ctx);
    return tokeniser();
  }
  switch (curr) {
  case ' ':
  case '\n':
  case '\r':
  case '\t': {
    prev_ctx = "whitespace";
    read(prev_ctx);
    return tokeniser();
  }
  case '.': {
    prev_ctx = "stop";
    read(prev_ctx);
    return Token::normal(TokenType::stop, this->getPosition(1));
  }
  case ',': {
    prev_ctx = "separator";
    read(prev_ctx);
    return Token::normal(TokenType::separator, this->getPosition(1));
  }
  case '(': {
    prev_ctx = "paranthesisOpen";
    read(prev_ctx);
    return Token::normal(TokenType::paranthesisOpen, this->getPosition(1));
  }
  case ')': {
    prev_ctx = "paranthesisClose";
    read(prev_ctx);
    return Token::normal(TokenType::paranthesisClose, this->getPosition(1));
  }
  case '[': {
    prev_ctx = "bracketOpen";
    read(prev_ctx);
    return Token::normal(TokenType::bracketOpen, this->getPosition(1));
  }
  case ']': {
    prev_ctx = "bracketClose";
    read(prev_ctx);
    return Token::normal(TokenType::bracketClose, this->getPosition(1));
  }
  case '{': {
    prev_ctx = "curlybraceOpen";
    read(prev_ctx);
    return Token::normal(TokenType::curlybraceOpen, this->getPosition(1));
  }
  case '}': {
    prev_ctx = "curlybraceClose";
    read(prev_ctx);
    return Token::normal(TokenType::curlybraceClose, this->getPosition(1));
  }
  case '@': {
    prev_ctx = "at";
    read(prev_ctx);
    return Token::normal(TokenType::at, this->getPosition(1));
  }
  case '^': {
    prev_ctx = "pointerAccess";
    read(prev_ctx);
    return Token::normal(TokenType::pointerAccess, this->getPosition(1));
  }
  case ':': {
    read("colon");
    if (curr == ':') {
      prev_ctx = "givenTypeSeparator";
      read(prev_ctx);
      return Token::normal(TokenType::givenTypeSeparator, this->getPosition(2));
    } else {
      prev_ctx = "colon";
      return Token::normal(TokenType::colon, this->getPosition(1));
    }
  }
  case '#': {
    prev_ctx = "hashtag";
    read(prev_ctx);
    return Token::normal(TokenType::hashtag, this->getPosition(1));
  }
  case '/': {
    prev_ctx = "operator";
    std::string value = "/";
    read(prev_ctx);
    if (curr == '*') {
      bool star = false;
      prev_ctx = "multiLineComment";
      read(prev_ctx);
      while ((star ? (curr != '/') : true) && !file.eof()) {
        if (star) {
          star = false;
        }
        if (curr == '*') {
          star = true;
        }
        read(prev_ctx);
      }
      if (file.eof()) {
        return Token::valued(TokenType::endOfFile, filePath,
                             this->getPosition(0));
      } else {
        read(prev_ctx);
        return tokeniser();
      }
    } else if (curr == '/') {
      prev_ctx = "singleLineComment";
      while ((curr != '\n' && prev != '\r') && !file.eof()) {
        read(prev_ctx);
      }
      if (file.eof()) {
        return Token::valued(TokenType::endOfFile, filePath,
                             this->getPosition(0));
      } else {
        read(prev_ctx);
        return tokeniser();
      }
    } else {
      return Token::valued(TokenType::binaryOperator, value,
                           this->getPosition(1));
    }
  }
  case '!': {
    prev_ctx = "operator";
    read(prev_ctx);
    if (curr == '=') {
      read(prev_ctx);
      return Token::valued(TokenType::assignedBinaryOperator,
                           "!=", this->getPosition(2));
    } else {
      return Token::valued(TokenType::unaryOperatorLeft, "!",
                           this->getPosition(1));
    }
  }
  case '~': {
    prev_ctx = "bitwiseNot";
    read(prev_ctx);
    if (curr == '=') {
      read(prev_ctx);
      return Token::valued(TokenType::assignedBinaryOperator,
                           "~=", this->getPosition(2));
    } else {
      return Token::valued(TokenType::binaryOperator, "~",
                           this->getPosition(1));
    }
  }
  case '&': {
    prev_ctx = "bitwiseAnd";
    read(prev_ctx);
    if (curr == '=') {
      read(prev_ctx);
      return Token::valued(TokenType::assignedBinaryOperator,
                           "&=", this->getPosition(2));
    } else {
      return Token::valued(TokenType::binaryOperator, "&",
                           this->getPosition(1));
    }
  }
  case '|': {
    prev_ctx = "bitwiseOr";
    read(prev_ctx);
    if (curr == '=') {
      read(prev_ctx);
      return Token::valued(TokenType::assignedBinaryOperator,
                           "|=", this->getPosition(2));
    } else {
      return Token::valued(TokenType::binaryOperator, "|",
                           this->getPosition(1));
    }
  }
  case '+':
  case '-':
  case '%':
  case '*':
  case '<':
  case '>': {
    prev_ctx = "operator";
    std::string opr = "";
    opr += curr;
    read(prev_ctx);
    if ((curr == '+' && opr == "+") || (curr == '-' && opr == "-")) {
      opr += curr;
      read(prev_ctx);
      const std::string identifier =
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
      return Token::valued((identifier.find(curr) != std::string::npos
                                ? TokenType::unaryOperatorLeft
                                : TokenType::unaryOperatorRight),
                           opr, this->getPosition(2));
    } else if (curr == '=' && opr != "<" && opr != ">") {
      opr += curr;
      read(prev_ctx);
      return Token::valued(TokenType::assignedBinaryOperator, opr,
                           this->getPosition(2));
    } else if ((curr == '<' && opr == "<") || (curr == '>' && opr == ">")) {
      opr += curr;
      read(prev_ctx);
      return Token::valued(TokenType::binaryOperator, opr,
                           this->getPosition(2));
    } else if (curr == '~' && opr == "<") {
      prev_ctx = "variationMarker";
      read(prev_ctx);
      return Token::normal(TokenType::variationMarker, this->getPosition(2));
    } else if (opr == "<") {
      return Token::normal(TokenType::lesserThan, this->getPosition(1));
    } else if (opr == ">") {
      if (curr == '-') {
        prev_ctx = "lambda";
        read(prev_ctx);
        return Token::normal(TokenType::lambda, this->getPosition(2));
      }
      return Token::normal(TokenType::greaterThan, this->getPosition(1));
    }
    return Token::valued(TokenType::binaryOperator, opr, this->getPosition(1));
  }
  case '=': {
    read("assignment");
    if (curr == '=') {
      prev_ctx = "operator";
      read(prev_ctx);
      return Token::valued(TokenType::binaryOperator,
                           "==", this->getPosition(2));
    } else if (curr == '>') {
      prev_ctx = "singleStatementMarker";
      read(prev_ctx);
      return Token::normal(TokenType::singleStatementMarker,
                           this->getPosition(2));
    } else {
      prev_ctx = "assignment";
      return Token::normal(TokenType::assignment, this->getPosition(1));
    }
  }
  case '\'': {
    read(prev_ctx);
    if (curr == '\'') {
      prev_ctx = "self";
      read(prev_ctx);
      return Token::normal(TokenType::self, this->getPosition(2));
    } else {
      prev_ctx = "child";
    }
    return Token::normal(TokenType::child, this->getPosition(1));
  }
  case '"': {
    bool escape = false;
    read("string");
    std::string str_val = "";
    while (escape ? !file.eof() : (curr != '"' && !file.eof())) {
      if (escape) {
        escape = false;
        if (curr == '\'')
          str_val += '\'';
        else if (curr == '"')
          str_val += '"';
        else if (curr == '\\')
          str_val += "\\\\";
        else if (curr == 'n')
          str_val += "\n";
        else if (curr == '?')
          str_val += "\?";
        else if (curr == 'b')
          str_val += "\b";
        else if (curr == 'a')
          str_val += "\a";
        else if (curr == 'f')
          str_val += "\f";
        else if (curr == 'r')
          str_val += "\r";
        else if (curr == 't')
          str_val += "\t";
        else if (curr == 'v')
          str_val += "\v";
        read("string");
      }
      if (curr == '\\' && prev != '\\') {
        escape = true;
      } else {
        str_val += curr;
      }
      read("string");
    }
    prev_ctx = "literal";
    read(prev_ctx);
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
    std::string num_val = "";
    bool is_float = false;
    const std::string digits = "0123456789";
    while (((digits.find(curr, 0) != std::string::npos) ||
            (is_float ? false : (curr == '.'))) &&
           !file.eof()) {
      if (curr == '.') {
        read("integer");
        if (digits.find(curr, 0) != std::string::npos) {
          is_float = true;
          num_val += '.';
        } else {
          /// This is in the reverse order since the last element is returned
          /// first
          buffer.push_back(
              Token::normal(TokenType::stop, this->getPosition(1)));
          buffer.push_back(Token::valued(TokenType::IntegerLiteral, num_val,
                                         this->getPosition(num_val.length())));
          return tokeniser();
        }
      }
      num_val += curr;
      read(is_float ? "float" : "integer");
    }
    prev_ctx = "literal";
    return Token::valued(is_float ? TokenType::FloatLiteral
                                  : TokenType::IntegerLiteral,
                         num_val, this->getPosition(num_val.length()) //
    );
  }
  case -1: {
    return Token::valued(TokenType::endOfFile, filePath, this->getPosition(0));
  }
  default: {
    const std::string alphabets =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if ((alphabets.find(curr, 0) != std::string::npos) || curr == '_') {
      const std::string digits = "0123456789";
      std::string value = "";
      while ((alphabets.find(curr, 0) != std::string::npos) ||
             (digits.find(curr, 0) != std::string::npos) ||
             (curr == '_') && !file.eof()) {
        value += curr;
        read("identifier");
      }
      prev_ctx = "keyword";
      if (value == "null")
        return Token::normal(TokenType::null, this->getPosition(4));
      else if (value == "bring")
        return Token::normal(TokenType::bring, this->getPosition(5));
      else if (value == "pub")
        return Token::normal(TokenType::Public, this->getPosition(3));
      else if (value == "let")
        return Token::normal(TokenType::let, this->getPosition(3));
      else if (value == "pvt")
        return Token::normal(TokenType::Private, this->getPosition(3));
      else if (value == "void")
        return Token::normal(TokenType::Void, this->getPosition(4));
      else if (value == "type")
        return Token::normal(TokenType::Type, this->getPosition(4));
      else if (value == "model")
        return Token::normal(TokenType::model, this->getPosition(5));
      else if (value == "const")
        return Token::normal(TokenType::constant, this->getPosition(5));
      else if (value == "space")
        return Token::normal(TokenType::space, this->getPosition(5));
      else if (value == "from")
        return Token::normal(TokenType::from, this->getPosition(4));
      else if (value == "to")
        return Token::normal(TokenType::to, this->getPosition(2));
      else if (value == "true")
        return Token::normal(TokenType::TRUE, this->getPosition(4));
      else if (value == "false")
        return Token::normal(TokenType::FALSE, this->getPosition(5));
      else if (value == "say")
        return Token::normal(TokenType::say, this->getPosition(3));
      else if (value == "as")
        return Token::normal(TokenType::as, this->getPosition(2));
      else if (value == "file")
        return Token::normal(TokenType::file, this->getPosition(4));
      else if (value == "lib")
        return Token::normal(TokenType::Library, this->getPosition(3));
      else if (value == "ref")
        return Token::normal(TokenType::reference, this->getPosition(3));
      else if (value == "bool")
        return Token::normal(TokenType::Bool, this->getPosition(4));
      else if (value == "size")
        return Token::normal(TokenType::size, this->getPosition(4));
      else if (value.substr(0, 1) == "i" &&
               ((value.length() > 1)
                    ? qat::utils::isInteger(value.substr(1, value.length() - 1))
                    : false)) {
        return Token::valued(TokenType::Integer,
                             value.substr(1, value.length() - 1),
                             this->getPosition(value.length()));
      } else if (value == "fbrain")
        return Token::valued(TokenType::Float, "brain", this->getPosition(6));
      else if (value == "fhalf")
        return Token::valued(TokenType::Float, "half", this->getPosition(5));
      else if (value == "f32")
        return Token::valued(TokenType::Float, "32", this->getPosition(3));
      else if (value == "f64")
        return Token::valued(TokenType::Float, "64", this->getPosition(3));
      else if (value == "f80")
        return Token::valued(TokenType::Float, "80", this->getPosition(3));
      else if (value == "f128ppc")
        return Token::valued(TokenType::Float, "128ppc", this->getPosition(7));
      else if (value == "f128")
        return Token::valued(TokenType::Float, "128", this->getPosition(4));
      else if (value == "str")
        return Token::normal(TokenType::String, this->getPosition(3));
      else if (value == "alias")
        return Token::normal(TokenType::alias, this->getPosition(5));
      else if (value == "for")
        return Token::normal(TokenType::For, this->getPosition(3));
      else if (value == "give")
        return Token::normal(TokenType::give, this->getPosition(4));
      else if (value == "expose")
        return Token::normal(TokenType::expose, this->getPosition(6));
      else if (value == "var")
        return Token::normal(TokenType::var, this->getPosition(3));
      else if (value == "if")
        return Token::normal(TokenType::If, this->getPosition(2));
      else if (value == "else")
        return Token::normal(TokenType::Else, this->getPosition(4));
      else {
        prev_ctx = "identifier";
        return Token::valued(TokenType::identifier, value,
                             this->getPosition(value.length()));
      }
    } else {
      throwError("Unrecognised character found: " + std::string(1, curr));
      return Token::normal(TokenType::endOfFile, this->getPosition(0));
    }
  }
  }
}

void qat::lexer::Lexer::printStatus() {
  if (Lexer::show_report) {
    double time_val = 0;
    std::string time_unit = "";

    if (timeInNS < 1000000) {
      time_val = ((double)timeInNS) / 1000;
      time_unit = " microseconds \n";
    } else if (timeInNS < 1000000) {
      time_val = ((double)timeInNS) / 1000000;
      time_unit = " milliseconds \n";
    } else {
      time_val = ((double)timeInNS) / 1000000000;
      time_unit = " seconds \n";
    }
    std::cout << colors::cyan << "[ LEXER ] " << colors::bold::green << filePath
              << colors::reset << "\n   " << --line_num << " lines & "
              << --total_char_count << " characters in " << colors::bold_
              << time_val << time_unit << colors::reset;
  }
  if (Lexer::emit_tokens) {
    std::cout << "\nAnalysed Tokens \n\n";
    for (std::size_t i = 0; i < tokens.size(); i++) {
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
      case TokenType::at:
        std::cout << " @ ";
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
      case TokenType::file:
        std::cout << " file ";
        break;
      case TokenType::Float:
        std::cout << " f" << tokens.at(i).value << " ";
        break;
      case TokenType::FloatLiteral:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::function:
        std::cout << " fn ";
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
        std::cout << " :: ";
        break;
      case TokenType::greaterThan:
        std::cout << " > ";
        break;
      case TokenType::hashtag:
        std::cout << " # ";
        break;
      case TokenType::binaryOperator:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::identifier:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::let:
        std::cout << " let ";
        break;
      case TokenType::Integer:
        std::cout << " i" << tokens.at(i).value << " ";
        break;
      case TokenType::IntegerLiteral:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::lambda:
        std::cout << " >- ";
        break;
      case TokenType::lesserThan:
        std::cout << " < ";
        break;
      case TokenType::Library:
        std::cout << " lib ";
        break;
      case TokenType::var:
        std::cout << " var ";
        break;
      case TokenType::Type:
        std::cout << " obj ";
        break;
      case TokenType::pointerAccess:
        std::cout << " ^ ";
        break;
      case TokenType::paranthesisClose:
        std::cout << " ) ";
        break;
      case TokenType::paranthesisOpen:
        std::cout << " ( ";
        break;
      case TokenType::Private:
        std::cout << " pvt ";
        break;
      case TokenType::Public:
        std::cout << " pub ";
        break;
      case TokenType::reference:
        std::cout << " ref ";
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
      case TokenType::space:
        std::cout << " space ";
        break;
      case TokenType::startOfFile:
        std::cout << "STARTOFFILE  -  " << filePath << "\n";
        break;
      case TokenType::stop:
        std::cout << " .\n";
        break;
      case TokenType::String:
        std::cout << " string ";
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
      case TokenType::Void:
        std::cout << " void ";
        break;
      case TokenType::expose:
        std::cout << " expose ";
        break;
      default:
        std::cout << " UNKNOWN ";
        break;
      }
    }
    std::cout << "\n" << std::endl;
  }
}

void qat::lexer::Lexer::throwError(std::string message) {
  std::cout << colors::red << "[ LEXER ERROR ] " << colors::bold::green
            << filePath << ":" << line_num << ":" << char_num << colors::reset
            << "\n";
  std::cout << "   " << message << "\n";
  exit(0);
}