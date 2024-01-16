#ifndef QAT_AST_DEFINE_CORE_HPP
#define QAT_AST_DEFINE_CORE_HPP

#include "../utils/visibility.hpp"
#include "./convertor.hpp"
#include "./expression.hpp"
#include "./member_function.hpp"
#include "./operator_function.hpp"
#include "./types/qat_type.hpp"
#include "constructor.hpp"
#include "destructor.hpp"
#include "node.hpp"
#include "types/generic_abstract.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat::ast {

class DefineCoreType final : public Node, public Commentable {
  friend class IR::GenericCoreType;

public:
  struct Member {
    Member(QatType* _type, Identifier _name, bool _variability, Maybe<VisibilitySpec> _visibSpec,
           Maybe<Expression*> _expression, FileRange _fileRange)
        : type(_type), name(_name), variability(_variability), visibSpec(_visibSpec), expression(_expression),
          fileRange(_fileRange) {}

    useit static inline Member* create(QatType* _type, Identifier _name, bool _variability,
                                       Maybe<VisibilitySpec> _visibSpec, Maybe<Expression*> _expression,
                                       FileRange _fileRange) {
      return std::construct_at(OwnNormal(Member), _type, _name, _variability, _visibSpec, _expression, _fileRange);
    }

    QatType*              type;
    Identifier            name;
    bool                  variability;
    Maybe<VisibilitySpec> visibSpec;
    Maybe<Expression*>    expression;
    FileRange             fileRange;

    useit Json toJson() const;
  };

  // Static member representation in the AST
  struct StaticMember {
    StaticMember(QatType* _type, Identifier _name, bool _variability, Expression* _value,
                 Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
        : type(_type), name(_name), variability(_variability), value(_value), visibSpec(_visibSpec),
          fileRange(_fileRange) {}

    useit static inline StaticMember* create(QatType* _type, Identifier _name, bool _variability, Expression* _value,
                                             Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
      return std::construct_at(OwnNormal(StaticMember), _type, _name, _variability, _value, _visibSpec, _fileRange);
    }

    QatType*              type;
    Identifier            name;
    bool                  variability;
    Expression*           value;
    Maybe<VisibilitySpec> visibSpec;
    FileRange             fileRange;

    useit Json toJson() const;
  };

private:
  Identifier                     name;
  Maybe<PrerunExpression*>       checker;
  bool                           isPacked;
  Vec<Member*>                   members;
  Vec<StaticMember*>             staticMembers;
  Vec<MemberDefinition*>         memberDefinitions;
  Vec<ConvertorDefinition*>      convertorDefinitions;
  Vec<OperatorDefinition*>       operatorDefinitions;
  Vec<ConstructorDefinition*>    constructorDefinitions;
  Maybe<FileRange>               trivialCopy;
  Maybe<FileRange>               trivialMove;
  mutable ConstructorDefinition* defaultConstructor   = nullptr;
  mutable ConstructorDefinition* copyConstructor      = nullptr;
  mutable ConstructorDefinition* moveConstructor      = nullptr;
  mutable OperatorDefinition*    copyAssignment       = nullptr;
  mutable OperatorDefinition*    moveAssignment       = nullptr;
  mutable DestructorDefinition*  destructorDefinition = nullptr;
  Maybe<VisibilitySpec>          visibSpec;

  Vec<ast::GenericAbstractType*> generics;
  Maybe<PrerunExpression*>       constraint;
  mutable Vec<IR::CoreType*>     coreTypes;
  void                           setCoreType(IR::CoreType*) const;
  void                           unsetCoreType() const;
  mutable Vec<IR::OpaqueType*>   opaquedTypes;
  useit bool                     hasOpaque() const;
  void                           setOpaque(IR::OpaqueType* opq) const;
  useit IR::OpaqueType*           getOpaque() const;
  void                            unsetOpaque() const;
  mutable IR::GenericCoreType*    genericCoreType = nullptr;
  mutable Vec<IR::GenericToFill*> genericsToFill;
  mutable Maybe<bool>             checkResult;

public:
  DefineCoreType(Identifier _name, Maybe<PrerunExpression*> _checker, Maybe<VisibilitySpec> _visibSpec,
                 FileRange _fileRange, Vec<ast::GenericAbstractType*> _generics, Maybe<PrerunExpression*> _constraint,
                 bool _isPacked = false)
      : Node(_fileRange), name(_name), checker(_checker), isPacked(_isPacked), visibSpec(_visibSpec),
        generics(_generics), constraint(_constraint) {}

  useit static inline DefineCoreType* create(Identifier _name, Maybe<PrerunExpression*> _checker,
                                             Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange,
                                             Vec<ast::GenericAbstractType*> _generics,
                                             Maybe<PrerunExpression*> _constraint, bool _isPacked) {
    return std::construct_at(OwnNormal(DefineCoreType), _name, _checker, _visibSpec, _fileRange, _generics, _constraint,
                             _isPacked);
  }

  COMMENTABLE_FUNCTIONS

  void addMember(Member* mem);
  void addStaticMember(StaticMember* stm);
  void addMemberDefinition(MemberDefinition* mdef);
  void addConvertorDefinition(ConvertorDefinition* cdef);
  void addConstructorDefinition(ConstructorDefinition* cdef);
  void addOperatorDefinition(OperatorDefinition* odef);
  void setDestructorDefinition(DestructorDefinition* ddef);
  void setDefaultConstructor(ConstructorDefinition* cDef);
  void setCopyConstructor(ConstructorDefinition* cDef);
  void setMoveConstructor(ConstructorDefinition* cDef);
  void setCopyAssignment(OperatorDefinition* mDef);
  void setMoveAssignment(OperatorDefinition* mDef);
  void createType(IR::Context* ctx) const;
  void defineType(IR::Context* ctx) final;
  void define(IR::Context* ctx) final;

  useit inline bool hasTrivialCopy() { return trivialCopy.has_value(); }
  inline void       setTrivialCopy(FileRange range) { trivialCopy = std::move(range); }
  useit inline bool hasTrivialMove() { return trivialMove.has_value(); }
  inline void       setTrivialMove(FileRange range) { trivialMove = std::move(range); }

  useit bool isGeneric() const;
  useit bool hasDefaultConstructor() const;
  useit bool hasDestructor() const;
  useit bool hasCopyConstructor() const;
  useit bool hasMoveConstructor() const;
  useit bool hasCopyAssignment() const;
  useit bool hasMoveAssignment() const;
  useit IR::CoreType* getCoreType() const;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::DEFINE_CORE_TYPE; }
  ~DefineCoreType() final;
};

} // namespace qat::ast

#endif