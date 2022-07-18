#ifndef QAT_AST_ARGUMENT_HPP
#define QAT_AST_ARGUMENT_HPP

#include "../utils/file_range.hpp"
#include "./types/qat_type.hpp"
#include <optional>
#include <string>

namespace qat::AST {

class Argument {
private:
  std::string name;

  utils::FileRange placement;

  QatType *type;

  bool isMember;

  Argument(std::string _name, utils::FileRange _placement, QatType *_type,
           bool _isMember);

public:
  static Argument *Normal(std::string name, utils::FileRange placement,
                          QatType *type);

  static Argument *ForConstructor(std::string name, utils::FileRange placement,
                                  QatType *type, bool isMember);

  std::string getName() const;

  utils::FileRange getFileRange() const;

  QatType *getType();

  bool isTypeMember() const;
};

} // namespace qat::AST

#endif