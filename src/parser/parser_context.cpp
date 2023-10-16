#include "./parser_context.hpp"
#include "../ast/types/generic_abstract.hpp"

namespace qat::parser {

ParserContext::ParserContext() = default;

bool ParserContext::hasTypedGeneric(const String& name) const {
  for (auto* tname : generics) {
    if (tname->isTyped() && tname->getName().value == name) {
      return true;
    }
  }
  return false;
}

bool ParserContext::hasConstGeneric(const String& name) const {
  for (auto* tname : generics) {
    if (tname->isPrerun() && tname->getName().value == name) {
      return true;
    }
  }
  return false;
}

void ParserContext::addAbstractGeneric(ast::GenericAbstractType* genericType) { generics.push_back(genericType); }

void ParserContext::removeNamedGenericAbstract(const String& name) {
  for (auto temp = generics.begin(); temp != generics.end(); temp++) {
    if ((*temp)->getName().value == name) {
      generics.erase(temp);
      break;
    }
  }
}

ast::TypedGeneric* ParserContext::getTypedGeneric(const String& name) {
  for (auto* temp : generics) {
    if (temp->isTyped() && temp->getName().value == name) {
      return temp->asTyped();
    }
  }
  return nullptr;
}

ast::PrerunGeneric* ParserContext::getConstGeneric(const String& name) {
  for (auto* temp : generics) {
    if (temp->isPrerun() && temp->getName().value == name) {
      return temp->asPrerun();
    }
  }
  return nullptr;
}

} // namespace qat::parser