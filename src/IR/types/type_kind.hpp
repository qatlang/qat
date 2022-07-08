#ifndef QAT_IR_TYPES_TYPE_KIND_HPP
#define QAT_IR_TYPES_TYPE_KIND_HPP

namespace qat {
namespace IR {

/**
 *  The kind of type of the QatType instance.
 *
 * I almost named this TypeType
 *
 */
enum class TypeKind {
  Void,            // Void type
  array,           // Array of elements of QatType type
  core,            // Struct type
  Float,           // Floating point number
  integer,         // Signed integer
  unsignedInteger, // Unsigned integer
  vector,          // Vector of elements of another QatType type
  sumType,         // Sum Type - Additions of different types (union)
  tuple,           // Tuple is a product type of multiple types
  pointer,         // Pointer to another QatType type
  reference,       // Reference to another QatType type
  function,
  /* Template type kinds */
  templateCoreType, // Template struct type
  templateSumType,  // Template sum type
  templateTuple,    // Template tuple type
  templatePointer,  // Template pointer
};

} // namespace IR
} // namespace qat

#endif