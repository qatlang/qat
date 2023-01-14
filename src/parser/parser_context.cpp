#include "./parser_context.hpp"
#include "../ast/types/generic_abstract.hpp"

namespace qat::parser {

ParserContext::ParserContext() = default;

bool ParserContext::hasNamedAbstractGeneric(const String& name) const {
  for (auto* tname : generics) {
    if (tname->getName() == name) {
      return true;
    }
  }
  return false;
}

void ParserContext::addAbstractGeneric(ast::GenericAbstractType* genericType) { generics.push_back(genericType); }

void ParserContext::removeNamedGenericAbstract(const String& name) {
  for (auto temp = generics.begin(); temp != generics.end(); temp++) {
    if ((*temp)->getName() == name) {
      generics.erase(temp);
      break;
    }
  }
}

ast::GenericAbstractType* ParserContext::getNamedAbstractGeneric(const String& name) {
  for (auto* temp : generics) {
    if (temp->getName() == name) {
      return temp;
    }
  }
  return nullptr;
}

// FIXME - Change to generic linked
ast::GenericAbstractType* ParserContext::duplicateTemplate(const String& name, bool isVariable, FileRange fileRange) {
  for (auto* temp : generics) {
    if (temp->getName() == name) {
      return new ast::GenericAbstractType(temp->getID(), name, isVariable, temp->getDefault(), std::move(fileRange));
    }
  }
  return nullptr;
}

} // namespace qat::parser