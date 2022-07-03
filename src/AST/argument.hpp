#ifndef QAT_AST_ARGUMENT_HPP
#define QAT_AST_ARGUMENT_HPP

#include "../utils/file_placement.hpp"
#include "./types/qat_type.hpp"
#include <optional>
#include <string>

namespace qat {
namespace AST {

class Argument {
private:
  std::string name;

  utils::FilePlacement placement;

  QatType *type;

  bool isMember;

  Argument(std::string _name, utils::FilePlacement _placement, QatType *_type,
           bool _isMember);

public:
  static Argument *Normal(std::string name, utils::FilePlacement placement,
                          QatType *type);

  static Argument *ForConstructor(std::string name,
                                  utils::FilePlacement placement, QatType *type,
                                  bool isMember);

  std::string getName() const;

  utils::FilePlacement getFilePlacement() const;

  QatType *getType();

  bool isTypeMember() const;
};

} // namespace AST
} // namespace qat

#endif