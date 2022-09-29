#include "./bring_paths.hpp"
#include <filesystem>

namespace qat::ast {

BringPaths::BringPaths(Vec<StringLiteral *>         _paths,
                       const utils::VisibilityInfo &_visibility,
                       utils::FileRange             _fileRange)
    : paths(std::move(_paths)), visibility(_visibility),
      Sentence(std::move(_fileRange)) {}

IR::Value *BringPaths::emit(IR::Context *ctx) {
  for (auto const &pathString : paths) {
    auto path = pathString->get_value();
    if (fs::exists(path)) {
      if (fs::is_directory(path)) {
        // TODO - Implement this
      } else if (fs::is_regular_file(path)) {
        // TODO - Implement this
      } else {
        ctx->Error("Cannot bring this file type", pathString->fileRange);
      }
    } else {
      ctx->Error("The path provided does not exist: " + path +
                     " and cannot be brought in.",
                 pathString->fileRange);
    }
  }
  return nullptr;
}

Json BringPaths::toJson() const {
  Vec<JsonValue> pths;
  for (auto *path : paths) {
    pths.push_back(path->toJson());
  }
  return Json()
      ._("nodeType", "bringPaths")
      ._("paths", std::move(pths))
      ._("visibility", visibility)
      ._("fileRange", fileRange);
}

} // namespace qat::ast