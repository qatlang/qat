#include "./bring_paths.hpp"
#include <filesystem>

namespace qat::ast {

BringPaths::BringPaths(std::vector<StringLiteral> _paths,
                       utils::VisibilityInfo _visibility,
                       utils::FileRange _fileRange)
    : paths(_paths), visibility(_visibility), Sentence(_fileRange) {}

IR::Value *BringPaths::emit(IR::Context *ctx) {
  namespace fs = std::filesystem;
  for (auto pathstr : paths) {
    auto path = pathstr.get_value();
    if (fs::exists(path)) {
      if (fs::is_directory(path)) {
        // TODO - Implement this
      } else if (fs::is_regular_file(path)) {
        // TODO - Implement this
      } else {
        ctx->throw_error("Cannot bring this file type", pathstr.fileRange);
      }
    } else {
      ctx->throw_error("The path provided does not exist: " + path +
                           " and cannot be brought in.",
                       pathstr.fileRange);
    }
  }
  return nullptr;
}

void BringPaths::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (isHeader) {
    auto base = fileRange.file;
    for (auto pathVal : paths) {
      auto path = pathVal.get_value();
      if (fs::is_regular_file(path) &&
          (fs::path(pathVal.get_value()).extension().string() == ".qat")) {
        file.addInclude(fs::path(path)
                            .lexically_relative(base)
                            .replace_extension(".hpp")
                            .string());
        file.addSingleLineComment(
            "Brought file " + fs::path(path).lexically_relative(base).string());
      } else if (fs::is_directory(path)) {
        file.addSingleLineComment("Brought folder " + path);
        for (auto mem : fs::path(path)) {
          if (fs::is_regular_file(mem) && (mem.extension() == ".qat")) {
            file.addInclude(mem.lexically_relative(base)
                                .replace_extension(".hpp")
                                .string());
          }
        }
      }
    }
  }
}

nuo::Json BringPaths::toJson() const {
  std::vector<nuo::JsonValue> pths;
  for (auto path : paths) {
    pths.push_back(path.toJson());
  }
  return nuo::Json()
      ._("nodeType", "bringPaths")
      ._("paths", pths)
      ._("visibility", visibility)
      ._("fileRange", fileRange);
}

} // namespace qat::ast