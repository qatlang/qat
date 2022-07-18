#ifndef QAT_AST_EXPRESSIONS_SIZE_OF_TYPE_HPP
#define QAT_AST_EXPRESSIONS_SIZE_OF_TYPE_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::AST {

/**
 *  SizeOfType is used to find the size of a type specified
 *
 */
class SizeOfType : public Expression {
private:
  /**
   *  Type to find the size of
   *
   */
  QatType *type;

public:
  /**
   *  SizeOfType is used to find the size of a type specified
   *
   * @param _type
   * @param _filePlacement
   */
  SizeOfType(QatType *_type, utils::FileRange _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::sizeOfType; }
};

} // namespace qat::AST

#endif