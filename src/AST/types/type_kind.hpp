#ifndef QAT_AST_TYPES_TYPE_KIND_HPP
#define QAT_AST_TYPES_TYPE_KIND_HPP

namespace qat {
namespace AST {

/**
 * @brief The kind of type of the QatType instance.
 *
 * I almost named this TypeType
 *
 */
enum class TypeKind {
  Void,     // Void type
  array,    // Array of elements of QatType type
  concrete, // Struct type
  Float,    // Floating point number
  integer,  // Signed or unsigned integer
  vector,   // Vector of elements of another QatType type
  sumType,  // Sum Type - Additions of different types (union)
  tuple,    // Tuple is a product type of multiple types
  pointer,  // Pointer to another QatType type
  /* Template type kinds */
  templateConcrete, // Template struct type
  templateSumType,  // Template sum type
  templateTuple,    // Template tuple type
  templatePointer,  // Template pointer
};

} // namespace AST
} // namespace qat

#endif