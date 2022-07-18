#ifndef QAT_AST_EXPRESSIONS_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_ENTITY_HPP

#include "../expression.hpp"
#include <string>

namespace qat::AST {

/**
 *  Entity represents either a variable or constant. The name of the
 * entity is mostly just an identifier, but it can be other values if the
 * entity is present in the global constant
 *
 */
class Entity : public Expression {

private:
  /**
   *  Name of the entity
   *
   */
  std::string name;

public:
  /**
   *  Entity represents either a variable or constant. The name of the
   * entity is mostly just an identifier, but it can be other values if the
   * entity is present in the global constant
   *
   * @param _name Name of the entity
   * @param _filePlacement FilePLacement instance that represents the range
   * spanned by the tokens making up this AST member
   */
  Entity(std::string _name, utils::FileRange _filePlacement)
      : name(_name), Expression(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return qat::AST::NodeType::entity; }
};

} // namespace qat::AST

#endif