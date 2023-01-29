#include "./const_generic.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/unsigned_literal.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

ConstGeneric::ConstGeneric(usize _index, Identifier _name, QatType* _expTy, Maybe<ast::ConstantExpression*> _defaultVal,
                           FileRange _range)
    : GenericAbstractType(_index, std::move(_name), GenericKind::constGeneric, std::move(_range)), expTy(_expTy),
      defaultValueAST(_defaultVal) {}

ConstGeneric* ConstGeneric::get(usize _index, Identifier _name, QatType* _expTy,
                                Maybe<ast::ConstantExpression*> _defaultVal, FileRange _range) {
  return new ConstGeneric(_index, std::move(_name), _expTy, _defaultVal, std::move(_range));
}

bool ConstGeneric::hasDefault() const { return defaultValueAST.has_value(); }

IR::ConstantValue* ConstGeneric::getDefault() const { return defaultValue; }

void ConstGeneric::emit(IR::Context* ctx) const {
  expressionType = expTy->emit(ctx);
  if (!expressionType->canBeConstGeneric()) {
    ctx->Error("The provided type is not qualified to be used for a const generic expression", expTy->fileRange);
  }
  if (hasDefault()) {
    auto* astVal = defaultValueAST.value();
    if (astVal->nodeType() == NodeType::integerLiteral &&
        (expressionType->isInteger() || expressionType->isUnsignedInteger())) {
      ((ast::IntegerLiteral*)astVal)
          ->setType(expressionType->isInteger() ? (IR::QatType*)expressionType->asInteger()
                                                : (IR::QatType*)expressionType->asUnsignedInteger());
    } else if (astVal->nodeType() == NodeType::unsignedLiteral && expressionType->isUnsignedInteger()) {
      ((ast::UnsignedLiteral*)astVal)->setType(expressionType->asUnsignedInteger());
    }
    defaultValue = astVal->emit(ctx);
    if (defaultValue) {
      if (!defaultValue->getType()->isSame(expressionType)) {
        ctx->Error("The expected type for the const generic expression is " +
                       ctx->highlightError(expressionType->toString()) + " but the provided expression is of type " +
                       ctx->highlightError(defaultValue->getType()->toString()),
                   astVal->fileRange);
      }
    } else {
      ctx->Error("No const expression generated from the default expression provided", astVal->fileRange);
    }
  }
}

IR::QatType* ConstGeneric::getType() const { return expressionType; }

IR::ConstantValue* ConstGeneric::getConstant() const { return expressionValue ? expressionValue : defaultValue; }

bool ConstGeneric::isSet() const { return (expressionValue != nullptr) || (defaultValue != nullptr); }

void ConstGeneric::setExpression(IR::ConstantValue* exp) const { expressionValue = exp; }

void ConstGeneric::unset() const { expressionValue = nullptr; }

IR::ConstGeneric* ConstGeneric::toIR() const { return IR::ConstGeneric::get(name, getConstant(), range); }

ConstGeneric::~ConstGeneric() {}

} // namespace qat::ast