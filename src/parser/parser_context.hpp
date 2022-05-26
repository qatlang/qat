#ifndef QAT_PARSER_PARSER_CONTEXT_HPP
#define QAT_PARSER_PARSER_CONTEXT_HPP

#include "../AST/types/qat_type.hpp"
#include "../utils/types.hpp"
#include <map>
#include <string>

namespace qat {
namespace parser {
/**
 * @brief ParserContext will be shared between different types of parser
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
   * @brief Add an alias for an entity
   *
   * @param name The name of the alias
   * @param value The entity for which the alias has to be added
   */
  void add_alias(const std::string name, const std::string value);

  /**
   * @brief Get the value of the provided alias. Whether alias is present for
   * this name has to be checked before calling this function
   *
   * @param name Name of the alias
   * @return std::string Name of the entity
   */
  std::string get_alias(const std::string name);

  /**
   * @brief Add an alias for a type
   *
   * @param name The name of the alias
   * @param value The type for which the alias has to be added
   */
  void add_type_alias(const std::string name, const qat::AST::QatType value);

  /**
   * @brief Get the type value of the provided alias. Whether type alias is
   * present for this name has to be checked before calling this function
   *
   * @param name Name of the type alias
   * @return qat::AST::QatType Type value of the alias
   */
  qat::AST::QatType get_type_alias(const std::string name);

  /**
   * @brief Checks whether there is an alias for an entity, with the provided
   * name
   *
   * @param name Name of the potential alias
   * @return true If there is an alias with the specified name
   * @return false IF there is no alias with the specified name
   */
  bool has_alias(const std::string name);

  /**
   * @brief Checks whether there is an alias for a type, with the provided
   * name
   *
   * @param name Name of the potential type alias
   * @return true If there is an alias with the specified name
   * @return false IF there is no alias with the specified name
   */
  bool has_type_alias(const std::string name);

  /**
   * @brief Add the provided bitwidth to the list of bitwidths available for the
   * context, for signed integers.
   *
   * Values of 1, 2, 4, 8, 16, 32, 64 and 128 are ignored since in the language
   * these are available by default without explicit declaration
   *
   * @param value Bitwidth to be added
   */
  void add_signed_bitwidth(const u64 value);

  /**
   * @brief Add the provided bitwidth to the list of bitwidths available for the
   * context, for unsigned integers
   *
   * Values of 1, 2, 4, 8, 16, 32, 64 and 128 are ignored since in the
   * language these are available by default without explicit declaration
   *
   * @param value Bitwidth to be added
   */
  void add_unsigned_bitwidth(const u64 value);

  /**
   * @brief Whether the provided bitwidth is already in the list of used
   * bitwidths for signed integers
   *
   * Values of 1, 2, 4, 8, 16, 32, 64, and 128 will return true since in the
   * language these are available by default without explicit declaration
   *
   * @param value Bitwidth to be checked for
   * @return true If the value is present
   * @return false If the value is not present
   */
  bool has_signed_bitwidth(const u64 value);

  /**
   * @brief Whether the provided bitwidth is already in the list of used
   * bitwidths for unsigned integers
   *
   * Values of 1, 2, 4, 8, 16, 32, 64, 128 will return true since in the
   * language these are available by default without explicit declaration
   *
   * @param value Bitwidth to be checked for
   * @return true If the value is present
   * @return false If the value is not present
   */
  bool has_unsigned_bitwidth(const u64 value);

private:
  /**
   * @brief All aliases declared by the user in this parser context
   *
   */
  std::map<std::string, std::string> aliases;

  /**
   * @brief All type aliases declared by the user in this parser context
   *
   */
  std::map<std::string, qat::AST::QatType> type_aliases;

  /**
   * @brief All bitwidths declared to be available by the user, for signed
   * integers
   *
   */
  std::vector<u64> signed_bitwidths;

  /**
   * @brief All bitwidths declared to be available by the user, for unsigned
   * integers
   *
   */
  std::vector<u64> unsigned_bitwidths;
};
} // namespace parser
} // namespace qat

#endif