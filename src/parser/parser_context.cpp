#include "./parser_context.hpp"
#include "../ast/types/generic_abstract.hpp"

namespace qat::parser {

ParserContext::ParserContext() = default;

bool ParserContext::hasTemplate(const String& name) const {
  for (auto* tname : templates) {
    if (tname->getName() == name) {
      return true;
    }
  }
  return false;
}

void ParserContext::addTemplate(ast::GenericAbstractType* templateType) { templates.push_back(templateType); }

void ParserContext::removeTemplate(const String& name) {
  for (auto temp = templates.begin(); temp != templates.end(); temp++) {
    if ((*temp)->getName() == name) {
      templates.erase(temp);
      break;
    }
  }
}

ast::GenericAbstractType* ParserContext::getTemplate(const String& name) {
  for (auto* temp : templates) {
    if (temp->getName() == name) {
      return temp;
    }
  }
  return nullptr;
}

ast::GenericAbstractType* ParserContext::duplicateTemplate(const String& name, bool isVariable, FileRange fileRange) {
  for (auto* temp : templates) {
    if (temp->getName() == name) {
      return new ast::GenericAbstractType(temp->getID(), name, isVariable, temp->getDefault(), fileRange);
    }
  }
  return nullptr;
}

} // namespace qat::parser