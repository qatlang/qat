#include "./parser_context.hpp"
#include "../ast/types/templated.hpp"

namespace qat::parser {

ParserContext::ParserContext() = default;

ParserContext::ParserContext(ParserContext &other)
    : aliases(other.aliases), templates(other.templates) {}

void ParserContext::add_alias(const String name, const String value) {
  if (!aliases.contains(name)) {
    aliases.insert({name, value});
  }
}

String ParserContext::get_alias(const String name) const {
  return aliases.find(name)->second;
}

bool ParserContext::has_alias(const String name) const {
  return aliases.contains(name);
}

bool ParserContext::hasTemplate(const String &name) const {
  for (auto *tname : templates) {
    if (tname->getName() == name) {
      return true;
    }
  }
  return false;
}

void ParserContext::addTemplate(ast::TemplatedType *templateType) {
  templates.push_back(templateType);
}

void ParserContext::removeTemplate(const String &name) {
  for (auto temp = templates.begin(); temp != templates.end(); temp++) {
    if ((*temp)->getName() == name) {
      templates.erase(temp);
      break;
    }
  }
}

ast::TemplatedType *ParserContext::getTemplate(const String &name) {
  for (auto *temp : templates) {
    if (temp->getName() == name) {
      return temp;
    }
  }
  return nullptr;
}

ast::TemplatedType *
ParserContext::duplicateTemplate(const String &name, bool isVariable,
                                 utils::FileRange fileRange) {
  for (auto *temp : templates) {
    if (temp->getName() == name) {
      return new ast::TemplatedType(temp->getID(), name, isVariable, fileRange);
    }
  }
  return nullptr;
}

} // namespace qat::parser