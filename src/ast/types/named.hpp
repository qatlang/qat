#ifndef QAT_AST_TYPES_NAMED_HPP
#define QAT_AST_TYPES_NAMED_HPP

#include "../../IR/context.hpp"
#include "../box.hpp"
#include "../function.hpp"
#include "./qat_type.hpp"

#include <string>
#include <vector>

namespace qat::ast {

// NamedType is a type, usually a core type, that can be identified by a name
class NamedType : public QatType {
private:
  u32    relative;
  String name;

public:
  NamedType(u32 relative, String name, bool variable,
            utils::FileRange fileRange);

  useit String getName() const;
  useit u32    getRelative() const;
  useit IR::QatType *emit(IR::Context *ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif