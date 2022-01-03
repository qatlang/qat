#ifndef QAT_LEXER_TOKEN_HPP
#define QAT_LEXER_TOKEN_HPP

#include <string>
#include "token_type.hpp"

namespace qat
{
    namespace lexer
    {
        /**
         * @brief Token constitutes of a symbol encountered by the Lexer
         * during file analysis. All tokens are later parsed through, by
         * the QAT Parser to obtain an abstract syntax tree.
         */
        class Token
        {
        private:
            explicit Token(
                TokenType _type,
                std::string _value = "") : type(_type), value(_value) {}

        public:            
            /**
             * @brief Tokens that represents a value. Integer literal, Double literal,
             * Identifiers are some examples of this kind of token
             * 
             * @param _type Type of the token - In the case of an Identifier token,
             * this is TokenType::identifier
             * @param _value Value of the token - In the case of an identifier token,
             * this is the name of the identifier
             * @return Token 
             */
            static Token valued(TokenType _type, std::string _value);

            /**
             * @brief Tokens that are by default, recognised by the language. These tokens
             * represents elements that are part of the language.
             * 
             * @param _type Type of the token - In the case of an Identifier token,
             * this is TokenType::identifier
             * @return Token 
             */
            static Token normal(TokenType _type);

            /**
             * @brief Type of the token. Can mostly refer to symbols that
             * are part of the language.
             */
            TokenType type;

            /**
             * @brief Value of the token. This will be empty for all tokens
             * that are part of the language
             */
            std::string value;

            /**
             * @brief Whether this token represents a value 
             */
            bool hasValue = false;
        };
    }
}

#endif