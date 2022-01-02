#ifndef QAT_AST_NODETYPE_HPP
#define QAT_AST_NODETYPE_HPP

namespace qat
{
    namespace AST
    {
        enum class NodeType
        {
            functionPrototype,
            functionDefinition,
            functionParagraph,
            localDeclaration,
            integerLiteral,
            doubleLiteral,
            stringLiteral,
        };
    }
}

#endif