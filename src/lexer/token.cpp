#include "./token.hpp"

qat::lexer::Token qat::lexer::Token::valued(
    qat::lexer::TokenType _type,
    std::string _value //
) {
    Token token = Token(_type, _value);
    token.hasValue = true;
    return token;
}

qat::lexer::Token qat::lexer::Token::normal(qat::lexer::TokenType _type) {
    return Token(_type, "");
}