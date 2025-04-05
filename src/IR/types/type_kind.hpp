#ifndef QAT_IR_TYPES_TYPE_KIND_HPP
#define QAT_IR_TYPES_TYPE_KIND_HPP

namespace qat::ir {

// I almost named this TypeType
enum class TypeKind {
	VOID,             // Void type
	ARRAY,            // Array of elements of Type type
	STRUCT,           // Struct type
	FLOAT,            // Floating point number
	INTEGER,          // Signed integer
	UNSIGNED_INTEGER, // Unsigned integer
	TEXT,             // Text type (String View)
	MIX,              // Mix Type (Discriminated Union / Sum Type)
	TUPLE,            // Tuple is a product type of multiple types
	POINTER,          // Pointer to another Type type
	REFERENCE,        // Reference to another Type type
	SLICE,            // Reference-like type that represents multiple values
	FUNCTION,         // Function type
	DEFINITION,       // A type definition
	CHOICE,           // Choice (C++ style enums)
	FUTURE,           // Future type
	MAYBE,            // Optional type
	REGION,           // Memory safe arena allocator
	OPAQUE,           // Opaque type for temporary representation
	TYPED,            // Representation for Type Information
	NATIVE,           // Type from C language
	RESULT,           // Result Type that also wraps an error
	VECTOR,           // SIMD Vector types
	TOGGLE,           // C-style unions
	POLYMORPH,        // Runtime behavioural polymorphism object
	FLAG,             // Bitflag
};

} // namespace qat::ir

#endif
