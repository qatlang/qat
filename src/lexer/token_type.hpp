#ifndef QAT_LEXER_TOKENS_HPP
#define QAT_LEXER_TOKENS_HPP

#include <map>
#include <string>

namespace qat {
    namespace lexer {
        /**
         * @brief Type of the token recognised by the Lexer.
         * This does not necessarily have to mean anything as
         * far as language semantics goes.
         */
        enum class TokenType {
            null = -1, // null represents an empty value
            FALSE = 0, // false 
            TRUE = 1,
            startOfFile,
            give,
            Public,
            Private,
            stop,
            Void,
            file,
            bring,
            From,
            child,
            external,
            function,
            Object,
            Class,
            identifier,
            paranthesisOpen,
            paranthesisClose,
            curlybraceOpen,
            curlybraceClose,
            bracketOpen,
            bracketClose,
            lesserThan,
            greatherThan,
            colon,
            at,
            Alias,
            For,
            constant,
            assignedDeclaration,
            equal,
            isEqual,
            hashtag,
            separator,
            Integer,
            IntegerLiteral,
            Double,
            DoubleLiteral,
            String,
            StringLiteral,
            say,
            as,
            Library,
            space,
            endOfFile,
        };
    }
}

#endif