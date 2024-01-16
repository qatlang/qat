#ifndef QAT_AST_TYPES_NAMED_HPP
#define QAT_AST_TYPES_NAMED_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

// NamedType is a type, usually a core type, that can be identified by a name
class NamedType : public QatType {
private:
  u32             relative;
  Vec<Identifier> names;

public:
  NamedType(u32 _relative, Vec<Identifier> _names, FileRange _fileRange)
      : QatType(_fileRange), relative(_relative), names(_names) {}

  useit static inline NamedType* create(u32 _relative, Vec<Identifier> _names, FileRange _fileRange) {
    return std::construct_at(OwnNormal(NamedType), _relative, _names, _fileRange);
  }

  useit String getName() const;
  useit u32    getRelative() const;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif