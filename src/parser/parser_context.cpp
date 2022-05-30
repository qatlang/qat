#include "./parser_context.hpp"

namespace qat {
namespace parser {

ParserContext::ParserContext() : aliases(), type_aliases() {}

ParserContext::ParserContext(ParserContext &other)
    : aliases(other.aliases), type_aliases(other.type_aliases) {}

void ParserContext::add_alias(const std::string name, const std::string value) {
  if (!aliases.contains(name)) {
    aliases.insert({name, value});
  }
}

std::string ParserContext::get_alias(const std::string name) {
  return aliases[name];
}

AST::QatType ParserContext::get_type_alias(const std::string name) {
  return type_aliases.find(name)->second;
}

void ParserContext::add_type_alias(const std::string name,
                                   const qat::AST::QatType value) {
  if (!type_aliases.contains(name)) {
    type_aliases.insert({name, value});
  }
}

bool ParserContext::has_alias(const std::string name) {
  return aliases.contains(name);
}

bool ParserContext::has_type_alias(const std::string name) {
  return type_aliases.contains(name);
}

void ParserContext::add_signed_bitwidth(const u64 value) {
  if (value != 1 && value != 8 && value != 16 && value != 32 && value != 64 &&
      value != 128) {
    signed_bitwidths.push_back(value);
  }
}

bool ParserContext::has_signed_bitwidth(const u64 value) {
  if (value == 1 && value == 8 && value == 16 && value == 32 && value == 64 &&
      value == 128) {
    return true;
  }
  for (auto item : signed_bitwidths) {
    if (item == value) {
      return true;
    }
  }
  return false;
}

bool ParserContext::has_unsigned_bitwidth(const u64 value) {
  if (value == 1 && value == 8 && value == 16 && value == 32 && value == 64 &&
      value == 128) {
    return true;
  }
  for (auto item : unsigned_bitwidths) {
    if (item == value) {
      return true;
    }
  }
  return false;
}

void ParserContext::add_unsigned_bitwidth(const u64 value) {
  if (value != 1 && value != 8 && value != 16 && value != 32 && value != 64 &&
      value != 128) {
    unsigned_bitwidths.push_back(value);
  }
}

} // namespace parser
} // namespace qat