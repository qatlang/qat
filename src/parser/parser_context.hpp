#ifndef QAT_PARSER_PARSER_CONTEXT_HPP
#define QAT_PARSER_PARSER_CONTEXT_HPP

#include "../ast/types/qat_type.hpp"
#include "../utils/helpers.hpp"
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
   *  Add an alias for a type
   *
   * @param name The name of the alias
   * @param value The type for which the alias has to be added
   */
  void add_type_alias(const String name, ast::QatType *value);

  /**
   *  Get the type value of the provided alias. Whether type alias is
   * present for this name has to be checked before calling this function
   *
   * @param name Name of the type alias
   * @return qat::ast::QatType Type value of the alias
   */
  ast::QatType *get_type_alias(const String name) const;

  /**
   *  Checks whether there is an alias for an entity, with the provided
   * name
   *
   * @param name Name of the potential alias
   * @return true If there is an alias with the specified name
   * @return false IF there is no alias with the specified name
   */
  bool has_alias(const String name) const;

  /**
   *  Checks whether there is an alias for a type, with the provided
   * name
   *
   * @param name Name of the potential type alias
   * @return true If there is an alias with the specified name
   * @return false IF there is no alias with the specified name
   */
  bool has_type_alias(const String name) const;

  /**
   *  Add the provided bitwidth to the list of bitwidths available for the
   * context, for signed integers.
   *
   * Values of 1, 2, 4, 8, 16, 32, 64 and 128 are ignored since in the language
   * these are available by default without explicit declaration
   *
   * @param value Bitwidth to be added
   */
  void add_signed_bitwidth(const u64 value);

  /**
   *  Add the provided bitwidth to the list of bitwidths available for the
   * context, for u64 integers
   *
   * Values of 1, 2, 4, 8, 16, 32, 64 and 128 are ignored since in the
   * language these are available by default without explicit declaration
   *
   * @param value Bitwidth to be added
   */
  void add_unsigned_bitwidth(const u64 value);

  /**
   *  Whether the provided bitwidth is already in the list of used
   * bitwidths for signed integers
   *
   * Values of 1, 2, 4, 8, 16, 32, 64, and 128 will return true since in the
   * language these are available by default without explicit declaration
   *
   * @param value Bitwidth to be checked for
   * @return true If the value is present
   * @return false If the value is not present
   */
  bool has_signed_bitwidth(const u64 value) const;

  /**
   *  Whether the provided bitwidth is already in the list of used
   * bitwidths for u64 integers
   *
   * Values of 1, 2, 4, 8, 16, 32, 64, 128 will return true since in the
   * language these are available by default without explicit declaration
   *
   * @param value Bitwidth to be checked for
   * @return true If the value is present
   * @return false If the value is not present
   */
  bool has_unsigned_bitwidth(const u64 value) const;

  /**
   * Whether the provided name is an existing typename in the previous scope
   *
   * @param name Name to check for
   * @return true If the typename exists
   * @return false If the typename does not exist
   */
  bool has_template_typename(const String name) const;

  /**
   * Add a new typename to the current context to be used by child nodes
   *
   * @param name
   */
  void add_template_typename(const String name);

private:
  /**
   *  All aliases declared by the user in this parser context
   *
   */
  std::map<String, String> aliases;

  /**
   *  All type aliases declared by the user in this parser context
   *
   */
  std::map<String, ast::QatType *> type_aliases;

  /**
   *  All bitwidths declared to be available by the user, for signed
   * integers
   *
   */
  Vec<u64> signed_bitwidths;

  /**
   *  All bitwidths declared to be available by the user, for unsigned
   * integers
   *
   */
  Vec<u64> unsigned_bitwidths;

  /**
   *  All template typenames available in the current scope
   *
   */
  Vec<String> template_typenames;
};

} // namespace qat::parser

#endif