#include "./global_declaration.hpp"

namespace qat::ast {

GlobalDeclaration::GlobalDeclaration(String _name, Maybe<QatType *> _type,
                                     Expression *_value, bool _isVariable,
                                     utils::FileRange _fileRange)
    : Node(_fileRange), name(_name), type(_type), value(_value),
      isVariable(_isVariable) {}

IR::Value *GlobalDeclaration::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json GlobalDeclaration::toJson() const {
  return nuo::Json()
      ._("nodeType", "globalDeclaration")
      ._("name", name)
      ._("hasType", type.has_value())
      ._("type", (type.has_value() ? type.value()->toJson() : nuo::Json()))
      ._("value", value->toJson())
      ._("variability", isVariable)
      ._("fileRange", fileRange);
}

} // namespace qat::ast