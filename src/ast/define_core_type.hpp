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
#include "member_parent_like.hpp"
#include "meta_info.hpp"
#include "node.hpp"
#include "types/generic_abstract.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat::ast {

class DefineCoreType final : public Node, public Commentable, public MemberParentLike {
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
  Identifier               name;
  Maybe<PrerunExpression*> checker;
  Vec<Member*>             members;
  Vec<StaticMember*>       staticMembers;
  Maybe<FileRange>         trivialCopy;
  Maybe<FileRange>         trivialMove;
  Maybe<VisibilitySpec>    visibSpec;
  Maybe<MetaInfo>          metaInfo;

  Vec<ast::GenericAbstractType*> generics;
  Maybe<PrerunExpression*>       constraint;
  mutable IR::CoreType*          resultCoreType = nullptr;
  mutable Vec<IR::OpaqueType*>   opaquedTypes;
  useit bool                     hasOpaque() const;
  void                           setOpaque(IR::OpaqueType* opq) const;
  useit IR::OpaqueType*           getOpaque() const;
  void                            unsetOpaque() const;
  mutable IR::GenericCoreType*    genericCoreType = nullptr;
  mutable Vec<IR::GenericToFill*> genericsToFill;
  mutable Maybe<bool>             checkResult;
  mutable Maybe<bool>             isPackedStruct;

public:
  DefineCoreType(Identifier _name, Maybe<PrerunExpression*> _checker, Maybe<VisibilitySpec> _visibSpec,
                 FileRange _fileRange, Vec<ast::GenericAbstractType*> _generics, Maybe<PrerunExpression*> _constraint,
                 Maybe<MetaInfo> _metaInfo)
      : Node(_fileRange), name(_name), checker(_checker), visibSpec(_visibSpec), metaInfo(_metaInfo),
        generics(_generics), constraint(_constraint) {}

  useit static inline DefineCoreType* create(Identifier _name, Maybe<PrerunExpression*> _checker,
                                             Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange,
                                             Vec<ast::GenericAbstractType*> _generics,
                                             Maybe<PrerunExpression*> _constraint, Maybe<MetaInfo> _metaInfo) {
    return std::construct_at(OwnNormal(DefineCoreType), _name, _checker, _visibSpec, _fileRange, _generics, _constraint,
                             _metaInfo);
  }

  COMMENTABLE_FUNCTIONS

  void addMember(Member* mem);
  void addStaticMember(StaticMember* stm);

  void createModule(IR::Context* ctx) const final;
  void createType(IR::CoreType** resultTy, IR::Context* ctx) const;
  void defineType(IR::Context* ctx) final;
  void define(IR::Context* ctx) final;
  void do_define(IR::CoreType** resultTy, IR::Context* ctx);

  useit inline bool hasTrivialCopy() { return trivialCopy.has_value(); }
  inline void       setTrivialCopy(FileRange range) { trivialCopy = std::move(range); }
  useit inline bool hasTrivialMove() { return trivialMove.has_value(); }
  inline void       setTrivialMove(FileRange range) { trivialMove = std::move(range); }

  useit bool            isGeneric() const;
  useit bool            hasDefaultConstructor() const;
  useit bool            hasDestructor() const;
  useit bool            hasCopyConstructor() const;
  useit bool            hasMoveConstructor() const;
  useit bool            hasCopyAssignment() const;
  useit bool            hasMoveAssignment() const;
  useit bool            is_define_core_type() const final { return true; }
  useit DefineCoreType* as_define_core_type() final { return this; }
  useit IR::Value* emit(IR::Context* ctx) final;
  useit IR::Value* do_emit(IR::CoreType* resultTy, IR::Context* ctx);
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::DEFINE_CORE_TYPE; }
  ~DefineCoreType() final;
};

} // namespace qat::ast

#endif