#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include "llvm/IR/Value.h"
#include "node_type.hpp"
#include "../IR/generator.hpp"

namespace qat
{
    namespace AST
    {
        class Node
        {
        public:
            virtual ~Node() = default;

            virtual llvm::Value *generate(IR::Generator *generator) = 0;

            virtual NodeType nodeType() = 0;
        };
    }
}

#endif