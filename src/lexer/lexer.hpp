#ifndef QAT_LEXER_LEXER_HPP
#define QAT_LEXER_LEXER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include "./token.hpp"
#include "./token_type.hpp"
#include "../CLI/color.hpp"

namespace qat
{
    namespace lexer
    {
        /**
         * @brief Lexer of the QAT Programming language. This handles lexical
         * analysis, emission of tokens and also will report on statistics
         * regarding the analysis
         * 
         */
        class Lexer
        {
        private:
            /**
             * @brief Path to the current file
             * 
             */
            std::string filePath;
            
            /**
             * @brief The current file that is being analysed by the lexer.
             * This is open only if the analysis is in progress.
             * 
             */
            std::fstream file;

            /**
             * @brief Context that can provide information about the previous
             * token emitted by the Lexer
             * 
             */
            std::string previousContext;

            /**
             * @brief Previous character read by the `readNext` function. This
             * is changed everytime a new character is read and is saved to the
             * current variable 
             * 
             */
            char previous;

            /**
             * @brief The latest character read by the `readNext` function. This
             * value is updated everytime a new character is read from the current
             * file 
             * 
             */
            char current;

            /**
             * @brief The current line of the lexer. This is incremented everytime
             * the End of Line is encountered
             * 
             */
            long long lineNumber = 1;

            /**
             * @brief The number pertaining to the current character read by the
             * lexer. This is incremented by the `readNext` function everytime a
             * character is read in the file. This is reset to zero after the End
             * of Line is encountered
             * 
             */
            long long characterNumber = 0;

            /**
             * @brief Total number of characters found in the file. This is for
             * reporting purposes
             * 
             */
            long long totalCharacterCount = 0;
            
            /**
             * @brief Vector of all tokens found during lexical analysis. This is
             * updated after each invocation of the `tokeniser` function.
             * 
             */
            std::vector<Token> tokens;

            /**
             * @brief A temporary buffer for tokens that have not yet been added to
             * the `tokens` vector
             * 
             */
            std::vector<Token> buffer;

            /**
             * @brief Time taken by the lexer to analyse the entire file, in
             * nanoseconds. This is later converted to appropriate higher units,
             * depending on the value
             * 
             */
            long long lexerTimeInNS = 0;

        public:
            Lexer() {}

            /**
             * @brief Whether the Lexer should emit tokens to the standard output.
             * If this is set to true, all tokens found in a file are dumped to the
             * standard output in a human-readable format.
             * 
             */
            static bool emitTokens;

            /**
             * @brief Whether the Lexer should display report on its behaviour and
             * analysis. For now, enabling this option shows the time taken by the
             * lexer to analyse a file.
             * 
             */
            static bool showReport;

            /**
             * @brief Logs any error occuring during Lexical Analysis.
             *  
             * @param message A meaningful message about the type of error
             */
            void logError(std::string message);

            /**
             * @brief Analyses the current file and emit tokens as identified
             * by the language
             * 
             * @return std::vector<Token> Returns all tokens identified during
             * analysis. 
             */
            std::vector<Token> analyse();

            /**
             * @brief This function reads the next character in the active file
             * of the Lexer. This can be customised using the context string
             * 
             * @param lexerContext A simple string that provides context for the
             * reader to customise reading behaviour.
             */
            void readNext(std::string lexerContext);

            /**
             * @brief Handles the change of the active file
             * 
             * @param newFile Path to the new file that should be analysed by the
             * lexer next
             */

            void changeFile(std::string newFile);

            /**
             * @brief Most important part of the Lexer. This emits tokens one at a
             * time as it goes through the source file
             * 
             * @return Token The latest token obtained during lexical analysis
             */
            Token tokeniser();

            /**
             * @brief Prints all status about the lexical analysis to the standard
             * output. This function's output is dependent on the CLI arguments
             * `--emit-tokens` and `--show-report` provided to the compiler
             * 
             */
            void printStatus();
        };
    }
}

#endif