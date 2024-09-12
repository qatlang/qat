#include "./parser_context.hpp"
#include "../ast/types/generic_abstract.hpp"

namespace qat::parser {

ParserContext::ParserContext() = default;

bool ParserContext::has_typed_generic(const String& name) const {
  for (auto* tname : generics) {
    if (tname->is_typed() && tname->get_name().value == name) {
      return true;
    }
  }
  return false;
}

bool ParserContext::has_prerun_generic(const String& name) const {
  for (auto* tname : generics) {
    if (tname->is_prerun() && tname->get_name().value == name) {
      return true;
    }
  }
  return false;
}

void ParserContext::add_abstract_generic(ast::GenericAbstractType* genericType) { generics.push_back(genericType); }

void ParserContext::remove_named_generic_abstract(const String& name) {
  for (auto temp = generics.begin(); temp != generics.end(); temp++) {
    if ((*temp)->get_name().value == name) {
      generics.erase(temp);
      break;
    }
  }
}

ast::TypedGenericAbstract* ParserContext::get_typed_generic(const String& name) {
  for (auto* temp : generics) {
    if (temp->is_typed() && temp->get_name().value == name) {
      return temp->as_typed();
    }
  }
  return nullptr;
}

ast::PrerunGenericAbstract* ParserContext::get_prerun_generic(const String& name) {
  for (auto* temp : generics) {
    if (temp->is_prerun() && temp->get_name().value == name) {
      return temp->as_prerun();
    }
  }
  return nullptr;
}

} // namespace qat::parser
