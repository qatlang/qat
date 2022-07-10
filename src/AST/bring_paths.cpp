#include "./bring_paths.hpp"
#include <filesystem>

namespace qat {
namespace AST {

BringPaths::BringPaths(std::vector<StringLiteral> _paths,
                       utils::VisibilityInfo _visibility,
                       utils::FilePlacement _filePlacement)
    : paths(_paths), visibility(_visibility), Sentence(_filePlacement) {}

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
        ctx->throw_error("Cannot bring this file type", pathstr.file_placement);
      }
    } else {
      ctx->throw_error("The path provided does not exist: " + path +
                           " and cannot be brought in.",
                       pathstr.file_placement);
    }
  }
  return nullptr;
}

void BringPaths::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (isHeader) {
    auto base = file_placement.file;
    for (auto pathVal : paths) {
      auto path = pathVal.get_value();
      if (fs::is_regular_file(path) &&
          (fs::path(pathVal.get_value()).extension().string() == ".qat")) {
        file.addInclude(
            fs::path(path).lexically_relative(base).replace_extension(".hpp").string());
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

backend::JSON BringPaths::toJSON() const {
  std::vector<backend::JSON> pths;
  for (auto path : paths) {
    pths.push_back(path.toJSON());
  }
  return backend::JSON()
      ._("nodeType", "bringPaths")
      ._("paths", pths)
      ._("visibility", visibility)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat