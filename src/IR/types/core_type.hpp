#ifndef QAT_IR_TYPES_CORE_TYPE_HPP
#define QAT_IR_TYPES_CORE_TYPE_HPP

#include "../../utils/visibility.hpp"
#include "../member_function.hpp"
#include "../static_member.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"
#include <string>
#include <utility>
#include <vector>

namespace qat::IR {

/**
 *  This represents a core type in the language
 *
 */
class CoreType : public QatType {
  friend class MemberFunction;

public:
  class Member {
  public:
    Member(String _name, QatType *_type, bool _variability,
           const utils::VisibilityInfo &_visibility)
        : name(std::move(_name)), type(_type), visibility(_visibility),
          variability(_variability) {}

    String                name;
    QatType              *type;
    utils::VisibilityInfo visibility;
    bool                  variability;
  };

private:
  String              name;
  QatModule          *parent;
  Vec<Member *>       members;
  Vec<StaticMember *> staticMembers;

  Vec<MemberFunction *>   memberFunctions; // Normal
  Vec<MemberFunction *>   constructors;    // Constructors
  Vec<MemberFunction *>   fromConvertors;  // From Convertors
  Vec<MemberFunction *>   toConvertors;    // To Convertors
  Vec<MemberFunction *>   staticFunctions; // Static
  MemberFunction         *destructor;      // Destructor
  Maybe<MemberFunction *> copyConstructor; // Copy constructor
  Maybe<MemberFunction *> moveConstructor; // Move constructor

  utils::VisibilityInfo visibility;

  // TODO - Add support for extension functions

public:
  CoreType(QatModule *mod, String _name, Vec<Member *> _members,
           const utils::VisibilityInfo &_visibility, llvm::LLVMContext &ctx);

  useit Maybe<usize>          getIndexOf(const String &member) const;
  useit bool                  hasMember(const String &member) const;
  useit String                getFullName() const;
  useit String                getName() const;
  useit u64                   getMemberCount() const;
  useit Member               *getMemberAt(u64 index);
  useit String                getMemberNameAt(u64 index) const;
  useit QatType              *getTypeOfMember(const String &member) const;
  useit bool                  hasStatic(const String &name) const;
  useit bool                  hasMemberFunction(const String &fnName) const;
  useit MemberFunction       *getMemberFunction(const String &fnName) const;
  useit bool                  hasStaticFunction(const String &fnName) const;
  useit const MemberFunction *getStaticFunction(const String &fnName) const;
  useit bool                  hasCopyConstructor() const;
  useit bool                  hasMoveConstructor() const;
  useit utils::VisibilityInfo getVisibility() const;
  useit QatModule            *getParent();
  useit nuo::Json toJson() const override;
  useit TypeKind  typeKind() const override;
  useit String    toString() const override;
  void addStaticMember(const String &name, QatType *type, bool variability,
                       Value *initial, const utils::VisibilityInfo &visibility,
                       llvm::LLVMContext &ctx);
};

} // namespace qat::IR

#endif