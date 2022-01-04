#include "lexer.hpp"

bool qat::lexer::Lexer::emitTokens = false;
bool qat::lexer::Lexer::showReport = false;

void qat::lexer::Lexer::readNext(std::string lexerContext)
{
    // try
    // {
    if (file.eof())
    {
        previous = current;
        current = -1;
    }
    else
    {
        previous = current;
        file.get(current);
        characterNumber++;
        totalCharacterCount++;
    }
    if (current == '\n')
    {
        lineNumber++;
        characterNumber = 0;
    }
    else if (current == '\r')
    {
        lineNumber++;
        previous = current;
        file.get(current);
        totalCharacterCount++;
        characterNumber = (current == '\n') ? 0 : 1;
    }
    // }
    // catch (...)
    // {
    //     logError("Analysis failed while reading the file");
    // }
}

std::vector<qat::lexer::Token> qat::lexer::Lexer::analyse()
{
    file.open(filePath, std::ios::in);
    auto lexerStartTime = std::chrono::high_resolution_clock::now();
    tokens.push_back(Token::valued(TokenType::startOfFile, filePath));
    readNext("start");
    while (!file.eof())
    {
        tokens.push_back(tokeniser());
    }
    file.close();
    if (tokens.back().type != TokenType::endOfFile)
    {
        tokens.push_back(Token::valued(TokenType::endOfFile, filePath));
    }
    std::chrono::nanoseconds lexerElapsed = std::chrono::high_resolution_clock::now() - lexerStartTime;
    lexerTimeInNS = lexerElapsed.count();
    return tokens;
}

void qat::lexer::Lexer::changeFile(std::string newFilePath)
{
    tokens.clear();
    filePath = newFilePath;
    previousContext = "";
    previous = -1;
    current = -1;
    lineNumber = 1;
    characterNumber = 0;
    lexerTimeInNS = 0;
    totalCharacterCount = 0;
}

