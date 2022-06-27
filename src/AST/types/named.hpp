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

  /**
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Type*
   */
  llvm::Type *generate(IR::Generator *generator);

  /**
   *  Get the name of the type
   *
   * @return std::string
   */
  std::string get_name() const;

  /**
   *  TypeKind is used to detect variants of the QatType
   *
   * @return TypeKind
   */
  TypeKind typeKind();

  backend::JSON toJSON() const;
};

} // namespace AST
} // namespace qat

#endif