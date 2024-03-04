#include "./context.hpp"
#include "../cli/color.hpp"
#include "../cli/config.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../sitter.hpp"
#include "../utils/logger.hpp"
#include "../utils/qat_region.hpp"
#include "./value.hpp"
#include "fstream"
#include "qat_module.hpp"
#include "clang/Basic/AddressSpaces.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace qat::ir {

#define Colored(val) (cfg->no_color_mode() ? "" : val)

#define ColoredOr(val, rep) (cfg->no_color_mode() ? rep : val)

CodeProblem::CodeProblem(bool _isError, String _message, Maybe<FileRange> _range)
    : isError(_isError), message(std::move(_message)), range(std::move(_range)) {}

CodeProblem::operator Json() const {
  return Json()
      ._("isError", isError)
      ._("message", message)
      ._("hasRange", range.has_value())
      ._("fileRange", range.has_value() ? (Json)(range.value()) : Json());
}

QatError::QatError() = default;

QatError::QatError(String _message, Maybe<FileRange> _range) : message(_message), fileRange(_range) {}

QatError& QatError::add(String value) {
  message.append(value);
  return *this;
}

QatError& QatError::colored(String value, const char* color) {
  auto* cfg = cli::Config::get();
  message.append(ColoredOr(color, "`") + message + ColoredOr(colors::bold::white, "`"));
  return *this;
}

void QatError::setRange(FileRange range) { fileRange = range; }

Ctx* Ctx::instance = nullptr;

Ctx::Ctx() : llctx(), clangTargetInfo(nullptr), builder(llctx), hasMain(false) {
  SHOW("llctx address: " << &llctx)
  SHOW("Builder llctx address: " << &builder.getContext())
  auto diagnosticEngine =
      clang::DiagnosticsEngine(llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(new clang::DiagnosticIDs()),
                               llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions>(new clang::DiagnosticOptions()));
  auto targetOpts    = std::make_shared<clang::TargetOptions>();
  targetOpts->Triple = cli::Config::get()->get_target_triple();
  clangTargetInfo    = clang::TargetInfo::CreateTargetInfo(diagnosticEngine, targetOpts);
  dataLayout         = llvm::DataLayout(clangTargetInfo->getDataLayoutString());
}

llvm::GlobalValue::LinkageTypes Ctx::getGlobalLinkageForVisibility(VisibilityInfo const& visibInfo) const {
  switch (visibInfo.kind) {
    case VisibilityKind::pub:
      return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
    case VisibilityKind::type:
      return llvm::GlobalValue::LinkageTypes::InternalLinkage;
    case VisibilityKind::folder:
    case VisibilityKind::file:
    case VisibilityKind::lib: {
      if (visibInfo.moduleVal) {
        if (!visibInfo.moduleVal->has_submodules()) {
          return llvm::GlobalValue::LinkageTypes::InternalLinkage;
        }
      }
      return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
    }
    case VisibilityKind::parent: {
      if (visibInfo.typePtr) {
        return llvm::GlobalValue::LinkageTypes::InternalLinkage;
      } else if (visibInfo.moduleVal) {
        if (!visibInfo.moduleVal->has_submodules()) {
          return llvm::GlobalValue::LinkageTypes::InternalLinkage;
        }
      }
      return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
    }
    case VisibilityKind::skill:
      return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
  }
}

String Ctx::highlightWarning(const String& message, const char* color) {
  auto* cfg = cli::Config::get();
  return ColoredOr(color, "`") + message + ColoredOr(colors::bold::white, "`");
}

void Ctx::write_json_result(bool status) const {
  SHOW("Pushing problems")
  Vec<JsonValue> problems;
  for (auto& prob : codeProblems) {
    SHOW("Pushing code problem")
    problems.push_back((Json)prob);
  }
  SHOW("Pushed all code problems")
  Json result;
  SHOW("Setting binary sizes in result")
  Vec<JsonValue> binarySizesJson;
  for (auto binSiz : binarySizes) {
    binarySizesJson.push_back(binSiz);
  }
  SHOW("Creating JSON object for result")
  result._("status", status)
      ._("problems", problems)
      ._("lineCount", lexer::Lexer::lineCount > 0 ? lexer::Lexer::lineCount - 1 : 0)
      ._("lexerTime", lexer::Lexer::timeInMicroSeconds)
      ._("parserTime", parser::Parser::timeInMicroSeconds)
      ._("compilationTime", qatCompileTimeInMs.has_value() ? qatCompileTimeInMs.value() : JsonValue())
      ._("linkingTime", clangAndLinkTimeInMs.has_value() ? clangAndLinkTimeInMs.value() : JsonValue())
      ._("binarySizes", binarySizesJson)
      ._("hasMain", hasMain);
  SHOW("Creating compilation result file")
  std::ofstream output;
  auto          outPath = cli::Config::get()->get_output_path() / "QatCompilationResult.json";
  if (fs::exists(outPath)) {
    fs::remove(outPath);
  }
  output.open(outPath, std::ios_base::out);
  if (output.is_open()) {
    output << result;
    output.close();
  }
  SHOW("compilation result file done")
}

