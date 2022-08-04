#ifndef QAT_BACKEND_CPP_HPP
#define QAT_BACKEND_CPP_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"

namespace qat::cpp {

class File;

class Content {
private:
  String value;

protected:
  friend class File;
  auto setContent(String val) -> void;

public:
  Content();

  Content &operator<<(const String &content);

  Content &operator<<(const char *content);

  useit String getContent() const;
};

class File;

class Namespace : public Content {
private:
  String name;

  Namespace *parent;

  Namespace *active;

  Vec<Namespace *> children;

  bool isFile;

public:
  Namespace(String _name, Namespace *parent, bool isFile);

  useit String     getName() const;
  useit String     getFullName() const;
  useit bool       hasParent() const;
  useit Namespace *getActive();
  useit Namespace *addChild(const String &name);
  useit File      *getFile();
  void             addEnclosedComment(const String &name);
  void             addSingleLineComment(const String &name);
  void             closeChild();
};

class File : public Namespace {
private:
  File(String _path, bool _isHeader);

  bool          isHeader;
  String        path;
  Vec<String>   includes;
  Vec<String>   headerIncludes;
  Vec<String>   loopIndices;
  Maybe<String> parent;
  bool          openBlock;
  bool          isArrayBracketSyntax;

public:
  useit static File Header(String path);
  useit static File Source(String path);
  void              addInclude(const String &unit);
  void              updateHeaderIncludes(const Vec<String> &values);
  void              moveIncludes(File &other);
  void              addLoopID(String IDVal);
  void              setParent(String val);
  void              popLastLoopIndex();
  void              setOpenBlock(bool val);
  void              setArraySyntax(bool val);
  void              write();
  useit Vec<String> getIncludes() const;
  useit bool        isHeaderFile() const;
  useit bool        isSourceFile() const;
  useit String      getLoopIndex() const;
  useit bool        getArraySyntaxIsBracket() const;
  useit bool        getOpenBlock() const;
  useit String      getParent();
};

} // namespace qat::cpp

#endif