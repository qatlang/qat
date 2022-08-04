#ifndef QAT_AST_TYPES_NAMED_HPP
#define QAT_AST_TYPES_NAMED_HPP

#include "../../IR/context.hpp"
#include "../box.hpp"
#include "../function_definition.hpp"
#include "./qat_type.hpp"

#include <string>
#include <vector>

namespace qat::ast {

/**
 *  NamedType is a type, usually a core type, that can be identified by a
 * name
 *
 */
class NamedType : public QatType {
private:
  /**
   *  Name of the type
   *
   */
  String name;

public:
  /**
   *  NamedType is a type, usually a core type, that can be identified by
   * a name
   *
   * @param _name Name of the type
   * @param _fileRange
   */
  NamedType(const String _name, const bool _variable,
            const utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

  // Get the name of the type
  String getName() const;

  TypeKind typeKind();

  nuo::Json toJson() const;

  String toString() const;
};

} // namespace qat::ast

#endif