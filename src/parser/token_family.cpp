#include "./token_family.hpp"

namespace qat::parser {

std::vector<lexer::TokenType> TokenFamily::templateTypeSpecifiers{
    lexer::TokenType::identifier,  lexer::TokenType::integerType,
    lexer::TokenType::voidType,    lexer::TokenType::floatType,
    lexer::TokenType::pointerType, lexer::TokenType::lesserThan,
    lexer::TokenType::greaterThan, lexer::TokenType::separator,
    lexer::TokenType::colon,       lexer::TokenType::integerLiteral,
    lexer::TokenType::floatLiteral};

std::vector<lexer::TokenType> TokenFamily::functionArgs{
    lexer::TokenType::identifier,  lexer::TokenType::integerType,
    lexer::TokenType::voidType,    lexer::TokenType::floatType,
    lexer::TokenType::pointerType, lexer::TokenType::lesserThan,
    lexer::TokenType::greaterThan, lexer::TokenType::separator,
    lexer::TokenType::colon};

std::vector<lexer::TokenType> TokenFamily::scopeLimiters{
    lexer::TokenType::bracketOpen,     lexer::TokenType::bracketClose,
    lexer::TokenType::curlybraceOpen,  lexer::TokenType::curlybraceClose,
    lexer::TokenType::parenthesisOpen, lexer::TokenType::parenthesisClose};

} // namespace qat::parser