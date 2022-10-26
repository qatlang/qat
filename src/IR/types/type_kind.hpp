#ifndef QAT_IR_TYPES_TYPE_KIND_HPP
#define QAT_IR_TYPES_TYPE_KIND_HPP

namespace qat::IR {

/**
 *  The nature of type of the QatType instance.
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
  stringSlice,     // String slice
  cstring,         // C string
  mixType,         // Mix Type (Discriminated Union / Sum Type)
  tuple,           // Tuple is a product type of multiple types
  pointer,         // Pointer to another QatType type
  reference,       // Reference to another QatType type
  function,
  definition, // A type definition
  choice,
  future,
  maybe,
};

} // namespace qat::IR

#endif