bool Ctx::has_generic_parameter_in_entity(String const& name) const {
  if (lastMainActiveGeneric.empty()) {
    return allActiveGenerics.back().hasGenericParameter(name);
  } else {
    for (auto i = lastMainActiveGeneric.back(); i < allActiveGenerics.size(); i++) {
      if (allActiveGenerics.at(i).hasGenericParameter(name)) {
        return true;
      }
    }
    return false;
  }
}

GenericParameter* Ctx::get_generic_parameter_from_entity(String const& name) const {
  if (lastMainActiveGeneric.empty()) {
    if (allActiveGenerics.back().hasGenericParameter(name)) {
      return allActiveGenerics.back().getGenericParameter(name);
    }
  } else {
    for (auto i = lastMainActiveGeneric.back(); i < allActiveGenerics.size(); i++) {
      if (allActiveGenerics.at(i).hasGenericParameter(name)) {
        return allActiveGenerics.at(i).getGenericParameter(name);
      }
    }
  }
  return nullptr;
}

void Ctx::add_error(ir::Mod* activeMod, String const& message, Maybe<FileRange> fileRange,
                    Maybe<Pair<String, FileRange>> pointTo) {
  while (!ctxMut.try_lock()) {
  }
  threadsWithErrors.insert(std::this_thread::get_id());
  auto* cfg = cli::Config::get();
  if (hasActiveGeneric()) {
    codeProblems.push_back(CodeProblem(true,
                                       "Errors generated while creating generic variant: " + getActiveGeneric().name,
                                       getActiveGeneric().fileRange));
    Logger::get()->errOut << "\n"
                          << Colored(colors::highIntensityBackground::red) << " ERROR " << Colored(colors::cyan)
                          << " --> " << Colored(colors::reset) << getActiveGeneric().fileRange.file.string() << ":"
                          << getActiveGeneric().fileRange.start << "\n"
                          << "Errors while creating generic variant: " << color(getActiveGeneric().name) << "\n"
                          << "\n";
  }
  codeProblems.push_back(CodeProblem(
      true, (hasActiveGeneric() ? ("Creating " + getActiveGeneric().name + " => ") : "") + message, fileRange));
  Logger::get()->errOut << "\n"
                        << Colored(colors::highIntensityBackground::red) << " ERROR " << Colored(colors::reset) << " ";
  Logger::get()->errOut << Colored(colors::bold::white)
                        << (hasActiveGeneric() ? ("Creating " + getActiveGeneric().name + " => ") : "") << message
                        << Colored(colors::reset) << "\n";
  if (fileRange) {
    Logger::get()->errOut << Colored(colors::cyan) << " --> " << Colored(colors::reset)
                          << fileRange.value().file.string() << ":" << fileRange.value().start << " to "
                          << fileRange.value().end;
    print_range_content(fileRange.value(), true, true);
  }
  if (pointTo.has_value()) {
    Logger::get()->errOut << (fileRange.has_value() ? "" : "\n") << Colored(colors::bold::white)
                          << pointTo.value().first << Colored(colors::reset) << "\n"
                          << Colored(colors::cyan) << " --> " << Colored(colors::reset)
                          << pointTo.value().second.file.string() << ":" << pointTo.value().second.start << " to "
                          << pointTo.value().second.end;
    print_range_content(pointTo.value().second, true, false);
  }
  Logger::get()->errOut << "\n";
  if (activeMod && !module_has_errors(activeMod)) {
    if (hasActiveGeneric()) {
      removeActiveGeneric();
    }
    modulesWithErrors.push_back(activeMod);
    for (const auto& modNRange : activeMod->get_brought_mentions()) {
      add_error(modNRange.first, "Error occured in this file", modNRange.second);
    }
  }
  Logger::get()->finish_output();
  ctxMut.unlock();
}

void Ctx::print_range_content(FileRange const& fileRange, bool isError, bool isContentError) const {
  auto cfg = cli::Config::get();
  if (!fs::is_regular_file(fileRange.file)) {
    return;
  }
  auto  lines       = get_range_content(fileRange);
  auto  endLine     = lines.first + lines.second.size();
  usize lineNumSize = std::to_string(lines.first).size();
  if (lineNumSize < std::to_string(endLine).size()) {
    lineNumSize = std::to_string(endLine).size();
  }
  usize lineIndex = 0;
  (isError ? Logger::get()->errOut : Logger::get()->out) << "\n";
  for (auto& lineInfo : lines.second) {
    String lineNum = std::to_string(lines.first + lineIndex);
    while (lineNum.size() < lineNumSize) {
      lineNum += " ";
    }
    (isError ? Logger::get()->errOut : Logger::get()->out) << lineNum << " | " << std::get<0>(lineInfo) << "\n";
    if (std::get<1>(lineInfo) != std::get<2>(lineInfo)) {
      String spacing;
      if (std::get<1>(lineInfo) > 0) {
        spacing.reserve(std::get<1>(lineInfo));
      }
      for (usize i = 0; i < std::get<1>(lineInfo); i++) {
        if (std::get<0>(lineInfo).at(i) == '\t') {
          spacing += "\t";
        } else {
          spacing += " ";
        }
      }
      auto   indicatorCount = std::get<2>(lineInfo) - std::get<1>(lineInfo);
      String indicator(indicatorCount, '^');
      (isError ? Logger::get()->errOut : Logger::get()->out)
          << String(lineNumSize, ' ') << " | " << spacing
          << (isContentError ? Colored(colors::bold::red) : Colored(colors::bold::purple)) << indicator
          << Colored(colors::reset) << "\n";
    }
    lineIndex++;
  }
}

