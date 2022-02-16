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

bool qat::lexer::Lexer::emitTokens = false;
bool qat::lexer::Lexer::showReport = false;

void qat::lexer::Lexer::readNext(std::string lexerContext) {
  try {
    if (file.eof()) {
      previous = current;
      current = -1;
    } else {
      previous = current;
      file.get(current);
      characterNumber++;
      totalCharacterCount++;
    }
    if (current == '\n') {
      lineNumber++;
      characterNumber = 0;
    } else if (current == '\r') {
      lineNumber++;
      previous = current;
      file.get(current);
      totalCharacterCount++;
      characterNumber = (current == '\n') ? 0 : 1;
    }
  } catch (...) {
    throwError("Analysis failed while reading the file");
  }
}

qat::utilities::FilePosition qat::lexer::Lexer::getPosition() {
  return qat::utilities::FilePosition(
      fsexp::path(filePath), lineNumber,
      ((characterNumber > 0) ? (characterNumber - 1) : 0));
}

std::vector<qat::lexer::Token> *qat::lexer::Lexer::analyse() {
  file.open(filePath, std::ios::in);
  auto lexerStartTime = std::chrono::high_resolution_clock::now();
  tokens.push_back(
      Token::valued(TokenType::startOfFile, filePath, this->getPosition()));
  readNext("start");
  while (!file.eof()) {
    tokens.push_back(tokeniser());
  }
  file.close();
  if (tokens.back().type != TokenType::endOfFile) {
    tokens.push_back(
        Token::valued(TokenType::endOfFile, filePath, this->getPosition()));
  }
  std::chrono::nanoseconds lexerElapsed =
      std::chrono::high_resolution_clock::now() - lexerStartTime;
  timeInNS = lexerElapsed.count();
  return &tokens;
}

void qat::lexer::Lexer::changeFile(std::string newFilePath) {
  tokens.clear();
  filePath = newFilePath;
  previousContext = "";
  previous = -1;
  current = -1;
  lineNumber = 1;
  characterNumber = 0;
  timeInNS = 0;
  totalCharacterCount = 0;
}

