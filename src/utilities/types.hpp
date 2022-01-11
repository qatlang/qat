#ifndef QAT_UTILITIES_TYPES_HPP
#define QAT_UTILITIES_TYPES_HPP

// Type aliases and definitions for simplicity and readability

namespace qat
{
    namespace utilities
    {
        /// A reference to a character array can be treated as a string
        using StringConstant = const char *;

        /// A reference to an array of character arrays can be treated as a list of string
        using StringConstantList = const char **;

        /// A reference to a character array can be treated as a string
        using String = char *;

        /// A reference to an array of character arrays can be treated as a list of string
        using StringList = char **;
    }
}

#endif