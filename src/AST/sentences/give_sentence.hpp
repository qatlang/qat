#ifndef QAT_AST_SENTENCES_GIVE_SENTENCE_HPP
#define QAT_AST_SENTENCES_GIVE_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include <optional>

namespace qat::AST {
/**
 *  GiveSentence represents a return statement in the language
 *
 */
class GiveSentence : public Sentence {
private:
  /**
   *  Expression that should be returned/given
   *
   */
  std::optional<Expression *> give_expr;

public:
  /**
   *  GiveSentence represents a return statement in the language
   *
   * @param _filePlacement
   */
  GiveSentence(std::optional<Expression *> _given_expr,
               utils::FileRange _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return qat::AST::NodeType::giveSentence; }
};

} // namespace qat::AST

#endif