qat::lexer::Token qat::lexer::Lexer::tokeniser() {
  if (buffer.size() != 0) {
    qat::lexer::Token token = buffer.back();
    buffer.pop_back();
    return token;
  }
  if (file.eof()) {
    return Token::valued(TokenType::endOfFile, filePath, this->getPosition());
  }
  if (previous == '\r') {
    previousContext = "whitespace";
    readNext(previousContext);
    return tokeniser();
  }
  switch (current) {
  case ' ':
  case '\n':
  case '\r':
  case '\t': {
    previousContext = "whitespace";
    readNext(previousContext);
    return tokeniser();
  }
  case '.': {
    previousContext = "stop";
    readNext(previousContext);
    return Token::normal(TokenType::stop, this->getPosition());
  }
  case ',': {
    previousContext = "separator";
    readNext(previousContext);
    return Token::normal(TokenType::separator, this->getPosition());
  }
  case '(': {
    previousContext = "paranthesisOpen";
    readNext(previousContext);
    return Token::normal(TokenType::paranthesisOpen, this->getPosition());
  }
  case ')': {
    previousContext = "paranthesisClose";
    readNext(previousContext);
    return Token::normal(TokenType::paranthesisClose, this->getPosition());
  }
  case '[': {
    previousContext = "bracketOpen";
    readNext(previousContext);
    return Token::normal(TokenType::bracketOpen, this->getPosition());
  }
  case ']': {
    previousContext = "bracketClose";
    readNext(previousContext);
    return Token::normal(TokenType::bracketClose, this->getPosition());
  }
  case '{': {
    previousContext = "curlybraceOpen";
    readNext(previousContext);
    return Token::normal(TokenType::curlybraceOpen, this->getPosition());
  }
  case '}': {
    previousContext = "curlybraceClose";
    readNext(previousContext);
    return Token::normal(TokenType::curlybraceClose, this->getPosition());
  }
  case '@': {
    previousContext = "at";
    readNext(previousContext);
    return Token::normal(TokenType::at, this->getPosition());
  }
  case '^': {
    previousContext = "staticChild";
    readNext(previousContext);
    return Token::normal(TokenType::staticChild, this->getPosition());
  }
  case ':': {
    readNext("colon");
    if (current == '>') {
      previousContext = "pointerAccess";
      readNext(previousContext);
      return Token::normal(TokenType::pointerAccess, this->getPosition());
    } else if (current == '=') {
      previousContext = "inferredAssignment";
      readNext(previousContext);
      return Token::normal(TokenType::inferredDeclaration, this->getPosition());
    } else if (current == ':') {
      previousContext = "givenTypeSeparator";
      readNext(previousContext);
      return Token::normal(TokenType::givenTypeSeparator, this->getPosition());
    } else {
      previousContext = "colon";
      return Token::normal(TokenType::colon, this->getPosition());
    }
  }
  case '#': {
    previousContext = "hashtag";
    readNext(previousContext);
    return Token::normal(TokenType::hashtag, this->getPosition());
  }
  case '/': {
    previousContext = "operator";
    std::string value = "/";
    readNext(previousContext);
    if (current == '*') {
      bool star = false;
      previousContext = "multiLineComment";
      readNext(previousContext);
      while ((star ? (current != '/') : true) && !file.eof()) {
        if (star) {
          star = false;
        }
        if (current == '*') {
          star = true;
        }
        readNext(previousContext);
      }
      if (file.eof()) {
        return Token::valued(TokenType::endOfFile, filePath,
                             this->getPosition());
      } else {
        readNext(previousContext);
        return tokeniser();
      }
    } else if (current == '/') {
      previousContext = "singleLineComment";
      while ((current != '\n' && previous != '\r') && !file.eof()) {
        readNext(previousContext);
      }
      if (file.eof()) {
        return Token::valued(TokenType::endOfFile, filePath,
                             this->getPosition());
      } else {
        readNext(previousContext);
        return tokeniser();
      }
    } else {
      return Token::valued(TokenType::binaryOperator, value,
                           this->getPosition());
    }
  }
  case '!': {
    previousContext = "operator";
    std::string value = "!";
    readNext(previousContext);
    if (current == '=') {
      value += current;
      readNext(previousContext);
      return Token::valued(TokenType::binaryOperator, value,
                           this->getPosition());
    }
    return Token::valued(TokenType::unaryOperatorLeft, value,
                         this->getPosition());
  }
  case '+':
  case '-':
  case '%':
  case '*':
  case '<':
  case '>': {
    previousContext = "operator";
    std::string opValue = "";
    opValue += current;
    readNext(previousContext);
    if ((current == '+' && opValue == "+") ||
        (current == '-' && opValue == "-")) {
      opValue += current;
      readNext(previousContext);
      const std::string identifier =
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
      return Token::valued((identifier.find(current) != std::string::npos
                                ? TokenType::unaryOperatorRight
                                : TokenType::unaryOperatorRight),
                           opValue, this->getPosition());
    } else if (current == '=' && opValue != "<" && opValue != ">") {
      opValue += current;
      readNext(previousContext);
      return Token::valued(TokenType::assignedBinaryOperator, opValue,
                           this->getPosition());
    } else if ((current == '<' && opValue == "<") ||
               (current == '>' && opValue == ">")) {
      opValue += current;
      readNext(previousContext);
      if (current == '=' && opValue == ">>") {
        previousContext = "lambda";
        readNext(previousContext);
        return Token::normal(TokenType::lambda, this->getPosition());
      }
      return Token::valued(TokenType::binaryOperator, opValue,
                           this->getPosition());
    } else if (opValue == "<") {
      return Token::normal(TokenType::lesserThan, this->getPosition());
    } else if (opValue == ">") {
      return Token::normal(TokenType::greaterThan, this->getPosition());
    }
    return Token::valued(TokenType::binaryOperator, opValue,
                         this->getPosition());
  }
  case '=': {
    readNext("assignment");
    if (current == '=') {
      previousContext = "operator";
      readNext(previousContext);
      return Token::valued(TokenType::binaryOperator,
                           "==", this->getPosition());
    } else {
      previousContext = "assignment";
      return Token::normal(TokenType::assignment, this->getPosition());
    }
  }
  case '\'': {
    if (previousContext == "identifier" || previousContext == "loop" ||
        previousContext == "private" || previousContext == "literal" ||
        previousContext == "object") {
      previousContext = "child";
      readNext(previousContext);
      return Token::normal(TokenType::child, this->getPosition());
    } else {
      bool escape = false;
      readNext("string");
      std::string stringValue = "";
      while (escape ? !file.eof() : (current != '\'' && !file.eof())) {
        if (escape) {
          escape = false;
          if (current == '\'')
            stringValue += '\'';
          else if (current == '"')
            stringValue += '"';
          else if (current == '\\')
            stringValue += "\\\\";
          else if (current == 'n')
            stringValue += "\n";
          else if (current == '?')
            stringValue += "\?";
          else if (current == 'b')
            stringValue += "\b";
          else if (current == 'a')
            stringValue += "\a";
          else if (current == 'f')
            stringValue += "\f";
          else if (current == 'r')
            stringValue += "\r";
          else if (current == 't')
            stringValue += "\t";
          else if (current == 'v')
            stringValue += "\v";
          readNext("string");
        }
        if (current == '\\' && previous != '\\') {
          escape = true;
        } else {
          stringValue += current;
        }
        readNext("string");
      }
      previousContext = "literal";
      readNext(previousContext);
      return Token::valued(TokenType::StringLiteral, stringValue,
                           this->getPosition());
    }
  }
  case '"': {
    bool escape = false;
    readNext("string");
    std::string stringValue = "";
    while (escape ? !file.eof() : (current != '"' && !file.eof())) {
      if (escape) {
        escape = false;
        if (current == '\'')
          stringValue += '\'';
        else if (current == '"')
          stringValue += '"';
        else if (current == '\\')
          stringValue += "\\\\";
        else if (current == 'n')
          stringValue += "\n";
        else if (current == '?')
          stringValue += "\?";
        else if (current == 'b')
          stringValue += "\b";
        else if (current == 'a')
          stringValue += "\a";
        else if (current == 'f')
          stringValue += "\f";
        else if (current == 'r')
          stringValue += "\r";
        else if (current == 't')
          stringValue += "\t";
        else if (current == 'v')
          stringValue += "\v";
        readNext("string");
      }
      if (current == '\\' && previous != '\\') {
        escape = true;
      } else {
        stringValue += current;
      }
      readNext("string");
    }
    previousContext = "literal";
    readNext(previousContext);
    return Token::valued(TokenType::StringLiteral, stringValue,
                         this->getPosition());
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
    std::string numberValue = "";
    bool isFloat = false;
    const std::string digits = "0123456789";
    while (((digits.find(current, 0) != std::string::npos) ||
            (isFloat ? false : (current == '.'))) &&
           !file.eof()) {
      if (current == '.') {
        readNext("integer");
        if (digits.find(current, 0) != std::string::npos) {
          isFloat = true;
          numberValue += '.';
        } else {
          /// This is in the reverse order since the last element is returned
          /// first
          buffer.push_back(Token::normal(TokenType::stop, this->getPosition()));
          buffer.push_back(Token::valued(TokenType::IntegerLiteral, numberValue,
                                         this->getPosition()));
          return tokeniser();
        }
      }
      numberValue += current;
      readNext(isFloat ? "float" : "integer");
    }
    previousContext = "literal";
    return Token::valued(isFloat ? TokenType::FloatLiteral
                                 : TokenType::IntegerLiteral,
                         numberValue, this->getPosition() //
    );
  }
  case -1: {
    return Token::valued(TokenType::endOfFile, filePath, this->getPosition());
  }
  default: {
    const std::string alphabets =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if ((alphabets.find(current, 0) != std::string::npos) || current == '_') {
      const std::string digits = "0123456789";
      std::string ambiguousValue = "";
      while ((alphabets.find(current, 0) != std::string::npos) ||
             (digits.find(current, 0) != std::string::npos) ||
             (current == '_') && !file.eof()) {
        ambiguousValue += current;
        readNext("identifier");
      }
      previousContext = "keyword";
      if (ambiguousValue == "null")
        return Token::normal(TokenType::null, this->getPosition());
      else if (ambiguousValue == "bring")
        return Token::normal(TokenType::bring, this->getPosition());
      else if (ambiguousValue == "pub")
        return Token::normal(TokenType::Public, this->getPosition());
      else if (ambiguousValue == "pvt")
        return Token::normal(TokenType::Private, this->getPosition());
      else if (ambiguousValue == "void")
        return Token::normal(TokenType::Void, this->getPosition());
      else if (ambiguousValue == "obj")
        return Token::normal(TokenType::Object, this->getPosition());
      else if (ambiguousValue == "box")
        return Token::normal(TokenType::box, this->getPosition());
      else if (ambiguousValue == "const")
        return Token::normal(TokenType::constant, this->getPosition());
      else if (ambiguousValue == "space")
        return Token::normal(TokenType::space, this->getPosition());
      else if (ambiguousValue == "from")
        return Token::normal(TokenType::from, this->getPosition());
      else if (ambiguousValue == "to")
        return Token::normal(TokenType::to, this->getPosition());
      else if (ambiguousValue == "true")
        return Token::normal(TokenType::TRUE, this->getPosition());
      else if (ambiguousValue == "false")
        return Token::normal(TokenType::FALSE, this->getPosition());
      else if (ambiguousValue == "say")
        return Token::normal(TokenType::say, this->getPosition());
      else if (ambiguousValue == "as")
        return Token::normal(TokenType::as, this->getPosition());
      else if (ambiguousValue == "file")
        return Token::normal(TokenType::file, this->getPosition());
      else if (ambiguousValue == "lib")
        return Token::normal(TokenType::Library, this->getPosition());
      else if (ambiguousValue == "ref")
        return Token::normal(TokenType::reference, this->getPosition());
      else if (ambiguousValue == "this")
        return Token::normal(TokenType::This, this->getPosition());
      else if (ambiguousValue == "bool")
        return Token::normal(TokenType::Bool, this->getPosition());
      else if (ambiguousValue == "size")
        return Token::normal(TokenType::size, this->getPosition());
      else if (ambiguousValue.substr(0, 1) == "i" &&
               ((ambiguousValue.length() > 1)
                    ? qat::utilities::isInteger(
                          ambiguousValue.substr(1, ambiguousValue.length() - 1))
                    : false)) {
        return Token::valued(
            TokenType::Integer,
            ambiguousValue.substr(1, ambiguousValue.length() - 1),
            this->getPosition());
      } else if (ambiguousValue == "fbrain")
        return Token::valued(TokenType::Float, "brain", this->getPosition());
      else if (ambiguousValue == "fhalf")
        return Token::valued(TokenType::Float, "half", this->getPosition());
      else if (ambiguousValue == "f32")
        return Token::valued(TokenType::Float, "32", this->getPosition());
      else if (ambiguousValue == "f64")
        return Token::valued(TokenType::Float, "64", this->getPosition());
      else if (ambiguousValue == "f80")
        return Token::valued(TokenType::Float, "80", this->getPosition());
      else if (ambiguousValue == "f128ppc")
        return Token::valued(TokenType::Float, "128ppc", this->getPosition());
      else if (ambiguousValue == "f128")
        return Token::valued(TokenType::Float, "128", this->getPosition());
      else if (ambiguousValue == "string")
        return Token::normal(TokenType::String, this->getPosition());
      else if (ambiguousValue == "alias")
        return Token::normal(TokenType::Alias, this->getPosition());
      else if (ambiguousValue == "for")
        return Token::normal(TokenType::For, this->getPosition());
      else if (ambiguousValue == "give")
        return Token::normal(TokenType::give, this->getPosition());
      else if (ambiguousValue == "expose")
        return Token::normal(TokenType::expose, this->getPosition());
      else if (ambiguousValue == "mod")
        return Token::normal(TokenType::mod, this->getPosition());
      else if (ambiguousValue == "if")
        return Token::normal(TokenType::If, this->getPosition());
      else if (ambiguousValue == "else")
        return Token::normal(TokenType::Else, this->getPosition());
      else {
        previousContext = "identifier";
        return Token::valued(TokenType::identifier, ambiguousValue,
                             this->getPosition());
      }
    } else {
      throwError("Unrecognised character found: " + std::string(1, current));
      return Token::normal(TokenType::endOfFile, this->getPosition());
    }
  }
  }
}

