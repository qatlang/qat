#ifndef QAT_AST_DEFINE_CORE_HPP
#define QAT_AST_DEFINE_CORE_HPP

#include "./expression.hpp"
#include "./types/qat_type.hpp"
#include "member_parent_like.hpp"
#include "meta_info.hpp"
#include "node.hpp"
#include "types/generic_abstract.hpp"

namespace qat::ast {

class DefineCoreType final : public IsEntity, public Commentable, public MemberParentLike {
  friend class ir::GenericCoreType;

public:
  struct Member {
    Member(Type* _type, Identifier _name, bool _variability, Maybe<VisibilitySpec> _visibSpec,
           Maybe<Expression*> _expression, FileRange _fileRange)
        : type(_type), name(_name), variability(_variability), visibSpec(_visibSpec), expression(_expression),
          fileRange(_fileRange) {}

    useit static inline Member* create(Type* _type, Identifier _name, bool _variability,
                                       Maybe<VisibilitySpec> _visibSpec, Maybe<Expression*> _expression,
                                       FileRange _fileRange) {
      return std::construct_at(OwnNormal(Member), _type, _name, _variability, _visibSpec, _expression, _fileRange);
    }

    Type*                 type;
    Identifier            name;
    bool                  variability;
    Maybe<VisibilitySpec> visibSpec;
    Maybe<Expression*>    expression;
    FileRange             fileRange;

    useit Json to_json() const;
  };

  // Static member representation in the AST
  struct StaticMember {
    StaticMember(Type* _type, Identifier _name, bool _variability, Expression* _value, Maybe<VisibilitySpec> _visibSpec,
                 FileRange _fileRange)
        : type(_type), name(_name), variability(_variability), value(_value), visibSpec(_visibSpec),
          fileRange(_fileRange) {}

    useit static inline StaticMember* create(Type* _type, Identifier _name, bool _variability, Expression* _value,
                                             Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
      return std::construct_at(OwnNormal(StaticMember), _type, _name, _variability, _value, _visibSpec, _fileRange);
    }

    Type*                 type;
    Identifier            name;
    bool                  variability;
    Expression*           value;
    Maybe<VisibilitySpec> visibSpec;
    FileRange             fileRange;

    useit Json to_json() const;
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
  mutable ir::StructType*        resultCoreType = nullptr;
  mutable Vec<ir::OpaqueType*>   opaquedTypes;
  useit bool                     hasOpaque() const;
  void                           setOpaque(ir::OpaqueType* opq) const;
  useit ir::OpaqueType*           get_opaque() const;
  void                            unsetOpaque() const;
  mutable ir::GenericCoreType*    genericCoreType = nullptr;
  mutable Vec<ir::GenericToFill*> genericsToFill;
  mutable Maybe<bool>             checkResult;
  mutable Maybe<bool>             isPackedStruct;
  mutable ir::EntityState*        entityState = nullptr;

public:
  DefineCoreType(Identifier _name, Maybe<PrerunExpression*> _checker, Maybe<VisibilitySpec> _visibSpec,
                 FileRange _fileRange, Vec<ast::GenericAbstractType*> _generics, Maybe<PrerunExpression*> _constraint,
                 Maybe<MetaInfo> _metaInfo)
      : IsEntity(_fileRange), name(_name), checker(_checker), visibSpec(_visibSpec), metaInfo(_metaInfo),
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

  void create_opaque(ir::Mod* mod, ir::Ctx* irCtx);
  void create_type(ir::StructType** resultTy, ir::Mod* mod, ir::Ctx* irCtx) const;
  void setup_type(ir::Mod* mod, ir::Ctx* irCtx);
  void do_define(ir::StructType* resultTy, ir::Mod* mod, ir::Ctx* irCtx);

  useit inline bool hasTrivialCopy() { return trivialCopy.has_value(); }
  inline void       setTrivialCopy(FileRange range) { trivialCopy = std::move(range); }
  useit inline bool hasTrivialMove() { return trivialMove.has_value(); }
  inline void       setTrivialMove(FileRange range) { trivialMove = std::move(range); }

  useit bool            isGeneric() const;
  useit bool            has_default_constructor() const;
  useit bool            has_destructor() const;
  useit bool            has_copy_constructor() const;
  useit bool            has_move_constructor() const;
  useit bool            has_copy_assignment() const;
  useit bool            has_move_assignment() const;
  useit bool            is_define_core_type() const final { return true; }
  useit DefineCoreType* as_define_core_type() final { return this; }

  void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;
  void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;
  void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

  void emit(ir::Ctx* irCtx);
  void do_emit(ir::StructType* resultTy, ir::Ctx* irCtx);

  useit Json     to_json() const final;
  useit NodeType nodeType() const final { return NodeType::DEFINE_CORE_TYPE; }
  ~DefineCoreType() final;
};

} // namespace qat::ast

#endif