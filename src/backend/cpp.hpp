#ifndef QAT_BACKEND_CPP_HPP
#define QAT_BACKEND_CPP_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace qat::cpp {

class File;

class Content {
private:
  std::string value;

protected:
  friend class File;
  void setContent(std::string val);

public:
  Content();

  void operator+=(std::string content);

  void operator+=(const char *content);

  std::string getContent() const;
};

class File;

class Namespace : public Content {
private:
  std::string name;

  Namespace *parent;

  Namespace *active;

  std::vector<Namespace *> children;

  bool isFile;

public:
  Namespace(std::string _name, Namespace *parent, bool isFile);

  std::string getName() const;

  std::string getFullName() const;

  bool hasParent() const;

  Namespace *getActive();

  Namespace *addChild(const std::string name);

  void addEnclosedComment(const std::string name);

  void addSingleLineComment(const std::string name);

  void closeChild();

  File *getFile();
};

class File : public Namespace {
private:
  bool isHeader;

  std::string path;

  std::vector<std::string> includes;

  std::vector<std::string> headerIncludes;

  std::vector<std::string> loopIndices;

  File(std::string _path, bool _isHeader);

  bool openBlock;

public:
  static File Header(std::string path);

  static File Source(std::string path);

  void addInclude(const std::string unit);

  std::vector<std::string> getIncludes() const;

  void updateHeaderIncludes(std::vector<std::string> values);

  void moveIncludes(File &other);

  bool isHeaderFile() const;

  bool isSourceFile() const;

  void addLoopID(std::string ID);

  std::string getLoopIndex() const;

  void popLastLoopIndex();

  void setOpenBlock(bool val);

  bool getOpenBlock() const;

  void write();
};

} // namespace qat::cpp

#endif