qat::lexer::Token qat::lexer::Lexer::tokeniser()
{
    if (buffer.size() != 0)
    {
        qat::lexer::Token token = buffer.back();
        buffer.pop_back();
        return token;
    }
    if (file.eof())
    {
        return Token::valued(TokenType::endOfFile, filePath);
    }
    if (previous == '\r')
    {
        previousContext = "whitespace";
        readNext(previousContext);
        return tokeniser();
    }
    switch (current)
    {
    case ' ':
    case '\n':
    case '\r':
    case '\t':
    {
        previousContext = "whitespace";
        readNext(previousContext);
        return tokeniser();
    }
    case '.':
    {
        previousContext = "stop";
        readNext(previousContext);
        return Token::normal(TokenType::stop);
    }
    case ',':
    {
        previousContext = "separator";
        readNext(previousContext);
        return Token::normal(TokenType::separator);
    }
    case '(':
    {
        previousContext = "paranthesisOpen";
        readNext(previousContext);
        return Token::normal(TokenType::paranthesisOpen);
    }
    case ')':
    {
        previousContext = "paranthesisClose";
        readNext(previousContext);
        return Token::normal(TokenType::paranthesisClose);
    }
    case '[':
    {
        previousContext = "bracketOpen";
        readNext(previousContext);
        return Token::normal(TokenType::bracketOpen);
    }
    case ']':
    {
        previousContext = "bracketClose";
        readNext(previousContext);
        return Token::normal(TokenType::bracketClose);
    }
    case '{':
    {
        previousContext = "curlybraceOpen";
        readNext(previousContext);
        return Token::normal(TokenType::curlybraceOpen);
    }
    case '}':
    {
        previousContext = "curlybraceClose";
        readNext(previousContext);
        return Token::normal(TokenType::curlybraceClose);
    }
    case '<':
    {
        previousContext = "lesserThan";
        readNext(previousContext);
        return Token::normal(TokenType::lesserThan);
    }
    case '>':
    {
        previousContext = "greaterThan";
        readNext(previousContext);
        return Token::normal(TokenType::greatherThan);
    }
    case '@':
    {
        previousContext = "at";
        readNext(previousContext);
        return Token::normal(TokenType::at);
    }
    case ':':
    {
        readNext("colon");
        if (current != '=')
        {
            previousContext = "colon";
            return Token::normal(TokenType::colon);
        }
        else
        {
            previousContext = "assignedDeclaration";
            readNext(previousContext);
            return Token::normal(TokenType::assignedDeclaration);
        }
    }
    case '#':
    {
        previousContext = "hashtag";
        readNext(previousContext);
        return Token::normal(TokenType::hashtag);
    }
    case '=':
    {
        readNext("equal");
        if (current != '=')
        {
            previousContext = "equal";
            return Token::normal(TokenType::equal);
        }
        else
        {
            previousContext = "isEqual";
            readNext(previousContext);
            return Token::normal(TokenType::isEqual);
        }
    }
    case '\'':
    {
        if (previousContext == "identifier" ||
            previousContext == "loop" ||
            previousContext == "private" ||
            previousContext == "literal" ||
            previousContext == "object")
        {
            previousContext = "child";
            readNext(previousContext);
            return Token::normal(TokenType::child);
        }
        else
        {
            bool escape = false;
            readNext("string");
            std::string stringValue = "";
            while (escape ? (!file.eof()) : (current != '\'' && !file.eof()))
            {
                if (escape)
                {
                    escape = false;
                    readNext("string");
                    if (current == '\'')
                        stringValue += '\'';
                    else if (current == '"')
                        stringValue += '"';
                    else if (current == '\\')
                        stringValue += "\\\\";
                    else
                    {
                        stringValue += "\\";
                        stringValue += current;
                    }
                }
                if (current == '\\' && previous != '\\')
                {
                    escape = true;
                }
                else
                {
                    stringValue += current;
                }
                readNext("string");
            }
            previousContext = "literal";
            readNext(previousContext);
            return Token::valued(TokenType::StringLiteral, stringValue);
        }
    }
    case '"':
    {
        bool escape = false;
        readNext("string");
        std::string stringValue = "";
        while (escape ? (!file.eof()) : (current != '"' && !file.eof()))
        {
            if (escape)
            {
                escape = false;
                readNext("string");
                if (current == '\'')
                    stringValue += '\'';
                else if (current == '"')
                    stringValue += '"';
                else if (current == '\\')
                    stringValue += "\\\\";
                else
                {
                    stringValue += "\\";
                    stringValue += current;
                }
            }
            if (current == '\\' && previous != '\\')
            {
                escape = true;
            }
            else
            {
                stringValue += current;
            }
            readNext("string");
        }
        previousContext = "literal";
        readNext(previousContext);
        return Token::valued(TokenType::StringLiteral, stringValue);
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
    case '9':
    {
        std::string numberValue = "";
        bool isDouble = false;
        const std::string digits = "0123456789";
        while (((digits.find(current, 0) != std::string::npos) ||
                (isDouble ? false : (current == '.'))) &&
               !file.eof())
        {
            if (current == '.')
            {
                readNext("integer");
                if (digits.find(current, 0) != std::string::npos)
                {
                    isDouble = true;
                    numberValue += '.';
                }
                else
                {
                    buffer.push_back(Token::normal(TokenType::stop));
                    buffer.push_back(Token::valued(TokenType::DoubleLiteral, numberValue));
                    return tokeniser();
                }
            }
            numberValue += current;
            readNext(isDouble ? "double" : "integer");
        }
        previousContext = "literal";
        return Token::valued(
            isDouble ? TokenType::DoubleLiteral : TokenType::IntegerLiteral,
            numberValue //
        );
    }
    case -1:
    {
        return Token::valued(TokenType::endOfFile, filePath);
    }
    default:
    {
        const std::string alphabets = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if ((alphabets.find(current, 0) != std::string::npos) || current == '_')
        {
            const std::string digits = "0123456789";
            std::string ambiguousValue = "";
            while ((alphabets.find(current, 0) != std::string::npos) ||
                   (digits.find(current, 0) != std::string::npos) ||
                   (current == '_') && !file.eof())
            {
                ambiguousValue += current;
                readNext("identifier");
            }
            previousContext = "keyword";
            if (ambiguousValue == "null")
                return Token::normal(TokenType::null);
            else if (ambiguousValue == "bring")
                return Token::normal(TokenType::bring);
            else if (ambiguousValue == "pub")
                return Token::normal(TokenType::Public);
            else if (ambiguousValue == "pvt")
                return Token::normal(TokenType::Private);
            else if (ambiguousValue == "void")
                return Token::normal(TokenType::Void);
            else if (ambiguousValue == "object")
                return Token::normal(TokenType::Object);
            else if (ambiguousValue == "class")
                return Token::normal(TokenType::Class);
            else if (ambiguousValue == "const")
                return Token::normal(TokenType::constant);
            else if (ambiguousValue == "space")
                return Token::normal(TokenType::space);
            else if (ambiguousValue == "from")
                return Token::normal(TokenType::From);
            else if (ambiguousValue == "true")
                return Token::normal(TokenType::TRUE);
            else if (ambiguousValue == "false")
                return Token::normal(TokenType::FALSE);
            else if (ambiguousValue == "say")
                return Token::normal(TokenType::say);
            else if (ambiguousValue == "as")
                return Token::normal(TokenType::as);
            else if (ambiguousValue == "file")
                return Token::normal(TokenType::file);
            else if (ambiguousValue == "lib")
                return Token::normal(TokenType::Library);
            else if (ambiguousValue == "int")
                return Token::normal(TokenType::Integer);
            else if (ambiguousValue == "double")
                return Token::normal(TokenType::Double);
            else if (ambiguousValue == "string")
                return Token::normal(TokenType::String);
            else if (ambiguousValue == "alias")
                return Token::normal(TokenType::Alias);
            else if (ambiguousValue == "for")
                return Token::normal(TokenType::For);
            else if (ambiguousValue == "give")
                return Token::normal(TokenType::give);
            else
            {
                previousContext = "identifier";
                return Token::valued(TokenType::identifier, ambiguousValue);
            }
        }
        else
        {
            logError("Unrecognised character found: " + std::string(1, current));
            exit(0);
        }
    }
    }
}

void qat::lexer::Lexer::printStatus()
{
    if (Lexer::showReport)
    {
        double timeValue = 0;
        std::string timeUnit = "";

        if (lexerTimeInNS < 1000000)
        {
            timeValue = ((double)lexerTimeInNS) / 1000;
            timeUnit = " microseconds \n";
        }
        else if (lexerTimeInNS < 1000000)
        {
            timeValue = ((double)lexerTimeInNS) / 1000000;
            timeUnit = " milliseconds \n";
        }
        else
        {
            timeValue = ((double)lexerTimeInNS) / 1000000000;
            timeUnit = " seconds \n";
        }
        std::cout << _CYAN "[ LEXER ] " _NORMALCOLOR "\"" << filePath << "\" " << --lineNumber << " lines & "
                  << --totalCharacterCount << " characters in "
                  << timeValue << timeUnit;
    }
    if (Lexer::emitTokens)
    {
        std::cout << "\nAnalysed Tokens \n\n";
        for (std::size_t i = 0; i < tokens.size(); i++)
        {
            switch (tokens.at(i).type)
            {
            case TokenType::Alias:
                std::cout << " alias ";
                break;
            case TokenType::as:
                std::cout << " as ";
                break;
            case TokenType::assignedDeclaration:
                std::cout << " := ";
                break;
            case TokenType::at:
                std::cout << " @ ";
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
            case TokenType::Double:
                std::cout << " double ";
                break;
            case TokenType::DoubleLiteral:
                std::cout << " " << tokens.at(i).value << " ";
                break;
            case TokenType::endOfFile:
                std::cout << "\nENDOFFILE  -  " << filePath << "\n";
                break;
            case TokenType::equal:
                std::cout << " = ";
                break;
            case TokenType::external:
                std::cout << " external ";
                break;
            case TokenType::file:
                std::cout << " file ";
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
            case TokenType::From:
                std::cout << " from ";
                break;
            case TokenType::give:
                std::cout << " give ";
                break;
            case TokenType::greatherThan:
                std::cout << " > ";
                break;
            case TokenType::hashtag:
                std::cout << " # ";
                break;
            case TokenType::identifier:
                std::cout << " " << tokens.at(i).value << " ";
                break;
            case TokenType::isEqual:
                std::cout << " == ";
                break;
            case TokenType::Integer:
                std::cout << " int ";
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
            case TokenType::Void:
                std::cout << " void ";
                break;
            case TokenType::null:
                std::cout << " null ";
                break;
            case TokenType::Object:
                std::cout << " obj ";
                break;
            case TokenType::Class:
                std::cout <<" class ";
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
            case TokenType::stop:
                std::cout << " .\n";
                break;
            case TokenType::String:
                std::cout << " string ";
                break;
            case TokenType::StringLiteral:
                std::cout << " \"" << tokens[i].value << "\" ";
                break;
            case TokenType::TRUE:
                std::cout << " true ";
                break;
            default:
                std::cout << " UNKNOWN ";
                break;
            }
        }
        std::cout << "\n"
                  << std::endl;
    }
}

void qat::lexer::Lexer::logError(std::string message)
{
    std::cout << "[LEXER ERROR] in line " << lineNumber
              << " at " << characterNumber << " of "
              << filePath << "\n";
    std::cout << _RED "   " << message << _NORMALCOLOR;
}