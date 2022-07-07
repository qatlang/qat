#ifndef QAT_AST_TYPES_NAMED_HPP
#define QAT_AST_TYPES_NAMED_HPP

#include "../../IR/generator.hpp"
#include "../box.hpp"
#include "../function_definition.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include <string>
#include <vector>

namespace qat {
namespace AST {

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
  std::string name;

public:
  /**
   *  NamedType is a type, usually a core type, that can be identified by
   * a name
   *
   * @param _name Name of the type
   * @param _filePlacement
   */
  NamedType(const std::string _name, const bool _variable,
            const utils::FilePlacement _filePlacement);

  llvm::Type *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  // Get the name of the type
  std::string get_name() const;

  TypeKind typeKind();

  backend::JSON toJSON() const;
};

} // namespace AST
} // namespace qat

#endif