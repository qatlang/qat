#include "./cpp.hpp"
#include <filesystem>
#include <fstream>
#include <ios>

namespace qat::cpp {

Content::Content() : value() {}

Content &Content::operator<<(std::string content) {
  value += content;
  return *this;
}

Content &Content::operator<<(const char *content) {
  value += content;
  return *this;
}

std::string Content::getContent() const { return value; }

void Content::setContent(std::string val) { value = val; }

Namespace::Namespace(std::string _name, Namespace *_parent, bool _isFile)
    : name(_name), parent(_parent), isFile(_isFile), children(), Content() {}

std::string Namespace::getName() const { return name; }

std::string Namespace::getFullName() const {
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

Namespace *Namespace::addChild(std::string name) {
  children.push_back(new Namespace(name, this, false));
  active = children.back();
  return active;
}

void Namespace::addEnclosedComment(const std::string comment) {
  *this << (" /** " + comment + " */ ");
}

void Namespace::addSingleLineComment(const std::string comment) {
  *this << ("// " + comment + "\n");
}

void Namespace::closeChild() { active = nullptr; }

File *Namespace::getFile() {
  if (isFile) {
    return (File *)this;
  } else if (hasParent()) {
    return parent->getFile();
  }
}

File::File(std::string _path, bool _isHeader)
    : path(_path), isHeader(_isHeader), Namespace("", nullptr, true) {}

File File::Header(std::string path) { return File(path, false); }

File File::Source(std::string path) { return File(path, true); }

void File::addInclude(const std::string unit) {
  bool exists = false;
  for (auto val : includes) {
    if (val == unit) {
      exists = true;
      break;
    }
  }
  for (auto val : headerIncludes) {
    if (val == unit) {
      exists = true;
      break;
    }
  }
  if (!exists) {
    includes.push_back(unit);
  }
}

std::vector<std::string> File::getIncludes() const { return includes; }

void File::moveIncludes(File &other) {
  for (auto inc : includes) {
    other.addInclude(inc);
  }
  includes.clear();
}

void File::updateHeaderIncludes(std::vector<std::string> values) {
  for (auto val : values) {
    headerIncludes.push_back(val);
  }
}

void File::addLoopID(std::string ID) { loopIndices.push_back(ID); }

std::string File::getLoopIndex() const { return loopIndices.back(); }

void File::popLastLoopIndex() { loopIndices.pop_back(); }

void File::setOpenBlock(bool val) { openBlock = val; }

bool File::getOpenBlock() const { return openBlock; }

bool File::isHeaderFile() const { return isHeader; }

bool File::isSourceFile() const { return !isHeader; }

void File::write() {
  std::string includePart;
  for (auto inc : includes) {
    if (inc.find("<") == 0 && inc.find_last_of('>')) {
      includePart += ("#include " + inc + "\n");
    } else {
      includePart += ("#include \"" + inc + "\"\n");
    }
  }
  setContent(includePart + "\n" + getContent());

  std::fstream outputStream;
  auto filepath = std::filesystem::path(path);
  outputStream.open(filepath, std::ios_base::out);
  if (outputStream.is_open()) {
    outputStream << getContent();
    outputStream.close();
  }
}

} // namespace qat::cpp