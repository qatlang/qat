#include "./expression_sentence.hpp"

namespace qat {
namespace AST {

ExpressionSentence::ExpressionSentence(Expression *_exp,
                                       utils::FilePlacement _filePlacement)
    : expr(_exp), qat::AST::Sentence(_filePlacement) {}

llvm::Value *ExpressionSentence::generate(IR::Generator *generator) {
  return expr->generate(generator);
}

} // namespace AST
} // namespace qat