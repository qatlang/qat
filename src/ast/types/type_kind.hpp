#ifndef QAT_AST_TYPES_TYPE_KIND_HPP
#define QAT_AST_TYPES_TYPE_KIND_HPP

namespace qat::ast {

/**
 *  The nature of type of the QatType instance.
 *
 * I almost named this TypeType
 *
 */
enum class TypeKind {
  Void,  // Void type
  array, // Array of elements of QatType type
  named, // Struct type
  genericNamed,
  Float,           // Floating point number
  integer,         // Signed integer
  unsignedInteger, // Unsigned integer
  stringSlice,     // String slice
  cType,           // C type
  tuple,           // Tuple is a product type of multiple types
  pointer,         // Pointer to another QatType type
  reference,       // Reference to another QatType type
  function,
  future,
  genericAbstract, // Generic abstract type
  maybe,
  linkedGeneric,
  result,
  selfType,
};

} // namespace qat::ast

#endif