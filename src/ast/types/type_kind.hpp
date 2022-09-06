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
  Void,            // Void type
  array,           // Array of elements of QatType type
  named,           // Struct type
  Float,           // Floating point number
  integer,         // Signed integer
  unsignedInteger, // Unsigned integer
  stringSlice,     // String slice
  cstring,         // C String
  tuple,           // Tuple is a product type of multiple types
  pointer,         // Pointer to another QatType type
  reference,       // Reference to another QatType type
  function,
  templated, // Templated type
};

} // namespace qat::ast

#endif