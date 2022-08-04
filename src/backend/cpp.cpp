#include "./cpp.hpp"
#include <fstream>
#include <ios>

namespace qat::cpp {

Content::Content() : value() {}

Content &Content::operator<<(const String &content) {
  value += content;
  return *this;
}

Content &Content::operator<<(const char *content) {
  value += content;
  return *this;
}

String Content::getContent() const { return value; }

void Content::setContent(String val) { value = std::move(val); }

Namespace::Namespace(String _name, Namespace *_parent, bool _isFile)
    : name(std::move(_name)), parent(_parent), isFile(_isFile),
      active(nullptr) {}

String Namespace::getName() const { return name; }

String Namespace::getFullName() const {
  if (parent) {
    return parent->getFullName() + "::" + name;
  } else {
    if (isFile) {
      return "";
    } else {
      return name;
    }
  }
}

bool Namespace::hasParent() const { return parent != nullptr; }

Namespace *Namespace::getActive() { return active ? active : this; }

Namespace *Namespace::addChild(const String &name) {
  children.push_back(new Namespace(name, this, false));
  active = children.back();
  return active;
}

void Namespace::addEnclosedComment(const String &comment) {
  *this << (" /** " + comment + " */ ");
}

void Namespace::addSingleLineComment(const String &comment) {
  *this << ("// " + comment + "\n");
}

void Namespace::closeChild() { active = nullptr; }

File *Namespace::getFile() { // NOLINT(misc-no-recursion)
  if (isFile) {
    return (File *)this;
  } else if (hasParent()) {
    return parent->getFile();
  }
}

File::File(String _path, bool _isHeader)
    : path(std::move(_path)), isHeader(_isHeader), openBlock(false),
      isArrayBracketSyntax(false), Namespace("", nullptr, true) {}

File File::Header(String path) { return {std::move(path), false}; }

File File::Source(String path) { return {std::move(path), true}; }

void File::addInclude(const String &unit) {
  bool exists = false;
  for (auto const &val : includes) {
    if (val == unit) {
      exists = true;
      break;
    }
  }
  for (auto const &val : headerIncludes) {
    if (val == unit) {
      exists = true;
      break;
    }
  }
  if (!exists) {
    includes.push_back(unit);
  }
}

Vec<String> File::getIncludes() const { return includes; }

void File::moveIncludes(File &other) {
  for (auto const &inc : includes) {
    other.addInclude(inc);
  }
  includes.clear();
}

void File::updateHeaderIncludes(const Vec<String> &values) {
  for (auto const &val : values) {
    headerIncludes.push_back(val);
  }
}

void File::addLoopID(String idVal) { loopIndices.push_back(std::move(idVal)); }

String File::getLoopIndex() const { return loopIndices.back(); }

void File::popLastLoopIndex() { loopIndices.pop_back(); }

void File::setOpenBlock(bool val) { openBlock = val; }

bool File::getOpenBlock() const { return openBlock; }

bool File::getArraySyntaxIsBracket() const { return isArrayBracketSyntax; }

void File::setArraySyntax(bool val) { isArrayBracketSyntax = val; }

bool File::isHeaderFile() const { return isHeader; }

bool File::isSourceFile() const { return !isHeader; }

void File::setParent(String val) { parent = std::move(val); }

String File::getParent() { return parent.value_or(""); }

void File::write() {
  String includePart = "// Transpiled by QAT compiler\n\n";
  for (auto const &inc : includes) {
    if (inc.find("<") == 0 && inc.find_last_of('>')) {
      includePart += ("#include " + inc + "\n");
    } else {
      includePart += ("#include \"" + inc + "\"\n");
    }
  }
  includePart += "\n";
  if (parent) {
    includePart += ("// qat lib\nnamespace " + parent.value() + " {\n\n");
  }
  includePart += getContent();
  if (parent) {
    includePart += "\n} // qat lib " + parent.value() + "\n";
  }

  std::fstream outputStream;
  auto         filepath = fs::path(path);
  outputStream.open(filepath, std::ios_base::out);
  if (outputStream.is_open()) {
    outputStream << getContent();
    outputStream.close();
  } else {
  }
}

} // namespace qat::cpp