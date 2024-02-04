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
  stringSlice,     // String Slice
  mixType,         // Mix Type (Discriminated Union / Sum Type)
  tuple,           // Tuple is a product type of multiple types
  pointer,         // Pointer to another QatType type
  reference,       // Reference to another QatType type
  function,        // Function type
  definition,      // A type definition
  choice,          // Choice (C++ style enums)
  future,          // Future type
  maybe,           // Optional type
  region,          // Memory safe arena allocator
  opaque,          // Opaque type for temporary representation
  typed,           // Type Wrapping to hold another type
  cType,           // Type from C language
  result,          // Result Type that also wraps an error
  vector,
};

} // namespace qat::IR

#endif