void qat::lexer::Lexer::printStatus() {
  if (Lexer::showReport) {
    double timeValue = 0;
    std::string timeUnit = "";

    if (timeInNS < 1000000) {
      timeValue = ((double)timeInNS) / 1000;
      timeUnit = " microseconds \n";
    } else if (timeInNS < 1000000) {
      timeValue = ((double)timeInNS) / 1000000;
      timeUnit = " milliseconds \n";
    } else {
      timeValue = ((double)timeInNS) / 1000000000;
      timeUnit = " seconds \n";
    }
    std::cout << _CYAN "[ LEXER ] " _GREEN _BOLD << filePath
              << _NOBOLD _NORMALCOLOR << "\n   " << --lineNumber << " lines & "
              << --totalCharacterCount << " characters in " _BOLD << timeValue
              << timeUnit << _NOBOLD;
  }
  if (Lexer::emitTokens) {
    std::cout << "\nAnalysed Tokens \n\n";
    for (std::size_t i = 0; i < tokens.size(); i++) {
      switch (tokens.at(i).type) {
      case TokenType::Alias:
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
      case TokenType::box:
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
      case TokenType::inferredDeclaration:
        std::cout << " := ";
        break;
      case TokenType::Integer:
        std::cout << " i" << tokens.at(i).value << " ";
        break;
      case TokenType::IntegerLiteral:
        std::cout << " " << tokens.at(i).value << " ";
        break;
      case TokenType::lesserThan:
        std::cout << " < ";
        break;
      case TokenType::Library:
        std::cout << " lib ";
        break;
      case TokenType::mod:
        std::cout << " mod ";
        break;
      case TokenType::Object:
        std::cout << " obj ";
        break;
      case TokenType::pointerAccess:
        std::cout << " :> ";
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
      case TokenType::separator:
        std::cout << " , ";
        break;
      case TokenType::space:
        std::cout << " space ";
        break;
      case TokenType::startOfFile:
        std::cout << "STARTOFFILE  -  " << filePath << "\n";
        break;
      case TokenType::staticChild:
        std::cout << " ^ ";
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
      case TokenType::This:
        std::cout << " this ";
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
  std::cout << _RED "[ LEXER ERROR ] " _GREEN _BOLD << filePath << ":"
            << lineNumber << ":" << characterNumber
            << _NORMALCOLOR _NOBOLD "\n";
  std::cout << "   " << message << "\n";
  exit(0);
}