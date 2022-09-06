#ifndef QAT_PARSER_PARSER_CONTEXT_HPP
#define QAT_PARSER_PARSER_CONTEXT_HPP

#include "../ast/types/qat_type.hpp"
#include "../utils/helpers.hpp"
#include <deque>
#include <map>
#include <string>

namespace qat::parser {

/**
 *  ParserContext will be shared between different types of parser
 * functions for managing shared entities/definitions within the scope
 *
 */
class ParserContext {
public:
  // Empty ParserContext instance
  ParserContext();

  // Copies the contents of the other ParserContext instance into this
  ParserContext(ParserContext &other);

  /**
   *  Add an alias for an entity
   *
   * @param name The name of the alias
   * @param value The entity for which the alias has to be added
   */
  void add_alias(const String name, const String value);

  /**
   *  Get the value of the provided alias. Whether alias is present for
   * this name has to be checked before calling this function
   *
   * @param name Name of the alias
   * @return String Name of the entity
   */
  String get_alias(const String name) const;

  /**
   *  Checks whether there is an alias for an entity, with the provided
   * name
   *
   * @param name Name of the potential alias
   * @return true If there is an alias with the specified name
   * @return false IF there is no alias with the specified name
   */
  bool has_alias(const String name) const;

  // Whether the provided name is an existing typename in the previous scope
  useit bool hasTemplate(const String &name) const;

  // Add a new typename to the current context to be used by child nodes
  void addTemplate(ast::TemplatedType *templateType);

  void removeTemplate(const String &name);

  useit ast::TemplatedType *getTemplate(const String &name);

  useit ast::TemplatedType *duplicateTemplate(const String    &name,
                                              bool             isVariable,
                                              utils::FileRange fileRange);

private:
  // All aliases declared by the user in this parser context
  std::map<String, String> aliases;

  // All bitwidths declared to be available by the user, for signed
  // integers
  Vec<u64> signed_bitwidths;

  // All bitwidths declared to be available by the user, for unsigned
  // integers
  Vec<u64> unsigned_bitwidths;

  // All template typenames available in the current scope
  std::deque<ast::TemplatedType *> templates;
};

} // namespace qat::parser

#endif