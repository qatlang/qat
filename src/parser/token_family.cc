#include "./token_family.hpp"

namespace qat::parser {

Vec<lexer::TokenType> TokenFamily::genericTypeSpecifiers{
	lexer::TokenType::identifier, lexer::TokenType::integerType,	lexer::TokenType::voidType,
	lexer::TokenType::floatType,  lexer::TokenType::markType,		lexer::TokenType::separator,
	lexer::TokenType::colon,	  lexer::TokenType::integerLiteral, lexer::TokenType::floatLiteral};

Vec<lexer::TokenType> TokenFamily::functionArgs{lexer::TokenType::identifier, lexer::TokenType::integerType,
												lexer::TokenType::voidType,	  lexer::TokenType::floatType,
												lexer::TokenType::markType,	  lexer::TokenType::separator,
												lexer::TokenType::colon};

Vec<lexer::TokenType> TokenFamily::scopeLimiters{lexer::TokenType::bracketOpen,		lexer::TokenType::bracketClose,
												 lexer::TokenType::curlybraceOpen,	lexer::TokenType::curlybraceClose,
												 lexer::TokenType::parenthesisOpen, lexer::TokenType::parenthesisClose};

} // namespace qat::parser