void Ctx::add_exe_path(fs::path path) {
  while (!ctxMut.try_lock()) {
  }
  executablePaths.push_back(path);
  ctxMut.unlock();
}

void Ctx::add_binary_size(usize size) {
  while (!ctxMut.try_lock()) {
  }
  binarySizes.push_back(size);
  ctxMut.unlock();
}

void Ctx::finalise_errors() {
  while (!ctxMut.try_lock()) {
  }
  if (!threadsWithErrors.empty() && (sitter->mainThread == std::this_thread::get_id())) {
    Logger::get()->out.emit();
    Logger::get()->errOut.emit();
    write_json_result(false);
    sitter->destroy();
    QatRegion::destroyAllBlocks();
    ctxMut.unlock();
    std::exit(0);
  } else if (!threadsWithErrors.empty()) {
    ctxMut.unlock();
    pthread_exit(nullptr);
  }
  ctxMut.unlock();
}

void Ctx::Error(ir::Mod* activeMod, const String& message, Maybe<FileRange> fileRange,
                Maybe<Pair<String, FileRange>> pointTo) {
  add_error(activeMod, message, fileRange, pointTo);
  finalise_errors();
}

void Ctx::Errors(ir::Mod* activeMod, Vec<QatError> errors) {
  for (auto& err : errors) {
    add_error(activeMod, err.message, err.fileRange);
  }
  finalise_errors();
}

void Ctx::Error(const String& message, Maybe<FileRange> fileRange, Maybe<Pair<String, FileRange>> pointTo) {
  add_error(nullptr, message, fileRange, pointTo);
  finalise_errors();
}

void Ctx::Errors(Vec<QatError> errors) {
  for (auto& err : errors) {
    add_error(nullptr, err.message, err.fileRange);
  }
  finalise_errors();
}

Pair<usize, Vec<std::tuple<String, u64, u64>>> Ctx::get_range_content(FileRange const& _range) const {
  Vec<std::tuple<String, u64, u64>> result;

  std::ifstream file(_range.file);
  String        line;
  u64           lineCount = 0;
  const usize   startLine = _range.start.line;
  const usize   startChar = _range.start.character;
  const usize   endLine   = _range.end.line;
  const usize   endChar   = _range.end.character;
  usize         firstLine = startLine;
  while (std::getline(file, line)) {
    lineCount++;
    usize i = 0;
    while (line[i] == '\t') {
      i++;
    }
    if ((startLine > 0u) && (lineCount == (startLine - 1))) {
      // Line before the first relevant line in file range
      result.push_back({line, 0u, 0u});
      firstLine = lineCount;
    } else if ((lineCount >= startLine) && (lineCount <= endLine)) {
      // Relevant line
      if (startLine == endLine) {
        // Only one line for the relevant content
        result.push_back({line, startChar, endChar});
      } else if (lineCount == startLine) {
        // First relevant line
        result.push_back({line, startChar, line.size()});
      } else if (lineCount == endLine) {
        // Last relevant line
        result.push_back({line, i, endChar});
      } else {
        // Relevant lines in the middle
        result.push_back({line, i, line.size()});
      }
    } else if ((endLine < UINT64_MAX) && (lineCount == (endLine + 1))) {
      // Line after the last relevant line in file range
      result.push_back({line, 0u, 0u});
      break;
    }
  }
  return {firstLine, result};
}

void Ctx::Warning(const String& message, const FileRange& fileRange) {
  while (!ctxMut.try_lock()) {
  }
  if (hasActiveGeneric()) {
    getActiveGeneric().warningCount++;
  }
  codeProblems.push_back(CodeProblem(
      false, (hasActiveGeneric() ? ("Creating " + joinActiveGenericNames(false) + " => ") : "") + message, fileRange));
  auto* cfg = cli::Config::get();
  Logger::get()->out << "\n"
                     << Colored(colors::highIntensityBackground::purple) << " WARNING " << Colored(colors::cyan)
                     << " --> " << Colored(colors::reset) << fileRange.file.string() << ":" << fileRange.start.line
                     << ":" << fileRange.start.character << Colored(colors::bold::white) << "\n"
                     << (hasActiveGeneric() ? ("Creating " + joinActiveGenericNames(true) + " => ") : "") << message
                     << Colored(colors::reset) << "\n";
  print_range_content(fileRange, false, false);
  ctxMut.unlock();
}

} // namespace qat::ir
