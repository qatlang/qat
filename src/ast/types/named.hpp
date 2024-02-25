#ifndef QAT_AST_TYPES_NAMED_HPP
#define QAT_AST_TYPES_NAMED_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class NamedType final : public QatType {
private:
  u32             relative;
  Vec<Identifier> names;

  Maybe<usize> typeSize;

public:
  NamedType(u32 _relative, Vec<Identifier> _names, FileRange _fileRange)
      : QatType(_fileRange), relative(_relative), names(_names) {}

  useit static inline NamedType* create(u32 _relative, Vec<Identifier> _names, FileRange _fileRange) {
    return std::construct_at(OwnNormal(NamedType), _relative, _names, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> expect, IR::EntityState* ent,
                           IR::Context* ctx) final;

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final { return typeSize; }
  useit String       getName() const;
  useit u32          getRelative() const;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif