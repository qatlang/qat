#include "./qat_sitter.hpp"
#include "./show.hpp"
#include <filesystem>

namespace qat {

QatSitter::QatSitter()
    : Context(new IR::Context()), Parser(new parser::Parser()),
      Lexer(new lexer::Lexer()) {}

void QatSitter::init() {
  auto config = cli::Config::get();
  if (config->isCompile()) {
    if (config->getPaths().size() == 1) {
      auto path = config->getPaths().at(0);
      if (fs::is_regular_file(path)) {
        SHOW("Lexer Setup")
        Lexer->changeFile(path.string());
        Lexer->analyse();
        SHOW("Parser setTokens")
        Parser->setTokens(Lexer->get_tokens());
        SHOW("Parsing")
        auto nodes = Parser->parse();
        SHOW("Parsing complete")
        //
        // {
        //   // QAT IR
        //   SHOW("Creating QatModule")
        //   auto *mod = new IR::QatModule(
        //       path.filename().string(), fs::absolute(path),
        //       IR::ModuleType::file, utils::VisibilityInfo::pub());
        //   SHOW("QatModule created")
        //   modules.push_back(mod);
        //   SHOW("Pushed to modules")
        //   top_modules.push_back(mod);
        //   SHOW("Pushed to Top modules")
        //   Context->mod = mod;
        //   SHOW("QatModule initialised in ctx")
        //   SHOW("Number of nodes " << nodes.size())
        //   for (std::size_t i = 0; i < nodes.size(); i++) {
        //     SHOW("Node at " << (i + 1))
        //     nodes.at(i)->emit(Context);
        //     i++;
        //   }
        //   SHOW("Module Output")
        //   return;
        // }

        // {
        //   // CPP
        //   std::string headerFileName =
        //       path.replace_extension(".hpp").filename().string();
        //   auto headerFile = backend::cpp::File::Header(
        //       path.replace_extension(".hpp").string());
        //   auto sourceFile = backend::cpp::File::Source(
        //       path.replace_extension(".cpp").string());
        //   sourceFile.addInclude("./" + headerFileName);
        //   for (auto node : nodes) {
        //     node->emitCPP(headerFile, true);
        //   }
        //   sourceFile.updateHeaderIncludes(headerFile.getIncludes());
        //   for (auto node : nodes) {
        //     node->emitCPP(sourceFile, false);
        //   }
        //   headerFile.write();
        //   sourceFile.write();
        //   return;
        // }

        {
          // JSON
          std::fstream jsonStream;
          auto filepath = path.replace_extension(".json").string();
          jsonStream.open(filepath, std::ios_base::out);
          if (jsonStream.is_open()) {
            jsonStream << "{\n\"contents\" : [";
            for (std::size_t j = 0; j < nodes.size(); j++) {
              auto nJson = nodes.at(j)->toJson();
              jsonStream << nJson.toString();
              if (j != (nodes.size() - 1)) {
                jsonStream << ",\n";
              }
            }
            jsonStream << "]\n}";
            jsonStream.close();
          }
        }
      }
    }
    // for (auto path : config->get_paths()) {
    //   auto mods = this->handle_top_modules(path);
    //   for (auto mod : mods) {
    //     top_modules.push_back(mod);
    //   }
    // }
  }
}

std::vector<IR::QatModule *> QatSitter::handle_top_modules(fs::path path) {
  std::vector<IR::QatModule *> result;
  if (fs::is_directory(path)) {
    auto libpath = path / "lib.qat";
    if (fs::exists(libpath) && fs::is_regular_file(libpath)) {
      result.push_back(IR::QatModule::Create("lib.qat", fs::absolute(libpath),
                                             IR::ModuleType::file,
                                             utils::VisibilityInfo::pub()));
    } else {
      for (auto sub : path) {
        auto subres = handle_top_modules(sub);
        for (auto submod : subres) {
          result.push_back(submod);
        }
      }
    }
  } else if (fs::is_regular_file(path)) {
    auto mod = IR::QatModule::Create(path.string(), fs::absolute(path),
                                     IR::ModuleType::file,
                                     utils::VisibilityInfo::pub());
    modules.push_back(mod);
    top_modules.push_back(mod);

    Lexer->changeFile(path.string());
    Lexer->analyse();
    Parser->setTokens(Lexer->get_tokens());
    auto nodes = Parser->parse();
    Context->mod = mod;
    for (auto node : nodes) {
      node->emit(Context);
    }
  }
  return result;
}

QatSitter::~QatSitter() {
  for (auto mod : modules) {
    delete mod;
  }
  modules.clear();
  top_modules.clear();
  delete Context;
  Context = nullptr;
  delete Parser;
  Parser = nullptr;
  delete Lexer;
  Lexer = nullptr;
}

} // namespace qat