#include "./box.hpp"

namespace qat {
namespace AST {

llvm::Value *Box::emit(IR::Generator *generator) {
  // FIXME - Perform name checks
  generator->mod->openBox(name, visibility);
}

void Box::emitCPP(backend::cpp::File &file, bool isHeader) const {
  file += ("namespace " + name + " ");
  file.addEnclosedComment("box");
  file += " {\n";
  for (auto mem : members) {
    mem->emitCPP(file, isHeader);
  }
  file += "} // namespace " + name + " (box)\n";
}

backend::JSON Box::toJSON() const {
  std::vector<backend::JSON> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "box")
      ._("members", mems)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat